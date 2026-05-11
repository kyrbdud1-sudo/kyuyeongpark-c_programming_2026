#ifndef INDICATORS_H
#define INDICATORS_H

#include <stddef.h>

void calculate_sma(const double *close, size_t count, int period, double *out);
void calculate_ema(const double *close, size_t count, int period, double *out);
void calculate_macd(const double *close, size_t count, int fast_period, int slow_period,
                    int signal_period, double *macd_line, double *signal_line);
void calculate_rsi(const double *close, size_t count, int period, double *out);
void calculate_atr(const double *high, const double *low, const double *close,
                   size_t count, int period, double *out);
void calculate_donchian_upper(const double *high, size_t count, int period,
                              int exclude_current, double *out);
void calculate_donchian_lower(const double *low, size_t count, int period,
                              int exclude_current, double *out);

#endif
