#include "backtest.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "indicators.h"

static int add_months_year(int year, int month, int add_months, int *out_year, int *out_month)
{
    int zero_based = (month - 1) + add_months;

    if (month < 1 || month > 12 || out_year == NULL || out_month == NULL) {
        return 0;
    }

    *out_year = year + zero_based / 12;
    *out_month = zero_based % 12 + 1;
    return 1;
}

static int same_month(const PriceData *row, int year, int month)
{
    return row->year == year && row->month == month;
}

static int find_first_index_of_month(const PriceData *data, size_t count, int year, int month)
{
    size_t i;

    for (i = 0; i < count; i++) {
        if (same_month(&data[i], year, month)) {
            return (int)i;
        }
    }

    return -1;
}

static int find_last_index_of_month(const PriceData *data, size_t count, int year, int month)
{
    size_t i;
    int found = -1;

    for (i = 0; i < count; i++) {
        if (same_month(&data[i], year, month)) {
            found = (int)i;
        } else if (found >= 0) {
            break;
        }
    }

    return found;
}

static int is_month_end_index(const PriceData *data, size_t count, int index, int end_index)
{
    if (index < 0 || (size_t)index >= count) {
        return 0;
    }

    if (index == end_index) {
        return 1;
    }

    if ((size_t)index + 1 >= count) {
        return 1;
    }

    return data[index].year != data[index + 1].year || data[index].month != data[index + 1].month;
}

static void format_month(char *buffer, size_t size, int year, int month)
{
    snprintf(buffer, size, "%04d-%02d", year, month);
}

const char *capital_mode_name(CapitalMode mode)
{
    switch (mode) {
    case CAPITAL_MONTHLY_INVEST_WHILE_HOLDING:
        return "Monthly contribution - invest while holding";
    case CAPITAL_MONTHLY_ACCUMULATE_UNTIL_SIGNAL:
        return "Monthly contribution - accumulate cash until signal";
    case CAPITAL_LUMP_SUM:
        return "Lump sum - initial 60,000,000 only";
    default:
        return "Unknown capital mode";
    }
}

const char *benchmark_type_name(CapitalMode mode)
{
    return (mode == CAPITAL_LUMP_SUM) ? "Lump-sum buy-and-hold" : "Monthly DCA";
}

static double short_equity(double short_notional, double short_entry_price,
                           double current_close, double additional_cash)
{
    double profit;

    if (short_notional <= 0.0 || short_entry_price <= 0.0 || current_close <= 0.0) {
        return additional_cash;
    }

    profit = short_notional * (short_entry_price - current_close) / short_entry_price;
    return short_notional + profit + additional_cash;
}

static void zero_strategy_state(double *cash, double *long_quantity,
                                double *average_buy_price, double *short_entry_price,
                                double *short_notional, double *additional_cash,
                                int *position)
{
    *cash = 0.0;
    *long_quantity = 0.0;
    *average_buy_price = 0.0;
    *short_entry_price = 0.0;
    *short_notional = 0.0;
    *additional_cash = 0.0;
    *position = 0;
}

static void buy_long_with_cash(double *cash, double *long_quantity,
                               double *average_buy_price, double close)
{
    double total_cost;
    double added_quantity;

    if (*cash <= 0.0 || close <= 0.0) {
        return;
    }

    total_cost = (*average_buy_price * *long_quantity) + *cash;
    added_quantity = *cash / close;
    *long_quantity += added_quantity;
    *average_buy_price = (*long_quantity > 0.0) ? total_cost / *long_quantity : 0.0;
    *cash = 0.0;
}

static void close_short_to_cash(double *cash, double *short_entry_price,
                                double *short_notional, double *additional_cash,
                                double close)
{
    *cash = short_equity(*short_notional, *short_entry_price, close, *additional_cash);
    *short_entry_price = 0.0;
    *short_notional = 0.0;
    *additional_cash = 0.0;
}

static double find_reference_high(const PriceData *data, int start_index, int current_index,
                                  int lookback_days)
{
    int i;
    int first = start_index;
    double high;

    if (lookback_days > 0 && current_index - lookback_days + 1 > first) {
        first = current_index - lookback_days + 1;
    }

    high = data[first].close;
    for (i = first + 1; i <= current_index; i++) {
        if (data[i].close > high) {
            high = data[i].close;
        }
    }
    return high;
}

static void drawdown_signal_for_day(const PriceData *data, int start_index,
                                    int current_index, const StrategyParams *params,
                                    int *long_today)
{
    double close = data[current_index].close;
    double high = find_reference_high(data, start_index, current_index,
                                      params->peak_lookback_days);
    double drawdown = (high > 0.0) ? (high - close) / high * 100.0 : 0.0;

    *long_today = drawdown >= params->drawdown_percent;
}

static BacktestResult run_single_window(const PriceData *data, int start_index, int end_index,
                                        const StrategyParams *params,
                                        const int *long_signal, const int *short_signal,
                                        int window_years, CapitalMode capital_mode)
{
    int i;
    double dca_shares = 0.0;
    double cash = 0.0;
    double long_quantity = 0.0;
    double average_buy_price = 0.0;
    double short_entry_price = 0.0;
    double short_notional = 0.0;
    double additional_cash = 0.0;
    double total_invested = 0.0;
    double *exit_ema = NULL;
    double *turtle_exit_upper = NULL;
    double *turtle_exit_lower = NULL;
    int position = 0;
    int strategy_dead = 0;
    BacktestResult result;
    int window_size = end_index - start_index + 1;

    memset(&result, 0, sizeof(result));
    if (window_years <= 0 || data[start_index].close <= 0.0 ||
        data[end_index].close <= 0.0) {
        return result;
    }

    if (capital_mode == CAPITAL_LUMP_SUM) {
        cash = LUMP_SUM_INITIAL_CAPITAL;
        total_invested = LUMP_SUM_INITIAL_CAPITAL;
        dca_shares = LUMP_SUM_INITIAL_CAPITAL / data[start_index].close;
    }

    format_month(result.start_month, sizeof(result.start_month),
                 data[start_index].year, data[start_index].month);
    format_month(result.end_month, sizeof(result.end_month),
                 data[end_index].year, data[end_index].month);

    if (params->type == STRATEGY_EMA_TREND) {
        double *window_close = (double *)malloc((size_t)window_size * sizeof(double));

        exit_ema = (double *)malloc((size_t)window_size * sizeof(double));
        if (window_close == NULL || exit_ema == NULL) {
            free(window_close);
            free(exit_ema);
            return result;
        }
        for (i = 0; i < window_size; i++) {
            window_close[i] = data[start_index + i].close;
        }
        calculate_ema(window_close, (size_t)window_size, params->exit_ema_period, exit_ema);
        free(window_close);
    } else if (params->type == STRATEGY_TURTLE) {
        double *window_high = (double *)malloc((size_t)window_size * sizeof(double));
        double *window_low = (double *)malloc((size_t)window_size * sizeof(double));

        turtle_exit_upper = (double *)malloc((size_t)window_size * sizeof(double));
        turtle_exit_lower = (double *)malloc((size_t)window_size * sizeof(double));
        if (window_high == NULL || window_low == NULL ||
            turtle_exit_upper == NULL || turtle_exit_lower == NULL) {
            free(window_high);
            free(window_low);
            free(turtle_exit_upper);
            free(turtle_exit_lower);
            return result;
        }
        for (i = 0; i < window_size; i++) {
            window_high[i] = data[start_index + i].high;
            window_low[i] = data[start_index + i].low;
        }
        calculate_donchian_upper(window_high, (size_t)window_size,
                                 params->exit_breakout_days, 1, turtle_exit_upper);
        calculate_donchian_lower(window_low, (size_t)window_size,
                                 params->exit_breakout_days, 1, turtle_exit_lower);
        free(window_high);
        free(window_low);
    }

    for (i = start_index; i <= end_index; i++) {
        double close = data[i].close;
        int local_index = i - start_index;
        int long_today = long_signal[i] == 1;
        int short_today = short_signal[i] == 1;
        int long_exit_today = 0;
        int short_exit_today = 0;

        if (close <= 0.0) {
            continue;
        }

        if (params->type == STRATEGY_DRAWDOWN_REBOUND) {
            drawdown_signal_for_day(data, start_index, i, params, &long_today);
            short_today = 0;
        } else if (params->type == STRATEGY_EMA_TREND && local_index > 0 &&
                   !isnan(exit_ema[local_index - 1]) && !isnan(exit_ema[local_index])) {
            double previous_close = data[i - 1].close;

            long_exit_today = previous_close >= exit_ema[local_index - 1] &&
                              close < exit_ema[local_index];
            short_exit_today = previous_close <= exit_ema[local_index - 1] &&
                               close > exit_ema[local_index];
        } else if (params->type == STRATEGY_TURTLE && local_index > 0) {
            long_exit_today = !isnan(turtle_exit_lower[local_index]) &&
                              close < turtle_exit_lower[local_index];
            short_exit_today = !isnan(turtle_exit_upper[local_index]) &&
                               close > turtle_exit_upper[local_index];
        }

        if (params->type != STRATEGY_DRAWDOWN_REBOUND && !strategy_dead && position == -1 &&
            short_equity(short_notional, short_entry_price, close, additional_cash) <= 0.0) {
            zero_strategy_state(&cash, &long_quantity, &average_buy_price,
                                &short_entry_price, &short_notional,
                                &additional_cash, &position);
            strategy_dead = 1;
        }

        /* Contributions happen on the last available trading day of each month. */
        if (capital_mode != CAPITAL_LUMP_SUM &&
            is_month_end_index(data, (size_t)end_index + 1, i, end_index)) {
            total_invested += MONTHLY_INVESTMENT;
            dca_shares += MONTHLY_INVESTMENT / close;

            if (!strategy_dead) {
                if (position == 1) {
                    if (capital_mode == CAPITAL_MONTHLY_ACCUMULATE_UNTIL_SIGNAL) {
                        cash += MONTHLY_INVESTMENT;
                    } else if (params->type != STRATEGY_DRAWDOWN_REBOUND ||
                        close < average_buy_price) {
                        double invest = cash + MONTHLY_INVESTMENT;
                        double old_cost = average_buy_price * long_quantity;
                        long_quantity += invest / close;
                        average_buy_price = (long_quantity > 0.0) ?
                            (old_cost + invest) / long_quantity : 0.0;
                        cash = 0.0;
                    } else {
                        cash += MONTHLY_INVESTMENT;
                    }
                } else if (position == -1) {
                    additional_cash += MONTHLY_INVESTMENT;
                } else {
                    cash += MONTHLY_INVESTMENT;
                }
            }
        }

        if (strategy_dead) {
            continue;
        }

        if (params->type == STRATEGY_DRAWDOWN_REBOUND) {
            if (position == 0) {
                if (long_today && cash > 0.0) {
                    buy_long_with_cash(&cash, &long_quantity, &average_buy_price, close);
                    position = 1;
                    result.long_entry_count++;
                }
            } else if (position == 1 && cash > 0.0 && close < average_buy_price) {
                buy_long_with_cash(&cash, &long_quantity, &average_buy_price, close);
            }
            continue;
        }

        if (params->type == STRATEGY_EMA_TREND || params->type == STRATEGY_TURTLE) {
            if (position == 1 && long_exit_today) {
                cash += long_quantity * close;
                long_quantity = 0.0;
                average_buy_price = 0.0;
                position = 0;
                result.long_exit_count++;
                continue;
            }
            if (position == -1 && short_exit_today) {
                close_short_to_cash(&cash, &short_entry_price, &short_notional,
                                    &additional_cash, close);
                if (cash <= 0.0) {
                    zero_strategy_state(&cash, &long_quantity, &average_buy_price,
                                        &short_entry_price, &short_notional,
                                        &additional_cash, &position);
                    strategy_dead = 1;
                    continue;
                }
                position = 0;
                result.short_exit_count++;
                continue;
            }
        }

        if (position == 0) {
            if (long_today && cash > 0.0) {
                buy_long_with_cash(&cash, &long_quantity, &average_buy_price, close);
                position = 1;
                result.long_entry_count++;
            } else if (short_today && cash > 0.0 && close > 0.0) {
                short_notional = cash;
                short_entry_price = close;
                additional_cash = 0.0;
                cash = 0.0;
                position = -1;
                result.short_entry_count++;
            }
        } else if (position == 1) {
            if (short_today && params->type != STRATEGY_EMA_TREND &&
                params->type != STRATEGY_TURTLE) {
                cash += long_quantity * close;
                long_quantity = 0.0;
                average_buy_price = 0.0;
                result.long_exit_count++;
                if (cash > 0.0) {
                    short_notional = cash;
                    short_entry_price = close;
                    additional_cash = 0.0;
                    cash = 0.0;
                    position = -1;
                    result.short_entry_count++;
                } else {
                    position = 0;
                }
            }
        } else if (position == -1) {
            if (long_today && params->type != STRATEGY_EMA_TREND &&
                params->type != STRATEGY_TURTLE) {
                close_short_to_cash(&cash, &short_entry_price, &short_notional,
                                    &additional_cash, close);
                position = 0;
                result.short_exit_count++;
                if (cash <= 0.0) {
                    cash = 0.0;
                    strategy_dead = 1;
                } else {
                    buy_long_with_cash(&cash, &long_quantity, &average_buy_price, close);
                    position = 1;
                    result.long_entry_count++;
                }
            }
        }
    }

    result.total_invested = total_invested;
    result.dca_final_value = dca_shares * data[end_index].close;
    if (strategy_dead) {
        result.strategy_final_value = 0.0;
    } else if (position == 1) {
        result.strategy_final_value = cash + long_quantity * data[end_index].close;
    } else if (position == -1) {
        result.strategy_final_value = short_equity(short_notional, short_entry_price,
                                                   data[end_index].close, additional_cash);
        if (result.strategy_final_value < 0.0) {
            result.strategy_final_value = 0.0;
        }
    } else {
        result.strategy_final_value = cash;
    }

    if (total_invested > 0.0) {
        result.dca_return = (result.dca_final_value - total_invested) / total_invested * 100.0;
        result.strategy_return = (result.strategy_final_value - total_invested) / total_invested * 100.0;
        result.excess_return = result.strategy_return - result.dca_return;
    }

    free(exit_ema);
    free(turtle_exit_upper);
    free(turtle_exit_lower);
    return result;
}

int run_long_short_strategy_backtest(const PriceData *data, size_t count,
                                     const StrategyParams *params,
                                     const int *long_signal,
                                     const int *short_signal,
                                     BacktestResult **out_results,
                                     size_t *out_count, int window_years,
                                     CapitalMode capital_mode)
{
    size_t i;
    size_t capacity = 256;
    size_t result_count = 0;
    BacktestResult *results;

    if (data == NULL || params == NULL || long_signal == NULL || short_signal == NULL ||
        out_results == NULL || out_count == NULL || count == 0 ||
        window_years <= 0 || capital_mode < CAPITAL_MONTHLY_INVEST_WHILE_HOLDING ||
        capital_mode > CAPITAL_LUMP_SUM) {
        return 0;
    }

    *out_results = NULL;
    *out_count = 0;

    results = (BacktestResult *)malloc(capacity * sizeof(BacktestResult));
    if (results == NULL) {
        return 0;
    }

    for (i = 0; i < count; i++) {
        int end_year;
        int end_month;
        int start_index;
        int end_index;

        if (i > 0 && data[i].year == data[i - 1].year && data[i].month == data[i - 1].month) {
            continue;
        }

        if (!add_months_year(data[i].year, data[i].month, window_years * 12,
                             &end_year, &end_month)) {
            continue;
        }

        start_index = find_first_index_of_month(data, count, data[i].year, data[i].month);
        end_index = find_last_index_of_month(data, count, end_year, end_month);
        if (start_index < 0 || end_index < 0 || end_index <= start_index) {
            continue;
        }

        if (result_count == capacity) {
            BacktestResult *grown;
            capacity *= 2;
            grown = (BacktestResult *)realloc(results, capacity * sizeof(BacktestResult));
            if (grown == NULL) {
                free(results);
                return 0;
            }
            results = grown;
        }

        results[result_count++] = run_single_window(data, start_index, end_index,
                                                    params, long_signal, short_signal,
                                                    window_years, capital_mode);
    }

    if (result_count == 0) {
        free(results);
        return 0;
    }

    *out_results = results;
    *out_count = result_count;
    return 1;
}

int save_backtest_results(const char *filename, const BacktestResult *results, size_t count)
{
    size_t i;
    FILE *fp;

    if (filename == NULL || results == NULL || count == 0) {
        return 0;
    }

    fp = fopen(filename, "w");
    if (fp == NULL) {
        fprintf(stderr, "Failed to write result CSV: %s\n", filename);
        return 0;
    }

    fprintf(fp, "start_month,end_month,total_invested,dca_final_value,dca_return,"
                "strategy_final_value,strategy_return,excess_return\n");

    for (i = 0; i < count; i++) {
        fprintf(fp, "%s,%s,%.2f,%.2f,%.6f,%.2f,%.6f,%.6f\n",
                results[i].start_month,
                results[i].end_month,
                results[i].total_invested,
                results[i].dca_final_value,
                results[i].dca_return,
                results[i].strategy_final_value,
                results[i].strategy_return,
                results[i].excess_return);
    }

    fclose(fp);
    return 1;
}

BacktestSummary summarize_results(const BacktestResult *results, size_t count)
{
    BacktestSummary summary = {0};
    size_t i;
    size_t wins = 0;

    if (results == NULL || count == 0) {
        return summary;
    }

    summary.window_count = count;
    summary.max_dca_return = -DBL_MAX;
    summary.min_dca_return = DBL_MAX;
    summary.max_strategy_return = -DBL_MAX;
    summary.min_strategy_return = DBL_MAX;

    for (i = 0; i < count; i++) {
        summary.avg_dca_return += results[i].dca_return;
        summary.avg_strategy_return += results[i].strategy_return;
        summary.avg_excess_return += results[i].excess_return;
        summary.long_entry_count += results[i].long_entry_count;
        summary.long_exit_count += results[i].long_exit_count;
        summary.short_entry_count += results[i].short_entry_count;
        summary.short_exit_count += results[i].short_exit_count;

        if (results[i].strategy_return > results[i].dca_return) {
            wins++;
        }
        if (results[i].dca_return > summary.max_dca_return) {
            summary.max_dca_return = results[i].dca_return;
        }
        if (results[i].dca_return < summary.min_dca_return) {
            summary.min_dca_return = results[i].dca_return;
        }
        if (results[i].strategy_return > summary.max_strategy_return) {
            summary.max_strategy_return = results[i].strategy_return;
        }
        if (results[i].strategy_return < summary.min_strategy_return) {
            summary.min_strategy_return = results[i].strategy_return;
        }
    }

    summary.avg_dca_return /= count;
    summary.avg_strategy_return /= count;
    summary.avg_excess_return /= count;
    summary.win_rate = (double)wins / count * 100.0;

    return summary;
}
