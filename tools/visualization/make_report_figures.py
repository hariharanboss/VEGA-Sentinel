"""VEGA Sentinel - regenerate report figures as vector PDFs.

Reproduces the two analysis figures used in the project report directly
from the committed raw datasets, saving them as vector PDF (plus a
fallback PNG). Vector PDF is embedded natively by pdfLaTeX, avoiding
libpng issues seen with matplotlib RGBA PNGs.

Run from the repository root:
    python tools/visualization/make_report_figures.py
"""
import os

import matplotlib

matplotlib.use("Agg")  # headless
import matplotlib.pyplot as plt
import pandas as pd

REPO = os.path.abspath(
    os.path.join(os.path.dirname(__file__), "..", "..")
)
RAW = os.path.join(REPO, "dataset", "raw")
OUT = os.path.join(REPO, "docs", "report")

SESSIONS = [
    "sentinel_data_20260911_153823.csv",
    "sentinel_data_20260911_165323.csv",
    "sentinel_data_20260911_182814.csv",
    "sentinel_data_20260911_190122.csv",
    "sentinel_data_20260911_190322.csv",
]


def load(name):
    df = pd.read_csv(os.path.join(RAW, name))
    t = pd.to_numeric(df["timestamp_ms"], errors="coerce")
    df["time_s"] = (t - t.iloc[0]) / 1000.0
    return df


def save(fig, name):
    pdf = os.path.join(OUT, name + ".pdf")
    png = os.path.join(OUT, name + ".png")
    fig.savefig(pdf)                       # vector, pdfTeX-native
    fig.savefig(png, dpi=200, facecolor="white")  # fallback
    plt.close(fig)
    print("wrote", pdf)
    print("wrote", png)


def mq2_comparison():
    """MQ-2 raw ADC response across normal vs flame-trial sessions."""
    fig, axes = plt.subplots(
        len(SESSIONS), 1, figsize=(10.8, 4.95), sharex=False
    )

    for ax, name in zip(axes, SESSIONS):
        df = load(name)
        ax.plot(df["time_s"], df["mq2_adc"], lw=0.9, color="tab:green")

        if "label" in df.columns:
            abnormal = (df["label"] != "normal").values
            start = None
            for i, flag in enumerate(list(abnormal) + [False]):
                if flag and start is None:
                    start = i
                elif not flag and start is not None:
                    ax.axvspan(
                        df["time_s"].iloc[start],
                        df["time_s"].iloc[i - 1],
                        color="orange", alpha=0.3,
                    )
                    start = None

        tag = "flame trial" if "flame" in str(
            df.get("experiment", pd.Series()).iloc[0] if "experiment" in df
            else "normal"
        ) else "normal"
        ax.set_ylabel("MQ-2\n(raw ADC)", fontsize=8)
        ax.set_title(name, loc="left", fontsize=8)
        ax.tick_params(labelsize=8)

    axes[-1].set_xlabel("Time since session start (s)", fontsize=9)
    fig.suptitle("MQ-2 raw ADC response: normal baselines vs flame trials",
                 fontsize=10)
    fig.tight_layout(rect=(0, 0, 1, 0.96))
    save(fig, "vega_sentinel_mq2_comparison")


def flame_incident():
    """The documented flame-trial session with the AI decision context."""
    df = load("sentinel_data_20260911_190322.csv")

    fig, axes = plt.subplots(4, 1, figsize=(10.8, 4.95), sharex=True)
    fig.suptitle(
        "Flame trial 2026-09-11 19:03 (session sentinel_data_20260911_190322)",
        fontsize=10,
    )

    axes[0].plot(df["time_s"], df["temp_c"], color="tab:red", lw=1.0)
    axes[0].set_ylabel("Temp (C)", fontsize=8)

    axes[1].plot(df["time_s"], df["humidity_pct"], color="tab:blue", lw=1.0)
    axes[1].set_ylabel("RH (%)", fontsize=8)

    axes[2].plot(df["time_s"], df["mq2_adc"], color="tab:green", lw=1.0)
    axes[2].set_ylabel("MQ-2\n(raw ADC)", fontsize=8)

    axes[3].step(df["time_s"], df["flame_state"], where="post",
                color="black", lw=1.2)
    axes[3].set_ylabel("Flame DO", fontsize=8)
    axes[3].set_xlabel("Time since session start (s)", fontsize=9)

    # Event shading + annotation
    for ax in axes:
        abnormal = (df["label"] != "normal").values
        start = None
        for i, flag in enumerate(list(abnormal) + [False]):
            if flag and start is None:
                start = i
            elif not flag and start is not None:
                ax.axvspan(
                    df["time_s"].iloc[start], df["time_s"].iloc[i - 1],
                    color="orange", alpha=0.3,
                )
                start = None
        ax.tick_params(labelsize=8)

    flame_rows = df[df["flame_state"] == 1]
    if len(flame_rows):
        t_hit = flame_rows["time_s"].iloc[0]
        axes[3].annotate(
            "flame detected\n(1 sample in window:\nflame_fraction = 0.1)",
            xy=(t_hit, 1), xytext=(t_hit - 8, 0.55), fontsize=7,
            arrowprops=dict(arrowstyle="->", lw=0.8),
        )

    axes[3].text(
        0.99, 0.08,
        "Live AI window: p ~ 0.34 < 0.5 -> NORMAL (documented miss).\n"
        "Root cause: only 7 abnormal windows in training data.",
        transform=axes[3].transAxes, fontsize=7, ha="right", va="bottom",
        bbox=dict(boxstyle="round", facecolor="mistyrose", alpha=0.9),
    )

    fig.tight_layout(rect=(0, 0, 1, 0.95))
    save(fig, "vega_sentinel_flame_incident")


if __name__ == "__main__":
    print("Regenerating report figures as vector PDFs...")
    mq2_comparison()
    flame_incident()
    print("Done.")
