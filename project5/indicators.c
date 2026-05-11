#include "indicators.h"

#include <math.h>
#include <stdlib.h>

void calculate_sma(const double *close, size_t count, int period, double *out)
{
    size_t i;
    double sum = 0.0;

    if (close == NULL || out == NULL || period <= 0) {
        return;
    }

    for (i = 0; i < count; i++) {
        out[i] = NAN;
        sum += close[i];

        if (i >= (size_t)period) {
            sum -= close[i - (size_t)period];
        }

        if (i + 1 >= (size_t)period) {
            out[i] = sum / period;
        }
    }
}

void calculate_ema(const double *close, size_t count, int period, double *out)
{
    size_t i;
    double sum = 0.0;
    double multiplier;

    if (close == NULL || out == NULL || period <= 0) {
        return;
    }

    for (i = 0; i < count; i++) {
        out[i] = NAN;
    }

    if (count < (size_t)period) {
        return;
    }

    for (i = 0; i < (size_t)period; i++) {
        sum += close[i];
    }

    /* Seed EMA with the SMA of the first period, then apply the standard recursive formula. */
    out[period - 1] = sum / period;
    multiplier = 2.0 / (period + 1.0);

    for (i = (size_t)period; i < count; i++) {
        out[i] = (close[i] - out[i - 1]) * multiplier + out[i - 1];
    }
}

void calculate_macd(const double *close, size_t count, int fast_period, int slow_period,
                    int signal_period, double *macd_line, double *signal_line)
{
    size_t i;
    double *fast_ema;
    double *slow_ema;
    double multiplier;
    size_t first_macd = 0;
    int found_first = 0;
    double sum = 0.0;
    size_t signal_start;

    if (close == NULL || macd_line == NULL || signal_line == NULL ||
        fast_period <= 0 || slow_period <= 0 || signal_period <= 0) {
        return;
    }

    for (i = 0; i < count; i++) {
        macd_line[i] = NAN;
        signal_line[i] = NAN;
    }

    fast_ema = (double *)malloc(count * sizeof(double));
    slow_ema = (double *)malloc(count * sizeof(double));
    if (fast_ema == NULL || slow_ema == NULL) {
        free(fast_ema);
        free(slow_ema);
        return;
    }

    calculate_ema(close, count, fast_period, fast_ema);
    calculate_ema(close, count, slow_period, slow_ema);

    for (i = 0; i < count; i++) {
        if (!isnan(fast_ema[i]) && !isnan(slow_ema[i])) {
            macd_line[i] = fast_ema[i] - slow_ema[i];
            if (!found_first) {
                first_macd = i;
                found_first = 1;
            }
        }
    }

    if (found_first && count - first_macd >= (size_t)signal_period) {
        /* The MACD signal line is an EMA of the MACD line, seeded by its first SMA. */
        signal_start = first_macd + (size_t)signal_period - 1;
        for (i = first_macd; i <= signal_start; i++) {
            sum += macd_line[i];
        }
        signal_line[signal_start] = sum / signal_period;
        multiplier = 2.0 / (signal_period + 1.0);
        for (i = signal_start + 1; i < count; i++) {
            signal_line[i] = (macd_line[i] - signal_line[i - 1]) * multiplier + signal_line[i - 1];
        }
    }

    free(fast_ema);
    free(slow_ema);
}

void calculate_rsi(const double *close, size_t count, int period, double *out)
{
    size_t i;
    double gain_sum = 0.0;
    double loss_sum = 0.0;
    double avg_gain;
    double avg_loss;

    if (close == NULL || out == NULL || period <= 0) {
        return;
    }

    for (i = 0; i < count; i++) {
        out[i] = NAN;
    }

    if (count <= (size_t)period) {
        return;
    }

    for (i = 1; i <= (size_t)period; i++) {
        double change = close[i] - close[i - 1];
        if (change >= 0.0) {
            gain_sum += change;
        } else {
            loss_sum += -change;
        }
    }

    /* Wilder RSI uses smoothed average gains and losses after the initial period. */
    avg_gain = gain_sum / period;
    avg_loss = loss_sum / period;
    out[period] = (avg_loss == 0.0) ? 100.0 : 100.0 - (100.0 / (1.0 + avg_gain / avg_loss));

    for (i = (size_t)period + 1; i < count; i++) {
        double change = close[i] - close[i - 1];
        double gain = change > 0.0 ? change : 0.0;
        double loss = change < 0.0 ? -change : 0.0;

        avg_gain = ((avg_gain * (period - 1)) + gain) / period;
        avg_loss = ((avg_loss * (period - 1)) + loss) / period;
        out[i] = (avg_loss == 0.0) ? 100.0 : 100.0 - (100.0 / (1.0 + avg_gain / avg_loss));
    }
}

void calculate_atr(const double *high, const double *low, const double *close,
                   size_t count, int period, double *out)
{
    size_t i;
    double sum = 0.0;

    if (high == NULL || low == NULL || close == NULL || out == NULL || period <= 0) {
        return;
    }

    for (i = 0; i < count; i++) {
        double tr;

        out[i] = NAN;
        if (i == 0) {
            tr = high[i] - low[i];
        } else {
            double high_low = high[i] - low[i];
            double high_prev_close = fabs(high[i] - close[i - 1]);
            double low_prev_close = fabs(low[i] - close[i - 1]);
            tr = fmax(high_low, fmax(high_prev_close, low_prev_close));
        }

        sum += tr;
        if (i >= (size_t)period) {
            double old_tr;
            size_t old = i - (size_t)period;

            if (old == 0) {
                old_tr = high[old] - low[old];
            } else {
                double high_low = high[old] - low[old];
                double high_prev_close = fabs(high[old] - close[old - 1]);
                double low_prev_close = fabs(low[old] - close[old - 1]);
                old_tr = fmax(high_low, fmax(high_prev_close, low_prev_close));
            }
            sum -= old_tr;
        }

        if (i + 1 >= (size_t)period) {
            out[i] = sum / period;
        }
    }
}

void calculate_donchian_upper(const double *high, size_t count, int period,
                              int exclude_current, double *out)
{
    size_t i;

    if (high == NULL || out == NULL || period <= 0) {
        return;
    }

    for (i = 0; i < count; i++) {
        size_t end;
        size_t start;
        size_t j;
        double max_value;

        out[i] = NAN;
        if (exclude_current) {
            if (i < (size_t)period) {
                continue;
            }
            end = i - 1;
        } else {
            if (i + 1 < (size_t)period) {
                continue;
            }
            end = i;
        }
        start = end + 1 - (size_t)period;
        max_value = high[start];
        for (j = start + 1; j <= end; j++) {
            if (high[j] > max_value) {
                max_value = high[j];
            }
        }
        out[i] = max_value;
    }
}

void calculate_donchian_lower(const double *low, size_t count, int period,
                              int exclude_current, double *out)
{
    size_t i;

    if (low == NULL || out == NULL || period <= 0) {
        return;
    }

    for (i = 0; i < count; i++) {
        size_t end;
        size_t start;
        size_t j;
        double min_value;

        out[i] = NAN;
        if (exclude_current) {
            if (i < (size_t)period) {
                continue;
            }
            end = i - 1;
        } else {
            if (i + 1 < (size_t)period) {
                continue;
            }
            end = i;
        }
        start = end + 1 - (size_t)period;
        min_value = low[start];
        for (j = start + 1; j <= end; j++) {
            if (low[j] < min_value) {
                min_value = low[j];
            }
        }
        out[i] = min_value;
    }
}
