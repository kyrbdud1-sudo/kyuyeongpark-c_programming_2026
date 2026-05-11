"""Plot rolling backtest results from backtest_result.csv."""

import os

import matplotlib.pyplot as plt
import pandas as pd


def read_strategy_info():
    info = {
        "strategy": None,
        "data": None,
        "window_years": 10,
        "long_short_enabled": False,
        "capital_mode": None,
        "capital_mode_id": 1,
        "benchmark_type": "Monthly DCA",
    }

    if not os.path.exists("strategy_info.txt"):
        return info

    with open("strategy_info.txt", "r", encoding="utf-8") as fp:
        lines = [line.strip() for line in fp.readlines()]

    for line in lines:
        if line.startswith("Strategy:"):
            info["strategy"] = line.split(":", 1)[1].strip()
        elif line.startswith("Data:"):
            info["data"] = line.split(":", 1)[1].strip()
        elif line.startswith("Backtest period:"):
            period_text = line.split(":", 1)[1].strip().split()[0]
            info["window_years"] = int(period_text)
        elif line.startswith("Long/Short enabled:"):
            info["long_short_enabled"] = line.split(":", 1)[1].strip().lower() in {"yes", "true", "1"}
        elif line.startswith("Capital mode:"):
            value = line.split(":", 1)[1].strip()
            info["capital_mode"] = value
            if "accumulate cash" in value.lower():
                info["capital_mode_id"] = 2
            elif "lump sum" in value.lower():
                info["capital_mode_id"] = 3
            else:
                info["capital_mode_id"] = 1
        elif line.startswith("Benchmark type:"):
            info["benchmark_type"] = line.split(":", 1)[1].strip()

    return info


def build_strategy_title(info):
    window_years = info["window_years"]
    strategy = info["strategy"]
    data = info["data"]
    base_title = f"Rolling {window_years}-Year Return"

    suffix = " - Long/Short enabled" if info["long_short_enabled"] else ""
    if info["capital_mode"]:
        suffix += f" - {info['capital_mode']}"
    if strategy and data:
        return f"{base_title} - {strategy} ({data}){suffix}"
    if strategy:
        return f"{base_title} - {strategy}{suffix}"
    return f"{base_title}{suffix}"


def output_filename(window_years, capital_mode_id):
    if capital_mode_id == 2:
        return f"rolling_{window_years}yr_monthly_mode2_return.png"
    if capital_mode_id == 3:
        return f"rolling_{window_years}yr_lumpsum_mode3_return.png"
    return f"rolling_{window_years}yr_monthly_mode1_return.png"

def main():
    result_file = "backtest_result.csv"
    info = read_strategy_info()
    window_years = info["window_years"]
    output_file = output_filename(window_years, info["capital_mode_id"])

    if not os.path.exists(result_file):
        raise FileNotFoundError("backtest_result.csv not found. Run the C backtest program first.")

    df = pd.read_csv(result_file)
    required_columns = {
        "start_month",
        "dca_return",
        "strategy_return",
        "excess_return",
    }
    missing = required_columns - set(df.columns)
    if missing:
        raise ValueError(f"Missing columns in result CSV: {sorted(missing)}")

    plt.figure(figsize=(12, 6))
    benchmark_label = "Buy & Hold benchmark" if info["capital_mode_id"] == 3 else "DCA benchmark"
    plt.plot(df["start_month"], df["dca_return"], label=f"{benchmark_label} rolling {window_years}-year return")
    plt.plot(df["start_month"], df["strategy_return"], label=f"Strategy rolling {window_years}-year return")
    plt.plot(df["start_month"], df["excess_return"], label=f"Excess return, Strategy - {benchmark_label}")

    tick_step = max(len(df) // 12, 1)
    plt.xticks(df["start_month"][::tick_step], rotation=45, ha="right")
    plt.xlabel("Start month")
    plt.ylabel("Return (%)")
    plt.title(build_strategy_title(info))
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig(output_file, dpi=150)
    print(f"Saved: {output_file}")


if __name__ == "__main__":
    main()
