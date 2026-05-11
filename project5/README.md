# C Rolling Backtest

This project runs rolling-window backtests on `kospi.csv` or `sp500.csv`.

Most strategies are long/short enabled:

- A buy signal is interpreted as a long signal.
- A sell signal is interpreted as a short signal.
- Strategy position can be `1` long, `0` cash, or `-1` short.
- Strategy 6 is the exception: it is long-only and never sells or shorts.
- DCA remains a long-only monthly investing benchmark.

## Build

```bash
gcc main.c data_loader.c indicators.c signals.c backtest.c -o backtest -lm
```

## Run

```bash
./backtest
```

The program asks for:

1. Data file: `1 = kospi.csv`, `2 = sp500.csv`
2. Rolling period: `1 = 10 years`, `2 = 20 years`
3. Capital mode
4. Strategy: `1` through `8`
5. Strategy parameters

## Capital Modes

The strategy list and signal rules are unchanged. Capital mode controls how cash enters each rolling-window backtest.

| Mode | Name | Benchmark | Strategy funding |
| --- | --- | --- | --- |
| 1 | Monthly contribution - invest while holding | Monthly DCA | 500,000 is contributed monthly. Long positions reinvest immediately, short positions store it as `additional_cash`, and cash state accumulates it as `cash`. |
| 2 | Monthly contribution - accumulate cash until signal | Monthly DCA | 500,000 is contributed monthly. Long positions keep new contributions as `cash`, short positions keep them as `additional_cash`, and accumulated cash is used at the next entry or transition. |
| 3 | Lump sum - initial 60,000,000 only | Lump-sum buy-and-hold | Each rolling window starts with `cash = 60,000,000`. No monthly contribution is made. Strategy signals manage only that initial capital. |

Mode 3 is not monthly DCA. The benchmark buys 60,000,000 at the rolling window start Close and holds to the window end. In the CSV, the existing `dca_*` column names are kept for compatibility, but they represent the buy-and-hold benchmark in mode 3.

## Long/Short Accounting

In monthly modes, each rolling window starts with `cash = 0` and no position. A contribution of 500,000 is made on the last available trading day of each month.

In lump-sum mode, each rolling window starts with `cash = 60,000,000`, `total_invested = 60,000,000`, and monthly contribution logic is disabled.

Common transition rules for strategies 1-5:

| Current position | Signal | Action |
| --- | --- | --- |
| Cash | Long | Invest all cash into long |
| Cash | Short | Enter 1x short using all cash as notional |
| Long | Short | Close long, then enter short with full equity |
| Short | Long | Close short, then enter long with full equity |
| Long | Long | No duplicate entry |
| Short | Short | No duplicate entry |

Long valuation:

```text
equity = cash + long_quantity * current_close
```

Short valuation uses a simplified 1x inverse-return model:

```text
short_profit = short_notional * (short_entry_price - current_close) / short_entry_price
equity = short_notional + short_profit + additional_cash
```

While short, monthly contributions are stored as `additional_cash`; no additional short is opened. If short equity becomes zero or negative, the strategy value for that rolling window is set to zero and trading stops for that strategy window.

While long, monthly contributions are normally invested immediately in the same long direction. Strategy 6 has a special accumulation rule: while long, new cash is invested only when `Close < average_buy_price`; otherwise it is held as cash.

Strategies 7 and 8 are long/short, but their exit rules go to cash. They do not immediately reverse into the opposite position on an exit. A new 20/200 EMA entry signal or Turtle entry breakout must occur while cash is held.

Taxes, fees, slippage, borrow costs, short interest, dividends, and FX are excluded.

## Strategies

| No. | Strategy | Position mode | Long entry | Short entry / exit |
| --- | --- | --- | --- | --- |
| 1 | SMA crossover | Long/Short | Short SMA crosses above long SMA | Short SMA crosses below long SMA enters/reverses short |
| 2 | EMA crossover | Long/Short | Short EMA crosses above long EMA | Short EMA crosses below long EMA enters/reverses short |
| 3 | MACD crossover | Long/Short | MACD line crosses above signal line | MACD line crosses below signal line enters/reverses short |
| 4 | RSI strategy | Long/Short | RSI crosses up through oversold level | RSI crosses down through overbought level enters/reverses short |
| 5 | RSI + MACD combined | Long/Short | RSI long and MACD long both occur within `buy_lookback_window` | RSI short and MACD short combined by `sell_combination_mode` within `sell_lookback_window` |
| 6 | Drawdown accumulation | Long-only | Close falls at least `drawdown_percent` from reference high | No short, no sell |
| 7 | 20/200 EMA entry + 50 EMA exit | Long/Short, exits to cash | Cash only: 20 EMA crosses above 200 EMA | Cash only: 20 EMA crosses below 200 EMA; exits use Close/50 EMA |
| 8 | Turtle trading | Long/Short, exits to cash | Cash only: Close is above prior entry upper channel | Cash only: Close is below prior entry lower channel; exits use prior exit channels |

### Strategy 6 Parameters

- `drawdown_percent`, default 30
- `peak_lookback_days`, default 0

If `peak_lookback_days` is `0`, the reference high is cumulative from the start of the rolling window. Otherwise it uses the recent lookback days.

Strategy 6 buys accumulated cash after a drawdown. After entry, it adds cash only when `Close < average_buy_price`; otherwise monthly contributions remain as cash. It never sells and never shorts.

### Strategy 7 Parameters

- `entry_fast_ema_period`, default 20
- `entry_slow_ema_period`, default 200
- `exit_ema_period`, default 50

The 20/200 EMA crossover is the only entry rule. The 50 EMA is exit-only:

- Long exits to cash when Close crosses down through the 50 EMA.
- Short exits to cash when Close crosses up through the 50 EMA.
- Exits do not immediately open the opposite position.

### Strategy 8 Parameters

- `entry_breakout_days`, default 20
- `exit_breakout_days`, default 10
- `atr_period`, default 20
- `use_pyramiding`, accepted but internally treated as 0
- `max_units`, default 4
- `add_unit_atr_multiple`, default 0.5

Donchian channels exclude the current day:

- Entry upper: highest High over the prior `entry_breakout_days`
- Entry lower: lowest Low over the prior `entry_breakout_days`
- Exit upper: highest High over the prior `exit_breakout_days`
- Exit lower: lowest Low over the prior `exit_breakout_days`

Long exits to cash when Close is below the exit lower channel. Short exits to cash when Close is above the exit upper channel. Exits do not immediately open the opposite position.

ATR is calculated as a simple moving average of True Range. Pyramiding input is accepted for compatibility, but this implementation prioritizes `use_pyramiding = 0` correctness and disables pyramiding internally.

## Output

`backtest_result.csv` keeps the existing format:

```csv
start_month,end_month,total_invested,dca_final_value,dca_return,strategy_final_value,strategy_return,excess_return
```

`total_invested` is the actual number of monthly contributions multiplied by 500,000.

In capital mode 3, `total_invested` is always 60,000,000 for every rolling window.

`strategy_info.txt` stores:

- Data file
- Backtest period
- Capital mode
- Initial capital or monthly contribution
- Benchmark type
- Strategy name
- Position mode
- Parameters
- `Long/Short enabled: yes` or `no`

## Plot

```bash
python plot_result.py
```

The output file depends on the selected rolling period:

- `rolling_10yr_monthly_mode1_return.png`
- `rolling_10yr_monthly_mode2_return.png`
- `rolling_10yr_lumpsum_mode3_return.png`
- `rolling_20yr_monthly_mode1_return.png`
- `rolling_20yr_monthly_mode2_return.png`
- `rolling_20yr_lumpsum_mode3_return.png`

The chart title includes the selected capital mode. In mode 3, the benchmark legend is shown as Buy & Hold benchmark.
