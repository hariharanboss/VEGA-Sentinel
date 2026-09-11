# TinyML Model — Model A (development model)

> **Status: development model — NOT final.** It proves the Edge-AI pipeline
> works end-to-end on ARIES v2.0. It does **not** yet have validated
> real-world performance. See "Documented failure case" below.

## Model choice and rationale

**Logistic regression on standardised window features** (scikit-learn:
`LogisticRegression(max_iter=1000, class_weight="balanced", random_state=42)`
after `StandardScaler`).

Why this and not a bigger model:

| Requirement | Logistic regression |
|---|---|
| Runs in C++ on 256 KB SRAM / 2 MB flash | 21 floats + intercept (~100 bytes) |
| Inference cost | 7 multiply-adds + 1 sigmoid (~µs @ 100 MHz) |
| Explainable (hackathon judging) | every coefficient is directly interpretable |
| No unverified frameworks | pure arithmetic; no TFLite Micro / Edge Impulse (none verified on ARIES v2.0) |
| Small-data trainability | works with 107 windows where deep nets would overfit |

## Features (order is contract)

```
0: humidity_mean    1: temp_mean      2: mq2_range
3: mq2_delta        4: mq2_std       5: humidity_std
6: flame_fraction
```

Feature order is identical in: `ml/training/train_model.py`,
`ml/deployment/export_model_to_cpp.py`, `firmware/sentinel_final/model.h`,
and the firmware's `computeFeatures()`.

## Embedded constants (Model A)

| Feature | Coefficient | Scaler mean | Scaler scale |
|---|---|---|---|
| humidity_mean | −2.610233 | 73.242897 | 1.548508 |
| temp_mean | +0.422538 | 28.448692 | 0.203029 |
| mq2_range | +0.384768 | 11.616822 | 17.851226 |
| mq2_delta | −0.322069 | 0.485981 | 14.173402 |
| mq2_std | −0.751392 | 3.734761 | 5.900505 |
| humidity_std | +0.561356 | 0.115325 | 0.163725 |
| flame_fraction | +0.890513 | 0.004673 | 0.031721 |
| **Intercept** | **−3.483560** | | |

These constants are **auto-generated** into `firmware/sentinel_final/sentinel_final/model.h`
by `ml/deployment/export_model_to_cpp.py` (from `results/model_results/model_a_constants.json`).
Never hand-edit `model.h`.

## Inference (on-device)

```
z_i  = (x_i − mean_i) / scale_i            # standardisation
s    = intercept + Σ coef_i · z_i          # linear score
p    = 1 / (1 + e^(−s))                    # sigmoid, numerically clamped
class = (p ≥ 0.5) ? ABNORMAL : NORMAL
```

Threshold is **fixed at 0.5**. It is a project rule that the threshold must
never be tuned to force a demonstration to succeed.

## Training data

`dataset/processed/sentinel_window_features_v3.csv` — 107 windows
(100 normal / 7 abnormal), 10-sample non-overlapping windows from the
sessions documented in `dataset/README.md`. A window is abnormal if it
contains ≥1 `critical` label.

## Development results (reproducible)

`python ml/training/train_model.py` reproduces on the committed dataset:

- 7-fold stratified CV accuracies: 0.9375, 0.8750, 1.0000, 0.9333, 0.8667, 1.0000, 1.0000
- **Mean CV accuracy: 0.9446** — *development result with leakage caveats; NOT a real-world accuracy claim.*

C++ verification (PC: `ml/deployment/model_a_test.cpp`; on-board:
`firmware/sentinel_final/model_a_aries_test/`):

| Reference window | Probability | Classification |
|---|---|---|
| Normal example | 0.018949 | NORMAL ✓ |
| Abnormal example | 0.992752 | ABNORMAL ✓ |

This proves the model math executes identically in Python and on the
ARIES. It does **not** prove real-world accuracy.

## Documented failure case (do not hide this)

A real controlled flame experiment was performed on hardware. The flame
sensor detected the flame (a live window reached `flame_fraction = 0.1`).
Model A scored it at **p ≈ 0.34 < 0.5 → NORMAL** → the buzzer stayed OFF.

**Interpretation:** the current model/data combination did not generalize
to that real event — too few flame examples in training. The response is
**not** to hard-code `flame → buzzer` and call it AI; the response is to
collect more/better data and retrain with trial-level validation
(see `dataset-methodology.md` → "Next data collection plan").

Note: re-scoring the *specific* 10-sample window ending at the flame hit
(with `mq2_range` 59, `mq2_std` 17.6 — a very volatile window) yields
p ≈ 0.99 → ABNORMAL. The live-run miss (p ≈ 0.34) reflects window
composition at that moment (which samples were in the window when inference
ran) — a further argument for more data and careful evaluation, and a
reminder that overlapping-window effects are real.

## Validation honesty

- Random-split CV (above) can leak overlapping windows → optimistic.
- Leave-one-trial-out validation exists (`ml/validation/validate_model.py`)
  but is data-starved: only 3 trials contain abnormal windows.
- **Not yet validated:** false-positive rate on fresh normal data, abnormal
  recall on fresh events, on-device inference timing/memory benchmarks.

## Retraining workflow

```bash
# after collecting new data into dataset/raw/ (and updating file lists):
python ml/data_processing/generate_windows.py --files <new_files...> --stride 1 --output results/windows_new.csv
python ml/training/train_model.py --dataset results/windows_new.csv
python ml/validation/validate_model.py --dataset results/windows_new.csv
python ml/deployment/export_model_to_cpp.py --json results/model_results/model_a_constants.json
# export script verifies reference windows before writing firmware/sentinel_final/model.h
```
