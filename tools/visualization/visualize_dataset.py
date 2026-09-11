"""VEGA Sentinel - dataset visualization.

Plots a collection session: temperature, humidity, MQ-2 raw ADC, flame
state, and (when present) the label/phase timeline. Useful for visually
inspecting data quality before training.

Usage (from repository root):
    python tools/visualization/visualize_dataset.py --file dataset/raw/sentinel_data_20260911_182814.csv
    python tools/visualization/visualize_dataset.py --file <csv> --no-show --output results/graphs/
"""
import argparse
import os
import sys

import matplotlib

import pandas as pd

COLUMNS = ["temp_c", "humidity_pct", "mq2_adc", "flame_state"]


def main():
    parser = argparse.ArgumentParser(
        description="Plot a VEGA Sentinel collection CSV"
    )
    parser.add_argument(
        "--file",
        required=True,
        help="Raw collection CSV (see dataset/raw/ for the format)",
    )
    parser.add_argument(
        "--output",
        default=None,
        help="Directory for PNG output. If omitted (or --no-save is not "
             "given... see below), figures are only shown.",
    )
    parser.add_argument(
        "--no-show",
        action="store_true",
        help="Do not open an interactive window (for headless use)",
    )
    args = parser.parse_args()

    if not os.path.exists(args.file):
        print(f"ERROR: file not found: {args.file}")
        sys.exit(1)

    # Headless-safe: only use a GUI backend when we intend to show
    if args.no_show:
        matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    df = pd.read_csv(args.file)

    missing = [c for c in COLUMNS if c not in df.columns]
    if missing:
        print(f"ERROR: missing columns {missing} in {args.file}")
        sys.exit(1)

    time_s = None
    if "timestamp_ms" in df.columns:
        t = pd.to_numeric(df["timestamp_ms"], errors="coerce")
        time_s = (t - t.iloc[0]) / 1000.0
    else:
        time_s = df.index * 2.5  # assume 2.5 s cadence

    fig, axes = plt.subplots(4, 1, figsize=(12, 10), sharex=True)
    fig.suptitle(
        f"VEGA Sentinel - {os.path.basename(args.file)}", fontsize=13
    )

    axes[0].plot(time_s, df["temp_c"], color="tab:red")
    axes[0].set_ylabel("Temperature (C)")

    axes[1].plot(time_s, df["humidity_pct"], color="tab:blue")
    axes[1].set_ylabel("Humidity (%)")

    axes[2].plot(time_s, df["mq2_adc"], color="tab:green")
    axes[2].set_ylabel("MQ-2 raw ADC")

    axes[3].step(time_s, df["flame_state"], where="post", color="black")
    axes[3].set_ylabel("Flame (1=detected)")
    axes[3].set_xlabel("Time (s)")

    # Mark labelled abnormal regions if present (shaded spans)
    if "label" in df.columns:
        abnormal_mask = (df["label"] != "normal").values
        start = None
        for i, flag in enumerate(list(abnormal_mask) + [False]):
            if flag and start is None:
                start = i
            elif not flag and start is not None:
                for ax in axes:
                    ax.axvspan(
                        time_s.iloc[start], time_s.iloc[i - 1],
                        color="orange", alpha=0.3,
                    )
                start = None

    fig.tight_layout(rect=(0, 0, 1, 0.96))

    if args.output:
        os.makedirs(args.output, exist_ok=True)
        out_path = os.path.join(
            args.output,
            os.path.splitext(os.path.basename(args.file))[0] + ".png",
        )
        fig.savefig(out_path, dpi=120)
        print(f"Saved: {out_path}")

    if not args.no_show:
        plt.show()
    else:
        plt.close(fig)

    print("Done.")


if __name__ == "__main__":
    main()
