"""VEGA Sentinel - feature analysis: normal vs abnormal separation.

Ranks every candidate feature by how well it separates normal from
abnormal windows (Cohen's d-style separation score) and by correlation
with the anomaly label. This analysis was used to select the 7 Model A
features.

Usage (from repository root):
    python ml/feature_analysis/analyze_features.py
    python ml/feature_analysis/analyze_features.py --dataset path/to/windows.csv
"""
import argparse
import os

import numpy as np
import pandas as pd

# All candidate features produced by ml/data_processing/generate_windows.py
CANDIDATE_FEATURES = [
    "humidity_mean", "humidity_std", "humidity_delta", "humidity_slope_per_s",
    "temp_mean", "temp_std", "temp_delta", "temp_slope_per_s",
    "mq2_mean", "mq2_std", "mq2_range", "mq2_delta", "mq2_slope_per_s",
    "flame_count", "flame_fraction",
]

# The 7 features actually used by Model A
MODEL_A_FEATURES = [
    "humidity_mean", "temp_mean", "mq2_range",
    "mq2_delta", "mq2_std", "humidity_std", "flame_fraction",
]


def main():
    parser = argparse.ArgumentParser(
        description="Analyse feature separation between normal and abnormal"
    )
    parser.add_argument(
        "--dataset",
        default=None,
        help="Window features CSV (default: <repo>/dataset/processed/"
             "sentinel_window_features_v3.csv)",
    )
    args = parser.parse_args()

    repo_root = os.path.abspath(
        os.path.join(os.path.dirname(__file__), "..", "..")
    )
    dataset = args.dataset or os.path.join(
        repo_root, "dataset", "processed", "sentinel_window_features_v3.csv"
    )

    df = pd.read_csv(dataset)

    # Older archive datasets used 'anomaly_label'; keep both supported
    if "anomaly_label" not in df.columns and "label" in df.columns:
        df["anomaly_label"] = (df["label"] != "normal").astype(int)

    available = [f for f in CANDIDATE_FEATURES if f in df.columns]
    missing = [f for f in CANDIDATE_FEATURES if f not in df.columns]
    if missing:
        print(f"NOTE: features not present in dataset: {missing}")

    normal = df[df["anomaly_label"] == 0]
    abnormal = df[df["anomaly_label"] == 1]

    print("=" * 60)
    print("VEGA SENTINEL - FEATURE ANALYSIS")
    print("=" * 60)
    print(f"Dataset      : {dataset}")
    print(f"Total windows: {len(df)} (normal={len(normal)}, "
          f"abnormal={len(abnormal)})")
    print()

    print("-" * 60)
    print("NORMAL vs ABNORMAL per-feature statistics")
    print("-" * 60)
    for feature in available:
        n_mean, n_std = normal[feature].mean(), normal[feature].std()
        a_mean, a_std = abnormal[feature].mean(), abnormal[feature].std()
        print(f"\n{feature}")
        print(f"  Normal   : mean={n_mean:.4f}, std={n_std:.4f}")
        print(f"  Abnormal : mean={a_mean:.4f}, std={a_std:.4f}")

    print()
    print("-" * 60)
    print("SEPARATION SCORE (|mean difference| / pooled std)")
    print("-" * 60)
    scores = []
    for feature in available:
        pooled_std = np.sqrt(
            (normal[feature].var() + abnormal[feature].var()) / 2
        )
        if pooled_std == 0:
            score = 0.0
        else:
            score = abs(abnormal[feature].mean() - normal[feature].mean()) \
                    / pooled_std
        scores.append((feature, float(score)))

    scores.sort(key=lambda x: x[1], reverse=True)
    for i, (feature, score) in enumerate(scores, 1):
        marker = "  [Model A]" if feature in MODEL_A_FEATURES else ""
        print(f"{i:2}. {feature:24s} score = {score:.4f}{marker}")

    print()
    print("-" * 60)
    print("CORRELATION WITH ANOMALY LABEL")
    print("-" * 60)
    correlations = []
    for feature in available:
        corr = df[feature].corr(df["anomaly_label"])
        correlations.append((feature, abs(corr), corr))

    correlations.sort(key=lambda x: x[1], reverse=True)
    for feature, abs_corr, corr in correlations:
        print(f"{feature:24s} correlation = {corr:+.4f}")

    print()
    print("Model A features:", ", ".join(MODEL_A_FEATURES))
    print()
    print("NOTE: statistics come from a small development dataset")
    print("(7 abnormal windows). Rankings are indicative only.")


if __name__ == "__main__":
    main()
