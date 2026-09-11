# Dataset — VEGA Sentinel

> **Honesty first:** no measurements are fabricated. The dataset is small,
> and the model trained on it is a development model. Real-world accuracy
> is **not yet validated**.

## Folders

| Folder | Contents |
|---|---|
| `raw/` | Collection sessions exactly as logged (see below) |
| `processed/` | Window-feature datasets (v3 = current training set; v1/v2 = development archives) |
| `sample/` | One small sample session demonstrating the format |

## Raw format

Firmware emits (serial, 115200): `timestamp_ms,temp_c,humidity_pct,mq2_adc,flame_state,dht_ok`
The PC logger (`tools/data_logger/sentinel_logger.py` v0.4) enriches each row:

```csv
laptop_timestamp,timestamp_ms,temp_c,humidity_pct,mq2_adc,flame_state,dht_ok,label,experiment,phase
2026-09-11T18:28:14.611,3353855,28.00,71.00,191,0,1,normal,flame_test,baseline
```

- `label`: `normal` / `warning` / `critical` (operator-annotated)
- `phase`: `baseline` / `event` / `recovery` (when recorded)
- `experiment`: session name (trial identity for leakage-aware validation)
- `mq2_adc`: **raw ADC counts 0–2047** (ADS1015 single-ended, after the 2×2.2 kΩ divider) — **not ppm, not calibrated**
- `dht_ok`: 1 = valid DHT22 sample; 0 = failed read (temp/hum = −999) — excluded from training

## Sessions in `raw/` (meaningful, used)

| File | Rows | ~Duration | Experiment | Notes |
|---|---|---|---|---|
| `sentinel_data_20260911_153823.csv` | 395 | 16.5 min | baseline | normal environment; no events |
| `sentinel_data_20260911_165323.csv` | 491 | 20.5 min | baseline | normal environment; no events |
| `sentinel_data_20260911_182814.csv` | 123 | 5.1 min | flame_test | supervised flame trial incl. event + recovery; 4 flame detections |
| `sentinel_data_20260911_190122.csv` | 46 | 1.9 min | flame_test_2 | critical-labelled event (no flame DO hits) |
| `sentinel_data_20260911_190322.csv` | 30 | 1.2 min | flame_test_2 | critical-labelled event; 1 flame detection (the documented missed-detection trial) |

## Sessions NOT treated as final training data

- `sentinel_data_20260911_164627.csv` — short logger test (51 rows incl. header; ~2 min)
- `sentinel_data_20260911_180124.csv`, `sentinel_data_20260911_180838.csv` — short tests / artificial or problematic labels

These are intentionally **not committed** to `raw/`. Historical labels in
early experiments include artificial/test entries; we do not silently reuse
them for training claims.

## Processed datasets (`processed/`)

| File | Contents |
|---|---|
| `sentinel_window_features_v3.csv` | **Current training set** — 107 windows (100 normal / 7 abnormal), 10-sample non-overlapping windows from the 5 meaningful sessions |
| `archive_sentinel_window_features_v1.csv` | development archive (overlapping windows, superseded) |
| `archive_sentinel_window_features_v2.csv` | development archive (non-overlapping, earlier file set) |

A window is abnormal iff it contains ≥1 `critical` sample. Features are
documented in `docs/dataset-methodology.md`; the 7 model features in
`docs/tinyml-model.md`.

## Regeneration

Everything in `processed/` can be regenerated reproducibly:

```bash
python ml/data_processing/generate_windows.py            # v3 logic (non-overlapping)
python ml/data_processing/generate_windows.py --stride 1 # deployment-style sliding windows
```

## Provenance & limitations

- Collected 2026-09-11 on the actual hardware build (ARIES v2.0 + MQ-2 divider + DHT22 + flame sensor), one room, one day.
- MQ-2 values depend on warm-up state and the specific module; they are raw responses — never ppm.
- `critical` labels are operator annotations during supervised trials, not independently measured ground truth.
- 107 windows / 7 abnormal is far too small for robust generalization — the reason the project's current priority is dataset expansion (see `MASTER_CONTEXT.md §13`).
