#ifndef SIGNALS_H
#define SIGNALS_H

#include <stddef.h>

#include "data_loader.h"

typedef enum {
    STRATEGY_SMA = 1,
    STRATEGY_EMA = 2,
    STRATEGY_MACD = 3,
    STRATEGY_RSI = 4,
    STRATEGY_RSI_MACD = 5,
    STRATEGY_DRAWDOWN_REBOUND = 6,
    STRATEGY_EMA_TREND = 7,
    STRATEGY_TURTLE = 8
} StrategyType;

typedef struct {
    StrategyType type;
    int short_period;
    int long_period;
    int macd_fast_period;
    int macd_slow_period;
    int macd_signal_period;
    int rsi_period;
    double rsi_oversold_level;
    double rsi_overbought_level;
    int buy_lookback_window;
    int sell_lookback_window;
    int sell_combination_mode;
    double drawdown_percent;
    int peak_lookback_days;
    int entry_fast_ema_period;
    int entry_slow_ema_period;
    int exit_ema_period;
    int entry_breakout_days;
    int exit_breakout_days;
    int atr_period;
    int use_pyramiding;
    int max_units;
    double add_unit_atr_multiple;
} StrategyParams;

int signal_within_window(const int *signal_array, int current_index, int lookback_window);
int generate_strategy_signals(const PriceData *data, size_t count, const StrategyParams *params,
                              int *buy_signal, int *sell_signal);
const char *strategy_name(StrategyType type);

#endif
