# Testing — VEGA Sentinel

> Testing philosophy: **bring-up sketches with explicit pass criteria**,
> then integration, then physical trials. Every pass criterion is observable
> in the serial monitor. No test result is claimed without having been run.

## 1. Board tests (`firmware/board_tests/`)

| Sketch | Verifies | Pass criteria |
|---|---|---|
| 01_hello_world | IDE + VEGA board package + upload + serial | prints once per second at 115200 |
| 02_rgb_led_test | Onboard RGB LED mapping & polarity | distinct R, G, B for 1 s each (GPIO24=R, 22=G, 23=B, active-low) |
| 03_adc_test | ADS1015 A0–A3 | stable readings, low jitter, tracks a known input |
| 08_simple_demo | Buttons + LED interaction | BTN0 → blue blink + "CHECK-IN acknowledged"; BTN1 → red "SIMULATED ALERT"; green heartbeat |

Status: **all executed successfully on our ARIES v2.0** during bring-up.

## 2. Sensor tests (`firmware/sensor_tests/`)

| Sketch | Verifies | Pass criteria |
|---|---|---|
| 04_mq2_raw_read | MQ-2 via divider on A0 | stabilises to a clean-air baseline after warm-up; rises with a safe controlled stimulus (e.g. alcohol swab near the sensor) |
| 05_dht22_test | DHT22 bit-banged driver | temperature tracks room temperature; humidity plausible; checksum errors rare |
| 06_combined_acquisition | aligned multi-sensor rows | one aligned row every 2.5 s with all values; MQ-2 stable; DHT errors visible, not silent |

Status: **executed successfully**; the bit-banged DHT22 driver proved
reliable on ARIES v2.0 where library approaches were avoided.

## 3. Data collection pipeline

- `07_data_logger` + `tools/data_logger/sentinel_logger.py`:
  row validation rejects malformed rows; rejected rows land in a separate
  CSV with a reason. **Executed across multiple sessions** (see
  `dataset/raw/`).
- Data QA: `tools/visualization/visualize_dataset.py` session plots
  (temperature/humidity/MQ-2/flame + shaded abnormal regions).

## 4. Model verification tests

| Test | What it proves | Result |
|---|---|---|
| `ml/deployment/model_a_test.cpp` (PC) | C++ math matches Python model | reference windows classified correctly |
| `firmware/sentinel_final/model_a_aries_test/` (on-board) | the same math executes on ARIES v2.0 | normal p = 0.018949 → NORMAL ✓; abnormal p = 0.992752 → ABNORMAL ✓ |
| `ml/deployment/export_model_to_cpp.py` | exported constants reproduce reference probabilities | refuses to write `model.h` otherwise |

These prove **math execution**, not real-world accuracy.

## 5. Hardware output test

- Buzzer driver (GPIO2 → 1 kΩ → 2N2222 → 5 V buzzer): physically verified
  ON/OFF; ARIES GPIO load ≈ 2.6 mA (within the 12 mA limit).

## 6. Physical trials

- **Controlled flame trial (2026-09-11, sessions 182814/190122/190322):**
  flame sensor detected the flame during supervised trials (flame=1 rows
  exist in the committed data).
- **Documented failure:** one live sliding-window run scored the event
  p ≈ 0.34 → NORMAL → buzzer OFF (flame_fraction was only 0.1 in that
  window). This is preserved as an honest engineering result; the planned
  response is dataset expansion + retraining, **not** a hard-coded
  flame→buzzer rule and **not** a tuned threshold.

## 7. Not yet tested / not yet validated

- Final physical validation with an improved model (after retraining).
- Robust false-positive evaluation on fresh normal data.
- On-device inference timing and memory benchmarks.
- Long-duration soak tests.
- Do not mark these complete until performed.

## 8. Regression rules for future changes

1. Firmware feature order must match `model.h` (`export_model_to_cpp.py`
   enforces the reference-window check).
2. Offline window/feature logic must remain identical to on-device logic.
3. Any new test must record its pass criteria and raw observations —
   no claimed results without executed tests.
4. Safety: all stimuli small, supervised, separated from electronics;
   never dangerous gas concentrations or uncontrolled fire.
