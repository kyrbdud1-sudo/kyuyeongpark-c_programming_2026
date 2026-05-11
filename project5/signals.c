#include "signals.h"

#include "indicators.h"

#include <math.h>
#include <stdlib.h>

int signal_within_window(const int *signal_array, int current_index, int lookback_window)
{
    int i;
    int start;

    if (signal_array == NULL || current_index < 0 || lookback_window < 0) {
        return 0;
    }

    start = current_index - lookback_window;
    if (start < 0) {
        start = 0;
    }

    for (i = start; i <= current_index; i++) {
        if (signal_array[i] == 1) {
            return 1;
        }
    }

    return 0;
}

static void clear_signals(size_t count, int *buy_signal, int *sell_signal)
{
    size_t i;

    for (i = 0; i < count; i++) {
        buy_signal[i] = 0;
        sell_signal[i] = 0;
    }
}

static void generate_cross_signals(const double *line_a, const double *line_b, size_t count,
                                   int *buy_signal, int *sell_signal)
{
    size_t i;

    for (i = 1; i < count; i++) {
        if (isnan(line_a[i - 1]) || isnan(line_b[i - 1]) ||
            isnan(line_a[i]) || isnan(line_b[i])) {
            continue;
        }

        /* Crossover signals compare yesterday's relation with today's relation. */
        if (line_a[i - 1] <= line_b[i - 1] && line_a[i] > line_b[i]) {
            buy_signal[i] = 1;
        } else if (line_a[i - 1] >= line_b[i - 1] && line_a[i] < line_b[i]) {
            sell_signal[i] = 1;
        }
    }
}

static void generate_rsi_signals_from_values(const double *rsi, size_t count,
                                             double oversold, double overbought,
                                             int *buy_signal, int *sell_signal)
{
    size_t i;

    for (i = 1; i < count; i++) {
        if (isnan(rsi[i - 1]) || isnan(rsi[i])) {
            continue;
        }

        if (rsi[i - 1] < oversold && rsi[i] >= oversold) {
            buy_signal[i] = 1;
        } else if (rsi[i - 1] > overbought && rsi[i] <= overbought) {
            sell_signal[i] = 1;
        }
    }
}

static int build_sma_or_ema_signals(const double *close, size_t count, const StrategyParams *params,
                                    int use_ema, int *buy_signal, int *sell_signal)
{
    double *short_line = (double *)malloc(count * sizeof(double));
    double *long_line = (double *)malloc(count * sizeof(double));

    if (short_line == NULL || long_line == NULL) {
        free(short_line);
        free(long_line);
        return 0;
    }

    if (use_ema) {
        calculate_ema(close, count, params->short_period, short_line);
        calculate_ema(close, count, params->long_period, long_line);
    } else {
        calculate_sma(close, count, params->short_period, short_line);
        calculate_sma(close, count, params->long_period, long_line);
    }

    generate_cross_signals(short_line, long_line, count, buy_signal, sell_signal);
    free(short_line);
    free(long_line);
    return 1;
}

static int build_macd_signals(const double *close, size_t count, const StrategyParams *params,
                              int *buy_signal, int *sell_signal)
{
    double *macd_line = (double *)malloc(count * sizeof(double));
    double *signal_line = (double *)malloc(count * sizeof(double));

    if (macd_line == NULL || signal_line == NULL) {
        free(macd_line);
        free(signal_line);
        return 0;
    }

    calculate_macd(close, count, params->macd_fast_period, params->macd_slow_period,
                   params->macd_signal_period, macd_line, signal_line);
    generate_cross_signals(macd_line, signal_line, count, buy_signal, sell_signal);

    free(macd_line);
    free(signal_line);
    return 1;
}

static int build_rsi_signals(const double *close, size_t count, const StrategyParams *params,
                             int *buy_signal, int *sell_signal)
{
    double *rsi = (double *)malloc(count * sizeof(double));

    if (rsi == NULL) {
        return 0;
    }

    calculate_rsi(close, count, params->rsi_period, rsi);
    generate_rsi_signals_from_values(rsi, count, params->rsi_oversold_level,
                                     params->rsi_overbought_level, buy_signal, sell_signal);

    free(rsi);
    return 1;
}

static int build_combined_signals(const double *close, size_t count, const StrategyParams *params,
                                  int *buy_signal, int *sell_signal)
{
    size_t i;
    int ok;
    int *rsi_buy_signal = (int *)calloc(count, sizeof(int));
    int *rsi_sell_signal = (int *)calloc(count, sizeof(int));
    int *macd_buy_signal = (int *)calloc(count, sizeof(int));
    int *macd_sell_signal = (int *)calloc(count, sizeof(int));

    if (rsi_buy_signal == NULL || rsi_sell_signal == NULL ||
        macd_buy_signal == NULL || macd_sell_signal == NULL) {
        free(rsi_buy_signal);
        free(rsi_sell_signal);
        free(macd_buy_signal);
        free(macd_sell_signal);
        return 0;
    }

    ok = build_rsi_signals(close, count, params, rsi_buy_signal, rsi_sell_signal) &&
         build_macd_signals(close, count, params, macd_buy_signal, macd_sell_signal);

    if (ok) {
        for (i = 0; i < count; i++) {
            int idx = (int)i;
            int has_rsi_buy = signal_within_window(rsi_buy_signal, idx, params->buy_lookback_window);
            int has_macd_buy = signal_within_window(macd_buy_signal, idx, params->buy_lookback_window);
            int has_rsi_sell = signal_within_window(rsi_sell_signal, idx, params->sell_lookback_window);
            int has_macd_sell = signal_within_window(macd_sell_signal, idx, params->sell_lookback_window);

            /* The combined strategy accepts either indicator first if both appear in the window. */
            buy_signal[i] = has_rsi_buy && has_macd_buy;
            if (params->sell_combination_mode == 1) {
                sell_signal[i] = has_rsi_sell && has_macd_sell;
            } else {
                sell_signal[i] = has_rsi_sell || has_macd_sell;
            }
        }
    }

    free(rsi_buy_signal);
    free(rsi_sell_signal);
    free(macd_buy_signal);
    free(macd_sell_signal);
    return ok;
}

static int build_ema_trend_signals(const double *close, size_t count, const StrategyParams *params,
                                   int *buy_signal, int *sell_signal)
{
    double *fast = (double *)malloc(count * sizeof(double));
    double *slow = (double *)malloc(count * sizeof(double));

    if (fast == NULL || slow == NULL) {
        free(fast);
        free(slow);
        return 0;
    }

    calculate_ema(close, count, params->entry_fast_ema_period, fast);
    calculate_ema(close, count, params->entry_slow_ema_period, slow);
    generate_cross_signals(fast, slow, count, buy_signal, sell_signal);

    free(fast);
    free(slow);
    return 1;
}

static int build_turtle_signals(const PriceData *data, size_t count, const StrategyParams *params,
                                int *buy_signal, int *sell_signal)
{
    double *close = (double *)malloc(count * sizeof(double));
    double *high = (double *)malloc(count * sizeof(double));
    double *low = (double *)malloc(count * sizeof(double));
    double *upper = (double *)malloc(count * sizeof(double));
    double *lower = (double *)malloc(count * sizeof(double));
    double *atr = (double *)malloc(count * sizeof(double));
    size_t i;

    if (close == NULL || high == NULL || low == NULL || upper == NULL || lower == NULL ||
        atr == NULL) {
        free(close);
        free(high);
        free(low);
        free(upper);
        free(lower);
        free(atr);
        return 0;
    }

    for (i = 0; i < count; i++) {
        close[i] = data[i].close;
        high[i] = data[i].high;
        low[i] = data[i].low;
    }

    calculate_donchian_upper(high, count, params->entry_breakout_days, 1, upper);
    calculate_donchian_lower(low, count, params->entry_breakout_days, 1, lower);
    calculate_atr(high, low, close, count, params->atr_period, atr);

    for (i = 1; i < count; i++) {
        if (!isnan(upper[i]) && data[i].close > upper[i]) {
            buy_signal[i] = 1;
        }
        if (!isnan(lower[i]) && data[i].close < lower[i]) {
            sell_signal[i] = 1;
        }
    }

    free(close);
    free(high);
    free(low);
    free(upper);
    free(lower);
    free(atr);
    return 1;
}

int generate_strategy_signals(const PriceData *data, size_t count, const StrategyParams *params,
                              int *buy_signal, int *sell_signal)
{
    double *close;
    size_t i;
    int ok;

    if (data == NULL || params == NULL || buy_signal == NULL || sell_signal == NULL) {
        return 0;
    }

    clear_signals(count, buy_signal, sell_signal);

    if (params->type == STRATEGY_DRAWDOWN_REBOUND) {
        return 1;
    }
    if (params->type == STRATEGY_TURTLE) {
        return build_turtle_signals(data, count, params, buy_signal, sell_signal);
    }

    close = (double *)malloc(count * sizeof(double));
    if (close == NULL) {
        return 0;
    }
    for (i = 0; i < count; i++) {
        close[i] = data[i].close;
    }

    switch (params->type) {
    case STRATEGY_SMA:
        ok = build_sma_or_ema_signals(close, count, params, 0, buy_signal, sell_signal);
        break;
    case STRATEGY_EMA:
        ok = build_sma_or_ema_signals(close, count, params, 1, buy_signal, sell_signal);
        break;
    case STRATEGY_MACD:
        ok = build_macd_signals(close, count, params, buy_signal, sell_signal);
        break;
    case STRATEGY_RSI:
        ok = build_rsi_signals(close, count, params, buy_signal, sell_signal);
        break;
    case STRATEGY_RSI_MACD:
        ok = build_combined_signals(close, count, params, buy_signal, sell_signal);
        break;
    case STRATEGY_EMA_TREND:
        ok = build_ema_trend_signals(close, count, params, buy_signal, sell_signal);
        break;
    default:
        ok = 0;
        break;
    }

    free(close);
    return ok;
}

const char *strategy_name(StrategyType type)
{
    switch (type) {
    case STRATEGY_SMA: return "SMA crossover";
    case STRATEGY_EMA: return "EMA crossover";
    case STRATEGY_MACD: return "MACD crossover";
    case STRATEGY_RSI: return "RSI strategy";
    case STRATEGY_RSI_MACD: return "RSI + MACD combined strategy";
    case STRATEGY_DRAWDOWN_REBOUND: return "Drawdown accumulation strategy";
    case STRATEGY_EMA_TREND: return "20/200 EMA entry + 50 EMA exit long-short strategy";
    case STRATEGY_TURTLE: return "Turtle trading long-short strategy";
    default: return "Unknown strategy";
    }
}
