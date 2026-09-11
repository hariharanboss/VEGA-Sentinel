# Controlled Flame Trials — 2026-09-11

> Real experiment record. Stimuli were small, supervised, and physically
> separated from the electronics. No dangerous conditions were created.

## Sessions

| Session (dataset/raw/) | Rows | Duration | Flame DO hits | Labels |
|---|---|---|---|---|
| sentinel_data_20260911_182814.csv | 123 | ~5.1 min | 4 | baseline 86 / event 10 / recovery 27; 15 critical rows |
| sentinel_data_20260911_190122.csv | 46 | ~1.9 min | 0 | baseline 27 / event 19; 19 critical rows |
| sentinel_data_20260911_190322.csv | 30 | ~1.2 min | 1 | baseline 12 / event 18; 17 critical rows |

## Observations

- The flame sensor detected the flame during the trials (flame=1 rows are
  present in the committed data: 4 hits in 182814, 1 hit in 190322).
- Sensor readings during the events: MQ-2 raw counts ranged ~101–259 with
  high window volatility (ranges up to ~59 counts), humidity ~68.7–71.7 %,
  temperature ~27.8–28.4 °C.
- The event windows are labelled `critical` by the operator; phase
  `event`/`recovery` was recorded in these sessions.

## Documented missed detection (important honest result)

During one live sliding-window run (session 190322), the window present at
inference time contained only **one** flame-positive sample
(`flame_fraction = 0.1`). Model A produced:

```
p(abnormal) ≈ 0.34  <  0.5   →   NORMAL   →   buzzer stayed OFF
```

**Interpretation:** the current model/data combination did not generalize
to that real event. Too few flame examples exist in the 107-window training
set; a single flame hit inside a window of otherwise ordinary readings
falls inside the learned normal region.

**Engineering response (decided, not negotiable):**

- ✗ Do **not** hard-code `if (flame) buzzer = ON` and present it as AI.
- ✗ Do **not** tune the 0.5 threshold to force the demo to pass.
- ✓ Collect more data (20–30 min normal + multiple supervised abnormal
  trials + recovery), regenerate windows with the deployment sliding-window
  method, retrain with trial-level splits, then re-run physical tests.

## Follow-up analysis (post-session, from committed data)

Re-scoring the exact 10-sample window ending at the flame=1 row of
session 190322 (mq2_range = 59, mq2_std ≈ 17.6, humidity_std ≈ 0.23) with
Model A yields p ≈ 0.99 → ABNORMAL. The live miss therefore depended on
which samples occupied the window at the moment inference ran — a concrete
illustration of window-composition sensitivity and of why evaluation must
use the same sliding-window semantics as deployment.

## Status

- Sensor hardware: **verified working** (flame detected physically and electrically).
- Model generalization: **not yet validated** — the reason dataset + model
  improvement is the project's current top priority (see MASTER_CONTEXT.md §13).
