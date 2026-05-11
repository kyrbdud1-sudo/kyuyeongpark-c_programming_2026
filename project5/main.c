#include <stdio.h>
#include <stdlib.h>

#include "backtest.h"
#include "data_loader.h"
#include "signals.h"

static int read_int(const char *prompt, int *out)
{
    int value;

    printf("%s", prompt);
    if (scanf("%d", &value) != 1) {
        fprintf(stderr, "Invalid integer input.\n");
        return 0;
    }
    *out = value;
    return 1;
}

static int read_double(const char *prompt, double *out)
{
    double value;

    printf("%s", prompt);
    if (scanf("%lf", &value) != 1) {
        fprintf(stderr, "Invalid number input.\n");
        return 0;
    }
    *out = value;
    return 1;
}

static int valid_rsi_levels(double oversold, double overbought)
{
    return oversold >= 0.0 && oversold <= 100.0 &&
           overbought >= 0.0 && overbought <= 100.0 &&
           oversold < overbought;
}

static const char *strategy_position_mode(StrategyType type)
{
    if (type == STRATEGY_DRAWDOWN_REBOUND) {
        return "Long-only";
    }
    if (type == STRATEGY_EMA_TREND || type == STRATEGY_TURTLE) {
        return "Long/Short, exits go to cash";
    }
    return "Long/Short";
}

static int read_backtest_period(int *window_years)
{
    int choice;

    printf("\nSelect backtest period:\n");
    printf("1: 10 years\n");
    printf("2: 20 years\n");

    if (!read_int("Choice: ", &choice) || (choice != 1 && choice != 2)) {
        fprintf(stderr, "Backtest period choice must be 1 or 2.\n");
        return 0;
    }

    *window_years = (choice == 1) ? 10 : 20;
    return 1;
}

static int read_capital_mode(CapitalMode *capital_mode)
{
    int choice;

    printf("\nSelect capital mode:\n");
    printf("1: Monthly contribution - invest while holding\n");
    printf("2: Monthly contribution - accumulate cash until signal\n");
    printf("3: Lump sum - initial 60,000,000 only\n");

    if (!read_int("Choice: ", &choice) || choice < 1 || choice > 3) {
        fprintf(stderr, "Invalid capital mode. Using default mode 1.\n");
        *capital_mode = CAPITAL_MONTHLY_INVEST_WHILE_HOLDING;
        return 1;
    }

    *capital_mode = (CapitalMode)choice;
    return 1;
}

static int read_strategy_params(StrategyParams *params)
{
    int strategy_choice;

    printf("\nSelect strategy:\n");
    printf("1: SMA crossover\n");
    printf("2: EMA crossover\n");
    printf("3: MACD crossover\n");
    printf("4: RSI strategy\n");
    printf("5: RSI + MACD combined strategy\n");
    printf("6: Drawdown accumulation strategy\n");
    printf("7: 20/200 EMA entry + 50 EMA exit long-short strategy\n");
    printf("8: Turtle trading long-short strategy\n");

    if (!read_int("Choice: ", &strategy_choice) || strategy_choice < 1 || strategy_choice > 8) {
        fprintf(stderr, "Strategy choice must be between 1 and 8.\n");
        return 0;
    }

    params->type = (StrategyType)strategy_choice;
    params->short_period = 20;
    params->long_period = 60;
    params->macd_fast_period = 12;
    params->macd_slow_period = 26;
    params->macd_signal_period = 9;
    params->rsi_period = 14;
    params->rsi_oversold_level = 30.0;
    params->rsi_overbought_level = 70.0;
    params->buy_lookback_window = 2;
    params->sell_lookback_window = 2;
    params->sell_combination_mode = 1;
    params->drawdown_percent = 30.0;
    params->peak_lookback_days = 0;
    params->entry_fast_ema_period = 20;
    params->entry_slow_ema_period = 200;
    params->exit_ema_period = 50;
    params->entry_breakout_days = 20;
    params->exit_breakout_days = 10;
    params->atr_period = 20;
    params->use_pyramiding = 0;
    params->max_units = 4;
    params->add_unit_atr_multiple = 0.5;

    if (params->type == STRATEGY_SMA || params->type == STRATEGY_EMA) {
        if (!read_int("short_period: ", &params->short_period) ||
            !read_int("long_period: ", &params->long_period)) {
            return 0;
        }
        if (params->short_period <= 1 || params->long_period <= 1 ||
            params->short_period >= params->long_period) {
            fprintf(stderr, "Periods must be > 1 and short_period must be smaller than long_period.\n");
            return 0;
        }
    } else if (params->type == STRATEGY_MACD) {
        if (!read_int("fast_period (default 12): ", &params->macd_fast_period) ||
            !read_int("slow_period (default 26): ", &params->macd_slow_period) ||
            !read_int("signal_period (default 9): ", &params->macd_signal_period)) {
            return 0;
        }
        if (params->macd_fast_period <= 1 || params->macd_slow_period <= 1 ||
            params->macd_signal_period <= 1 ||
            params->macd_fast_period >= params->macd_slow_period) {
            fprintf(stderr, "MACD periods must be > 1 and fast_period must be smaller than slow_period.\n");
            return 0;
        }
    } else if (params->type == STRATEGY_RSI) {
        if (!read_int("rsi_period (default 14): ", &params->rsi_period) ||
            !read_double("oversold_level (default 30): ", &params->rsi_oversold_level) ||
            !read_double("overbought_level (default 70): ", &params->rsi_overbought_level)) {
            return 0;
        }
        if (params->rsi_period <= 1 ||
            !valid_rsi_levels(params->rsi_oversold_level, params->rsi_overbought_level)) {
            fprintf(stderr, "RSI period must be > 1 and levels must satisfy 0 <= oversold < overbought <= 100.\n");
            return 0;
        }
    } else if (params->type == STRATEGY_RSI_MACD) {
        if (!read_int("rsi_period (default 14): ", &params->rsi_period) ||
            !read_double("rsi_oversold_level (default 30): ", &params->rsi_oversold_level) ||
            !read_double("rsi_overbought_level (default 70): ", &params->rsi_overbought_level) ||
            !read_int("macd_fast_period (default 12): ", &params->macd_fast_period) ||
            !read_int("macd_slow_period (default 26): ", &params->macd_slow_period) ||
            !read_int("macd_signal_period (default 9): ", &params->macd_signal_period) ||
            !read_int("buy_lookback_window: ", &params->buy_lookback_window) ||
            !read_int("sell_lookback_window: ", &params->sell_lookback_window) ||
            !read_int("sell_combination_mode (1=AND, 2=OR): ", &params->sell_combination_mode)) {
            return 0;
        }
        if (params->rsi_period <= 1 || params->macd_fast_period <= 1 ||
            params->macd_slow_period <= 1 || params->macd_signal_period <= 1 ||
            params->macd_fast_period >= params->macd_slow_period ||
            !valid_rsi_levels(params->rsi_oversold_level, params->rsi_overbought_level) ||
            params->buy_lookback_window < 0 || params->sell_lookback_window < 0 ||
            (params->sell_combination_mode != 1 && params->sell_combination_mode != 2)) {
            fprintf(stderr, "Invalid combined strategy parameters.\n");
            return 0;
        }
    } else if (params->type == STRATEGY_DRAWDOWN_REBOUND) {
        if (!read_double("drawdown_percent (default 30): ", &params->drawdown_percent) ||
            !read_int("peak_lookback_days (0=cumulative): ", &params->peak_lookback_days)) {
            return 0;
        }
        if (params->drawdown_percent <= 0.0 || params->peak_lookback_days < 0) {
            fprintf(stderr, "Drawdown percent must be positive and peak lookback must be >= 0.\n");
            return 0;
        }
    } else if (params->type == STRATEGY_EMA_TREND) {
        if (!read_int("entry_fast_ema_period (default 20): ", &params->entry_fast_ema_period) ||
            !read_int("entry_slow_ema_period (default 200): ", &params->entry_slow_ema_period) ||
            !read_int("exit_ema_period (default 50): ", &params->exit_ema_period)) {
            return 0;
        }
        if (params->entry_fast_ema_period <= 1 || params->entry_slow_ema_period <= 1 ||
            params->exit_ema_period <= 1 ||
            params->entry_fast_ema_period >= params->entry_slow_ema_period) {
            fprintf(stderr, "EMA trend periods must be > 1 and fast must be smaller than slow.\n");
            return 0;
        }
    } else if (params->type == STRATEGY_TURTLE) {
        if (!read_int("entry_breakout_days (default 20): ", &params->entry_breakout_days) ||
            !read_int("exit_breakout_days (default 10): ", &params->exit_breakout_days) ||
            !read_int("atr_period (default 20): ", &params->atr_period) ||
            !read_int("use_pyramiding (0/1, simplified as 0): ", &params->use_pyramiding) ||
            !read_int("max_units (default 4): ", &params->max_units) ||
            !read_double("add_unit_atr_multiple (default 0.5): ", &params->add_unit_atr_multiple)) {
            return 0;
        }
        if (params->entry_breakout_days <= 1 || params->exit_breakout_days <= 1 ||
            params->atr_period <= 1 || params->max_units <= 0 ||
            params->add_unit_atr_multiple <= 0.0 ||
            (params->use_pyramiding != 0 && params->use_pyramiding != 1)) {
            fprintf(stderr, "Invalid Turtle strategy parameters.\n");
            return 0;
        }
        params->use_pyramiding = 0;
    }

    return 1;
}

static void write_strategy_info(const char *data_file, const StrategyParams *params,
                                int window_years, CapitalMode capital_mode)
{
    FILE *fp = fopen("strategy_info.txt", "w");

    if (fp == NULL) {
        fprintf(stderr, "Warning: failed to write strategy_info.txt.\n");
        return;
    }

    fprintf(fp, "Data: %s\n", data_file);
    fprintf(fp, "Strategy: %s\n", strategy_name(params->type));
    fprintf(fp, "Backtest period: %d years\n", window_years);
    fprintf(fp, "Capital mode: %s\n", capital_mode_name(capital_mode));
    if (capital_mode == CAPITAL_LUMP_SUM) {
        fprintf(fp, "Initial capital: %.0f\n", LUMP_SUM_INITIAL_CAPITAL);
    } else {
        fprintf(fp, "Monthly contribution: %.0f\n", MONTHLY_INVESTMENT);
    }
    fprintf(fp, "Benchmark type: %s\n", benchmark_type_name(capital_mode));
    fprintf(fp, "Position mode: %s\n", strategy_position_mode(params->type));
    fprintf(fp, "Long/Short enabled: %s\n",
            params->type == STRATEGY_DRAWDOWN_REBOUND ? "no" : "yes");
    fprintf(fp, "Parameters: ");
    switch (params->type) {
    case STRATEGY_SMA:
    case STRATEGY_EMA:
        fprintf(fp, "short_period=%d long_period=%d\n",
                params->short_period, params->long_period);
        break;
    case STRATEGY_MACD:
        fprintf(fp, "macd_fast=%d macd_slow=%d macd_signal=%d\n",
                params->macd_fast_period, params->macd_slow_period,
                params->macd_signal_period);
        break;
    case STRATEGY_RSI:
        fprintf(fp, "rsi_period=%d oversold=%.2f overbought=%.2f\n",
                params->rsi_period, params->rsi_oversold_level,
                params->rsi_overbought_level);
        break;
    case STRATEGY_RSI_MACD:
        fprintf(fp, "rsi_period=%d oversold=%.2f overbought=%.2f "
                    "macd_fast=%d macd_slow=%d macd_signal=%d "
                    "buy_lookback=%d sell_lookback=%d sell_mode=%d\n",
                params->rsi_period, params->rsi_oversold_level,
                params->rsi_overbought_level, params->macd_fast_period,
                params->macd_slow_period, params->macd_signal_period,
                params->buy_lookback_window, params->sell_lookback_window,
                params->sell_combination_mode);
        break;
    case STRATEGY_DRAWDOWN_REBOUND:
        fprintf(fp, "drawdown=%.2f peak_lookback=%d\n",
                params->drawdown_percent, params->peak_lookback_days);
        break;
    case STRATEGY_EMA_TREND:
        fprintf(fp, "entry_fast_ema=%d entry_slow_ema=%d exit_ema=%d\n",
                params->entry_fast_ema_period, params->entry_slow_ema_period,
                params->exit_ema_period);
        break;
    case STRATEGY_TURTLE:
        fprintf(fp, "entry_breakout=%d exit_breakout=%d atr_period=%d "
                    "use_pyramiding=%d max_units=%d add_unit_atr_multiple=%.2f\n",
                params->entry_breakout_days, params->exit_breakout_days,
                params->atr_period, params->use_pyramiding, params->max_units,
                params->add_unit_atr_multiple);
        break;
    default:
        fprintf(fp, "unknown\n");
        break;
    }
    fclose(fp);
}

static void print_summary(const char *data_file, const StrategyParams *params,
                          const BacktestSummary *summary, int window_years,
                          CapitalMode capital_mode)
{
    const char *benchmark_label = (capital_mode == CAPITAL_LUMP_SUM) ?
        "Buy-and-hold benchmark" : "DCA";

    printf("\nBacktest summary\n");
    printf("Data file: %s\n", data_file);
    printf("Strategy: %s\n", strategy_name(params->type));
    printf("Position mode: %s\n", strategy_position_mode(params->type));
    printf("Capital mode: %s\n", capital_mode_name(capital_mode));
    printf("Benchmark type: %s\n", benchmark_type_name(capital_mode));
    printf("Backtest period: %d years\n", window_years);
    printf("Rolling %d-year windows: %zu\n", window_years, summary->window_count);
    printf("%s average cumulative return: %.2f%%\n", benchmark_label, summary->avg_dca_return);
    printf("Strategy average cumulative return: %.2f%%\n", summary->avg_strategy_return);
    printf("Strategy win rate vs %s: %.2f%%\n", benchmark_label, summary->win_rate);
    printf("%s max return: %.2f%%\n", benchmark_label, summary->max_dca_return);
    printf("%s min return: %.2f%%\n", benchmark_label, summary->min_dca_return);
    printf("Strategy max return: %.2f%%\n", summary->max_strategy_return);
    printf("Strategy min return: %.2f%%\n", summary->min_strategy_return);
    printf("Average excess return: %.2f%%\n", summary->avg_excess_return);
    if (params->type == STRATEGY_TURTLE) {
        printf("Turtle long entry count: %zu\n", summary->long_entry_count);
        printf("Turtle long exit count: %zu\n", summary->long_exit_count);
        printf("Turtle short entry count: %zu\n", summary->short_entry_count);
        printf("Turtle short exit count: %zu\n", summary->short_exit_count);
    }
}

int main(void)
{
    int data_choice;
    const char *data_file;
    PriceData *data = NULL;
    size_t count = 0;
    int *long_signal = NULL;
    int *short_signal = NULL;
    BacktestResult *results = NULL;
    size_t result_count = 0;
    StrategyParams params;
    BacktestSummary summary;
    int window_years;
    CapitalMode capital_mode;
    int exit_code = 1;

    printf("Select data file:\n");
    printf("1: kospi.csv\n");
    printf("2: sp500.csv\n");
    if (!read_int("Choice: ", &data_choice) || (data_choice != 1 && data_choice != 2)) {
        fprintf(stderr, "Data choice must be 1 or 2.\n");
        return 1;
    }
    data_file = (data_choice == 1) ? "kospi.csv" : "sp500.csv";

    if (!read_backtest_period(&window_years)) {
        return 1;
    }

    if (!read_capital_mode(&capital_mode)) {
        return 1;
    }

    if (!read_strategy_params(&params)) {
        return 1;
    }

    if (!load_csv(data_file, &data, &count) || !validate_date_order(data, count)) {
        free(data);
        return 1;
    }

    long_signal = (int *)calloc(count, sizeof(int));
    short_signal = (int *)calloc(count, sizeof(int));
    if (long_signal == NULL || short_signal == NULL) {
        fprintf(stderr, "Failed to allocate arrays.\n");
        goto cleanup;
    }

    if (!generate_strategy_signals(data, count, &params, long_signal, short_signal)) {
        fprintf(stderr, "Failed to generate strategy signals.\n");
        goto cleanup;
    }

    if (!run_long_short_strategy_backtest(data, count, &params, long_signal, short_signal,
                                          &results, &result_count, window_years,
                                          capital_mode)) {
        fprintf(stderr, "Failed to run rolling %d-year backtest.\n", window_years);
        goto cleanup;
    }

    if (!save_backtest_results("backtest_result.csv", results, result_count)) {
        goto cleanup;
    }

    write_strategy_info(data_file, &params, window_years, capital_mode);
    summary = summarize_results(results, result_count);
    print_summary(data_file, &params, &summary, window_years, capital_mode);
    printf("\nSaved: backtest_result.csv\n");
    printf("Saved: strategy_info.txt\n");

    exit_code = 0;

cleanup:
    free(data);
    free(long_signal);
    free(short_signal);
    free(results);
    return exit_code;
}
