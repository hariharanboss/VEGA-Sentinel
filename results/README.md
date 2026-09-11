# results/ — Real Experiment & Model Results

Everything here is a **real, reproducible output** of committed code on
committed data (or a direct observation on hardware). Nothing is fabricated.
Outputs of pipeline scripts land here by default.

## experiments/

Physical test records and session analyses.

- `flame_trial_20260911.md` — the controlled flame trials of 2026-09-11,
  including the **documented missed detection** (AI p ≈ 0.34 → NORMAL while
  the flame sensor reported flame). Honest record; the driver for the
  current dataset-expansion priority.

## model_results/

- `sentinel_anomaly_results_v1.csv` — per-window output of the early
  distance-based anomaly detector experiment (development archive).
- `model_a_constants.json` — Model A embedded constants (written by
  `ml/training/train_model.py`; consumed by the C++ export).

## graphs/

Session plots from `tools/visualization/visualize_dataset.py` land here
when run with `--output results/graphs/`. (Regenerate rather than trusting
stale images.)

## Reproducibility

From the repository root:

```bash
python ml/data_processing/generate_windows.py          # windows -> results/
python ml/feature_analysis/analyze_features.py         # separation report (stdout)
python ml/training/train_model.py                      # CV + constants -> results/model_results/
python ml/validation/validate_model.py                 # leakage-aware validation (stdout)
```

Reproducible figures on the committed dataset (all re-run 2026-09-12):

| Metric | Value | Interpretation |
|---|---|---|
| Model A 7-fold random-split CV mean | **0.9446** | development estimate; overlap leakage possible |
| Model A leave-one-trial-out mean | **0.7449** | leakage-aware; the honest development number |
| Reference normal window p | 0.018949 | C++ export verification |
| Reference abnormal window p | 0.992752 | C++ export verification |

The gap between 0.9446 and 0.7449 is exactly why trial-level validation
matters — and why dataset expansion + retraining is the current priority.

## Not yet validated

- Final model performance (retraining is the current priority).
- Robust false-positive rate on fresh normal data.
- Abnormal recall on fresh supervised trials.
- On-device inference timing / memory benchmarks.
