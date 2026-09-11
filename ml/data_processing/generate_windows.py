"""VEGA Sentinel - sliding-window dataset generation.

Reads the raw collection CSVs from dataset/raw/ (or --raw-dir), cleans them
the same way the firmware treats samples (DHT-valid rows only, MQ-2 within
the ADS1015 single-ended range), and extracts window features using the
SAME window logic as deployment (window size 10, one-sample step for
sliding windows; use --stride to change).

This is the cleaned-up, path-configurable version of the v3 preprocessing
used to produce dataset/processed/sentinel_window_features_v3.csv.

Usage (from repository root):
    python ml/data_processing/generate_windows.py
    python ml/data_processing/generate_windows.py --stride 10 --output dataset/processed/custom.csv
"""
import argparse
import os
import sys

import numpy as np
import pandas as pd

# Window parameters - must match firmware/sentinel_final
WINDOW_SIZE = 10

# Raw column format produced by firmware + PC logger
REQUIRED_COLUMNS = [
    "timestamp_ms", "temp_c", "humidity_pct", "mq2_adc",
    "flame_state", "dht_ok", "label",
]


def calculate_slope(values, times):
    """Least-squares slope of values against relative time (per second)."""
    if len(values) < 2:
        return 0.0

    x = np.asarray(times, dtype=float)
    y = np.asarray(values, dtype=float)
    x = x - x[0]

    if np.ptp(x) == 0:
        return 0.0

    return float(np.polyfit(x, y, 1)[0])


def make_features(window, source_file, window_number):
    """Compute all window features (superset of the 7 model features).

    The 7 MODEL A features are: humidity_mean, temp_mean, mq2_range,
    mq2_delta, mq2_std, humidity_std, flame_fraction. Additional columns
    are kept for analysis but are not used by the model.
    """
    temp = window["temp_c"].values
    humidity = window["humidity_pct"].values
    mq2 = window["mq2_adc"].values
    flame = window["flame_state"].values
    times = window["timestamp_ms"].values / 1000.0

    critical_count = int((window["label"] == "critical").sum())

    # Window is abnormal if it contains any 'critical' event label.
    anomaly_label = 1 if critical_count > 0 else 0

    return {
        "source_file": source_file,
        "window_number": window_number,

        # Model features (order matters - see ml/training/train_model.py)
        "humidity_mean": np.mean(humidity),
        "temp_mean": np.mean(temp),
        "mq2_range": np.max(mq2) - np.min(mq2),
        "mq2_delta": mq2[-1] - mq2[0],
        "mq2_std": np.std(mq2, ddof=1),
        "humidity_std": np.std(humidity, ddof=1),
        "flame_fraction": np.mean(flame),

        # Additional analysis-only features (not used by Model A)
        "mq2_mean": np.mean(mq2),
        "mq2_min": np.min(mq2),
        "mq2_max": np.max(mq2),
        "mq2_slope_per_s": calculate_slope(mq2, times),
        "temp_std": np.std(temp, ddof=1),
        "temp_delta": temp[-1] - temp[0],
        "temp_slope_per_s": calculate_slope(temp, times),
        "humidity_delta": humidity[-1] - humidity[0],
        "humidity_slope_per_s": calculate_slope(humidity, times),
        "flame_count": np.sum(flame),
        "mq2_last": mq2[-1],
        "temp_last": temp[-1],
        "humidity_last": humidity[-1],
        "flame_last": flame[-1],

        # Labels and metadata
        "critical_count": critical_count,
        "anomaly_label": anomaly_label,
        "label": "critical" if anomaly_label else "normal",
        "phase": (
            window["phase"].iloc[-1]
            if "phase" in window.columns
            else "unknown"
        ),
        "window_duration_s": times[-1] - times[0],
    }


def clean_dataframe(df):
    """Apply the same validity rules the firmware uses for samples."""
    df = df.copy()

    numeric_columns = [
        "timestamp_ms", "temp_c", "humidity_pct",
        "mq2_adc", "flame_state", "dht_ok",
    ]
    for col in numeric_columns:
        df[col] = pd.to_numeric(df[col], errors="coerce")

    df = df.dropna(subset=numeric_columns)

    # DHT must be valid (matches firmware: invalid samples are discarded)
    df = df[df["dht_ok"] == 1]

    # MQ-2 raw ADC range on this core (ADS1015 single-ended)
    df = df[(df["mq2_adc"] >= 0) & (df["mq2_adc"] <= 2047)]

    return df


def main():
    parser = argparse.ArgumentParser(
        description="Generate sliding-window feature dataset from raw CSVs"
    )
    parser.add_argument(
        "--raw-dir",
        default=None,
        help="Directory containing sentinel_data_*.csv "
             "(default: <repo>/dataset/raw)",
    )
    parser.add_argument(
        "--files",
        nargs="*",
        default=None,
        help="Specific CSV filenames inside --raw-dir "
             "(default: the 5 meaningful files listed in dataset/README.md)",
    )
    parser.add_argument(
        "--stride",
        type=int,
        default=None,
        help="Window step in samples. Default: WINDOW_SIZE (non-overlapping "
             "windows, matching the v3 development dataset). Use 1 for the "
             "deployment-style sliding window.",
    )
    parser.add_argument(
        "--output",
        default=None,
        help="Output CSV path (default: <repo>/results/"
             "sentinel_window_features_generated.csv)",
    )
    args = parser.parse_args()

    # Resolve repository root (script lives in <repo>/ml/data_processing)
    repo_root = os.path.abspath(
        os.path.join(os.path.dirname(__file__), "..", "..")
    )

    raw_dir = args.raw_dir or os.path.join(repo_root, "dataset", "raw")
    stride = args.stride or WINDOW_SIZE
    output = args.output or os.path.join(
        repo_root, "results", "sentinel_window_features_generated.csv"
    )

    # Meaningful collection sessions (see dataset/README.md - excludes
    # short logger tests and artificial-label files)
    default_files = [
        "sentinel_data_20260911_153823.csv",
        "sentinel_data_20260911_165323.csv",
        "sentinel_data_20260911_182814.csv",
        "sentinel_data_20260911_190122.csv",
        "sentinel_data_20260911_190322.csv",
    ]
    files = args.files or default_files

    print("=" * 60)
    print("VEGA SENTINEL - WINDOW DATASET GENERATION")
    print(f"window_size={WINDOW_SIZE} stride={stride}")
    print("=" * 60)

    all_windows = []

    for filename in files:
        filepath = os.path.join(raw_dir, filename)
        if not os.path.exists(filepath):
            print(f"WARNING: file not found: {filepath}")
            continue

        df = pd.read_csv(filepath)

        missing = [c for c in REQUIRED_COLUMNS if c not in df.columns]
        if missing:
            print(f"WARNING: {filename} missing columns {missing} - skipped")
            continue

        df = clean_dataframe(df)
        print(f"{filename}: clean rows = {len(df)}")

        window_number = 0
        for start in range(0, len(df) - WINDOW_SIZE + 1, stride):
            window = df.iloc[start:start + WINDOW_SIZE]
            all_windows.append(
                make_features(window, filename, window_number)
            )
            window_number += 1

        print(f"  -> windows created: {window_number}")

    if not all_windows:
        print("ERROR: no windows were created.")
        sys.exit(1)

    dataset = pd.DataFrame(all_windows)

    os.makedirs(os.path.dirname(output), exist_ok=True)
    dataset.to_csv(output, index=False)

    print()
    print("=" * 60)
    print("DATASET CREATED")
    print("=" * 60)
    print(f"Output        : {output}")
    print(f"Total windows : {len(dataset)}")
    print(f"Normal        : {int((dataset['anomaly_label'] == 0).sum())}")
    print(f"Abnormal      : {int((dataset['anomaly_label'] == 1).sum())}")
    print()
    print("Windows by source file:")
    print(
        pd.crosstab(
            dataset["source_file"], dataset["anomaly_label"]
        ).to_string()
    )
    print()
    print("NOTE: this is a development dataset. Not sufficient for")
    print("real-world accuracy claims. See docs/dataset-methodology.md.")


if __name__ == "__main__":
    main()
