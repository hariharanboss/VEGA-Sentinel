"""VEGA Sentinel - Model A training (logistic regression).

Trains the logistic-regression classifier on the window feature dataset,
reports 7-fold stratified cross-validation, prints the embedded constants
(scaler means/scales, coefficients, intercept) and saves them to JSON for
the C++ export step.

This is the reproducible version of the script that produced Model A:
the printed constants must match firmware/sentinel_final/model.h.

IMPORTANT: cross-validation here randomly splits windows. Windows from the
same physical trial can appear in both train and test folds (overlap
leakage). Treat the CV result as a DEVELOPMENT estimate, not real-world
accuracy. See ml/validation/validate_model.py for leakage-aware evaluation.

Usage (from repository root):
    python ml/training/train_model.py
    python ml/training/train_model.py --dataset path/windows.csv
"""
import argparse
import json
import os

import pandas as pd
from sklearn.linear_model import LogisticRegression
from sklearn.model_selection import StratifiedKFold, cross_val_score
from sklearn.pipeline import Pipeline
from sklearn.preprocessing import StandardScaler

# Feature order - must match model.h and firmware computation order
FEATURES = [
    "humidity_mean",
    "temp_mean",
    "mq2_range",
    "mq2_delta",
    "mq2_std",
    "humidity_std",
    "flame_fraction",
]

# Fixed training configuration (reproducibility)
RANDOM_STATE = 42
CV_FOLDS = 7  # only 7 abnormal windows exist - one per fold

OUTPUT_JSON = "model_a_constants.json"


def main():
    parser = argparse.ArgumentParser(
        description="Train Model A (logistic regression) for VEGA Sentinel"
    )
    parser.add_argument(
        "--dataset",
        default=None,
        help="Window features CSV (default: <repo>/dataset/processed/"
             "sentinel_window_features_v3.csv)",
    )
    parser.add_argument(
        "--output-json",
        default=None,
        help=f"Where to save the model constants JSON "
             f"(default: <repo>/results/model_results/{OUTPUT_JSON})",
    )
    args = parser.parse_args()

    repo_root = os.path.abspath(
        os.path.join(os.path.dirname(__file__), "..", "..")
    )
    dataset = args.dataset or os.path.join(
        repo_root, "dataset", "processed", "sentinel_window_features_v3.csv"
    )
    output_json = args.output_json or os.path.join(
        repo_root, "results", "model_results", OUTPUT_JSON
    )

    df = pd.read_csv(dataset)

    if "anomaly_label" not in df.columns:
        df["anomaly_label"] = (df["label"] != "normal").astype(int)

    X = df[FEATURES]
    y = df["anomaly_label"]

    print("=" * 60)
    print("VEGA SENTINEL - MODEL A TRAINING")
    print("=" * 60)
    print("\nFeature order (must match model.h):")
    for i, feature in enumerate(FEATURES):
        print(f"  {i}: {feature}")
    print(f"\nDataset: {dataset}")
    print(f"  Total windows : {len(df)}")
    print(f"  Normal        : {int((y == 0).sum())}")
    print(f"  Abnormal      : {int((y == 1).sum())}")

    # Logistic regression with balanced class weights (7 abnormal vs
    # 100 normal windows would otherwise be dominated by the normal class)
    model = Pipeline([
        ("scaler", StandardScaler()),
        ("classifier", LogisticRegression(
            max_iter=1000,
            class_weight="balanced",
            random_state=RANDOM_STATE,
        )),
    ])

    # ----------------------------------------------------------
    # Cross-validation (DEVELOPMENT ESTIMATE - random window split,
    # overlap leakage possible; see module docstring)
    # ----------------------------------------------------------
    print(f"\n{'=' * 60}\n{CV_FOLDS}-FOLD STRATIFIED CROSS-VALIDATION\n{'=' * 60}")

    cv = StratifiedKFold(
        n_splits=CV_FOLDS, shuffle=True, random_state=RANDOM_STATE
    )
    scores = cross_val_score(model, X, y, cv=cv, scoring="accuracy")

    print("Fold accuracies:")
    for i, score in enumerate(scores, 1):
        print(f"  Fold {i}: {score:.4f}")
    print(f"\nMean accuracy: {scores.mean():.4f}")
    print(f"Std deviation: {scores.std():.4f}")

    # ----------------------------------------------------------
    # Final fit on all available data (this is what gets deployed)
    # ----------------------------------------------------------
    model.fit(X, y)
    scaler = model.named_steps["scaler"]
    classifier = model.named_steps["classifier"]

    print(f"\n{'=' * 60}\nEMBEDDED CONSTANTS (for model.h)\n{'=' * 60}")

    print("\nStandardScaler parameters:")
    for feature, mean, scale in zip(FEATURES, scaler.mean_, scaler.scale_):
        print(f"  {feature:18s} mean={mean:.6f}  scale={scale:.6f}")

    print(f"\nLogistic regression intercept: {classifier.intercept_[0]:.6f}")
    print("\nLogistic regression coefficients:")
    for feature, coef in zip(FEATURES, classifier.coef_[0]):
        print(f"  {feature:18s} {coef:+.6f}")

    print(f"\n{'=' * 60}\nHONESTY NOTE\n{'=' * 60}")
    print("The CV accuracy above is a development estimate produced by")
    print("randomly splitting windows from the same physical trials.")
    print("It is NOT a real-world accuracy claim. The dataset has only")
    print(f"{int((y == 1).sum())} abnormal windows.")

    # ----------------------------------------------------------
    # Save constants for the C++ export step
    # ----------------------------------------------------------
    constants = {
        "model_name": "Model A",
        "model_type": "logistic_regression",
        "trained_on": os.path.relpath(dataset, repo_root),
        "windows_total": int(len(df)),
        "windows_normal": int((y == 0).sum()),
        "windows_abnormal": int((y == 1).sum()),
        "cv_mean_accuracy_development": float(scores.mean()),
        "random_state": RANDOM_STATE,
        "feature_order": FEATURES,
        "scaler_mean": [float(m) for m in scaler.mean_],
        "scaler_scale": [float(s) for s in scaler.scale_],
        "coefficients": [float(c) for c in classifier.coef_[0]],
        "intercept": float(classifier.intercept_[0]),
        "threshold": 0.5,
    }

    os.makedirs(os.path.dirname(output_json), exist_ok=True)
    with open(output_json, "w", encoding="utf-8") as f:
        json.dump(constants, f, indent=2)

    print(f"\nConstants saved: {output_json}")
    print("Next step: python ml/deployment/export_model_to_cpp.py")


if __name__ == "__main__":
    main()
