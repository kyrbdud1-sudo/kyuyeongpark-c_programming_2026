#ifndef BACKTEST_H
#define BACKTEST_H

#include <stddef.h>

#include "data_loader.h"
#include "signals.h"

#define MONTHLY_INVESTMENT 500000.0
#define LUMP_SUM_INITIAL_CAPITAL 60000000.0

typedef enum {
    CAPITAL_MONTHLY_INVEST_WHILE_HOLDING = 1,
    CAPITAL_MONTHLY_ACCUMULATE_UNTIL_SIGNAL = 2,
    CAPITAL_LUMP_SUM = 3
} CapitalMode;

typedef struct {
    char start_month[8];
    char end_month[8];
    double total_invested;
    double dca_final_value;
    double dca_return;
    double strategy_final_value;
    double strategy_return;
    double excess_return;
    size_t long_entry_count;
    size_t long_exit_count;
    size_t short_entry_count;
    size_t short_exit_count;
} BacktestResult;

typedef struct {
    size_t window_count;
    double avg_dca_return;
    double avg_strategy_return;
    double win_rate;
    double max_dca_return;
    double min_dca_return;
    double max_strategy_return;
    double min_strategy_return;
    double avg_excess_return;
    size_t long_entry_count;
    size_t long_exit_count;
    size_t short_entry_count;
    size_t short_exit_count;
} BacktestSummary;

int run_long_short_strategy_backtest(const PriceData *data, size_t count,
                                     const StrategyParams *params,
                                     const int *long_signal,
                                     const int *short_signal,
                                     BacktestResult **out_results,
                                     size_t *out_count, int window_years,
                                     CapitalMode capital_mode);
int save_backtest_results(const char *filename, const BacktestResult *results, size_t count);
BacktestSummary summarize_results(const BacktestResult *results, size_t count);
const char *capital_mode_name(CapitalMode mode);
const char *benchmark_type_name(CapitalMode mode);

#endif
