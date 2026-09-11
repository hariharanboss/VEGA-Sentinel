"""VEGA Sentinel - model validation (leakage-aware).

Compares candidate feature sets and, most importantly, performs
**experiment-level validation**: whole physical trials are held out so
that windows from the same trial never appear in both train and test.
This addresses the overlap-leakage concern in the historical random-split
cross-validation.

Because the committed dataset contains abnormal windows from only 3
trials, experiment-level validation is extremely data-limited. Results
are reported honestly and must be treated as indicative only.

Usage (from repository root):
    python ml/validation/validate_model.py
"""
import argparse
import os

import numpy as np
import pandas as pd
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import (
    accuracy_score,
    classification_report,
    confusion_matrix,
)
from sklearn.pipeline import Pipeline
from sklearn.preprocessing import StandardScaler

MODEL_A_FEATURES = [
    "humidity_mean", "temp_mean", "mq2_range",
    "mq2_delta", "mq2_std", "humidity_std", "flame_fraction",
]


def evaluate_random_cv(X, y, features, folds=7, seed=42):
    """Historical evaluation: stratified CV with random window split.

    Kept for comparison, but this can leak overlapping windows between
    train and test folds.
    """
    from sklearn.model_selection import StratifiedKFold, cross_val_score

    model = Pipeline([
        ("scaler", StandardScaler()),
        ("classifier", LogisticRegression(
            max_iter=1000, class_weight="balanced", random_state=seed
        )),
    ])
    cv = StratifiedKFold(n_splits=folds, shuffle=True, random_state=seed)
    scores = cross_val_score(
        model, X[features], y, cv=cv, scoring="accuracy"
    )
    return scores.mean(), scores.std()


def evaluate_leave_one_trial_out(df, features, seed=42):
    """Leakage-aware evaluation: hold out one experiment (trial) at a time.

    Only trials containing abnormal windows are useful as test folds for
    abnormal recall; normal-only trials serve as false-positive probes.

    Returns a list of dicts with per-fold results.
    """
    y = df["anomaly_label"]
    results = []

    trials = sorted(df["source_file"].unique())

    for held_out in trials:
        test_mask = df["source_file"] == held_out
        train_mask = ~test_mask

        y_train = y[train_mask]
        if y_train.nunique() < 2:
            # Cannot train without both classes in the training folds.
            results.append({
                "held_out_trial": held_out,
                "status": "skipped (training folds lack both classes)",
                "test_windows": int(test_mask.sum()),
                "test_abnormal": int(y[test_mask].sum()),
            })
            continue

        model = Pipeline([
            ("scaler", StandardScaler()),
            ("classifier", LogisticRegression(
                max_iter=1000, class_weight="balanced", random_state=seed
            )),
        ])
        model.fit(df.loc[train_mask, features], y_train)

        pred = model.predict(df.loc[test_mask, features])
        truth = y[test_mask]

        cm = confusion_matrix(truth, pred, labels=[0, 1])
        results.append({
            "held_out_trial": held_out,
            "status": "evaluated",
            "test_windows": int(test_mask.sum()),
            "test_abnormal": int(truth.sum()),
            "accuracy": float(accuracy_score(truth, pred)),
            "confusion_matrix (rows=truth 0/1, cols=pred 0/1)":
                cm.tolist(),
        })

    return results


def main():
    parser = argparse.ArgumentParser(
        description="Leakage-aware validation for VEGA Sentinel models"
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
    if "anomaly_label" not in df.columns:
        df["anomaly_label"] = (df["label"] != "normal").astype(int)

    y = df["anomaly_label"]

    print("=" * 60)
    print("VEGA SENTINEL - MODEL VALIDATION")
    print("=" * 60)
    print(f"Dataset: {dataset}")
    print(f"Windows: {len(df)} (normal={int((y==0).sum())}, "
          f"abnormal={int((y==1).sum())})")
    print(f"Trials  : {df['source_file'].nunique()}")
    print()

    # ----------------------------------------------------------
    # 1. Reference: random-split CV (historical method, leaky)
    # ----------------------------------------------------------
    mean_acc, std_acc = evaluate_random_cv(df, y, MODEL_A_FEATURES)
    print("-" * 60)
    print("Model A - 7-fold random-split CV (DEVELOPMENT, may leak)")
    print("-" * 60)
    print(f"Accuracy: {mean_acc:.4f} +/- {std_acc:.4f}")
    print("This is the historical 0.9446-style development figure.")
    print("NOT a real-world claim. See docs/dataset-methodology.md.")
    print()

    # ----------------------------------------------------------
    # 2. Leave-one-trial-out (leakage-aware)
    # ----------------------------------------------------------
    print("-" * 60)
    print("Model A - Leave-one-trial-out (leakage-aware)")
    print("-" * 60)
    results = evaluate_leave_one_trial_out(df, MODEL_A_FEATURES)
    for r in results:
        print(f"\nHeld out: {r['held_out_trial']}")
        print(f"  status        : {r['status']}")
        print(f"  test windows  : {r['test_windows']}")
        if "accuracy" in r:
            print(f"  test abnormal : {r['test_abnormal']}")
            print(f"  accuracy      : {r['accuracy']:.4f}")
            print(f"  confusion     : "
                  f"{r['confusion_matrix (rows=truth 0/1, cols=pred 0/1)']}")

    evaluated = [r for r in results if r["status"] == "evaluated"]
    if evaluated:
        accs = [r["accuracy"] for r in evaluated]
        print(f"\nLeave-one-trial-out mean accuracy over "
              f"{len(evaluated)} evaluated folds: {np.mean(accs):.4f}")
    else:
        print("\nNo fold could be evaluated (too few labelled trials).")

    print()
    print("=" * 60)
    print("HONESTY NOTE")
    print("=" * 60)
    print("The committed dataset has abnormal windows in only 3 trials.")
    print("Leave-one-trial-out results are extremely limited and noisy.")
    print("Robust validation requires the planned data collection")
    print("(20-30 min normal + multiple supervised abnormal trials).")
    print("Nothing here is a final real-world accuracy claim.")


if __name__ == "__main__":
    main()
