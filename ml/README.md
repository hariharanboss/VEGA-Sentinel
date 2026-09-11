# ml/ — reproducible ML pipeline

From raw sensor CSVs to embedded C++ constants, with every result
reproducible from committed code and committed data.

```
ml/
├── data_processing/generate_windows.py   # raw CSVs -> window features
├── feature_analysis/analyze_features.py  # normal-vs-abnormal separation
├── training/train_model.py               # trains Model A -> constants JSON
├── validation/validate_model.py          # random CV + leakage-aware LOTO
├── deployment/export_model_to_cpp.py     # constants JSON -> firmware model.h
└── deployment/model_a_test.cpp          # PC-side C++ reference check
```

## Pipeline (run from the repository root)

```bash
pip install -r requirements.txt

python ml/data_processing/generate_windows.py     # windows from raw CSVs
python ml/feature_analysis/analyze_features.py    # feature ranking report
python ml/training/train_model.py                 # CV + embedded constants
python ml/validation/validate_model.py            # validation incl. LOTO
python ml/deployment/export_model_to_cpp.py       # regenerate model.h
```

Every script defaults to the committed dataset and writes outputs under
`results/`. No manual steps, no hand-copied numbers.

## Documented facts (must stay true)

- **Feature order** (index 0–6): `humidity_mean, temp_mean, mq2_range, mq2_delta, mq2_std, humidity_std, flame_fraction` — identical in the training script, the exported `model.h`, and the firmware computation.
- **Labels:** `0 = normal`, `1 = abnormal` (a window is abnormal when it contains ≥1 `critical` event label).
- **Split:** 7-fold stratified CV, `random_state=42`, `class_weight="balanced"`. This split is **random over windows** and can leak overlapping windows — the mean accuracy (≈0.9446 on the committed dataset) is a **development result, not final real-world accuracy**.
- **Model:** logistic regression + sigmoid; threshold fixed at 0.5.
- **Export verification:** `export_model_to_cpp.py` refuses to write `model.h` unless the exported constants reproduce the reference probabilities (normal p≈0.018949, abnormal p≈0.992752).

## Honest status

- Dataset: 107 windows (100 normal / 7 abnormal) — too small for real-world accuracy claims.
- Trial-level (leave-one-trial-out) validation exists in `validate_model.py` but is data-starved (abnormal windows come from only 3 trials). Not yet validated robustly.
- Model A is a **development model**. The next milestone is dataset expansion + retraining (see `MASTER_CONTEXT.md §13`).
