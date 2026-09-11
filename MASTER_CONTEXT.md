# MASTER_CONTEXT — VEGA Sentinel

> **Purpose of this file:** preserve the complete, verified project context so that any future contributor — human or AI coding assistant — can continue the work without re-discovering facts the hard way. Read this file before writing any code or documentation.

**Project:** VEGA Sentinel: Edge-AI Multi-Sensor Anomaly Detection for Enclosed Space Safety
**Team:** Abnormal Detectors (4 members) — Sri Sivasubramaniya Nadar College of Engineering (SSNCE)
**Hackathon:** EMBRIX'26 – VEGATHON 2026, Track 2 (Edge AI & TinyML Challenge)

---

## 1. Board identity — read this first

- The target board is **VEGA ARIES v2.0**.
- **Do not confuse it with VEGA ARIES IoT v2.0.** Any documentation, code, pin information, or claims must refer specifically to **ARIES v2.0**.
- Verified board facts (official C-DAC/VEGA documentation):

| Item | Value |
|---|---|
| Processor | THEJAS32 / VEGA ET1031 (RISC-V) |
| Clock | 100 MHz |
| SRAM | 256 KB |
| Flash | 2 MB |
| I/O voltage | 3.3 V |
| Max I/O current | 12 mA per I/O |
| Analog inputs | 4 (A0–A3, via onboard ADS1015) |
| Arduino IDE | 1.8.19 |
| VEGA Arduino core used | C-DAC **v1.1.2** |
| Serial baud | 115200 |

Official sources:
- C-DAC: https://www.cdac.in/index.aspx?id=product_details&productId=ARIESv2.0
- VEGA: https://vegaprocessors.in/ariesv2.php
- Datasheet: https://vegaprocessors.in/files/ARIESv2%200_Datasheet_v2.pdf
- Pinout: https://vegaprocessors.in/files/PINOUT_ARIES%20V2.0_.pdf

**Rule: never invent board capabilities.** If additional VEGA-specific information is needed, verify from official C-DAC/VEGA documentation.

## 2. Hardware (verified, final)

Current final hardware — **do not reintroduce an LCD or a relay** unless explicitly requested:

1. VEGA ARIES v2.0
2. MQ-2 gas/smoke sensor module
3. DHT22 temperature/humidity sensor
4. 3-pin IR flame sensor module (active-LOW DO)
5. 2N2222 NPN transistor
6. 1 kΩ resistor (transistor base)
7. 5 V active buzzer (external 5 V supply)
8. Two 2.2 kΩ resistors (MQ-2 analog divider)
9. USB connection for programming/data logging

### Verified wiring

```
DHT22:                          MQ-2:                          FLAME SENSOR:
  VCC  → ARIES 3.3V                VCC → 5V                       VCC → ARIES 3.3V
  DATA → ARIES GPIO0               GND → GND                      GND → ARIES GND
  GND  → ARIES GND                 AO  → divider → ARIES A0       DO → ARIES GPIO1
                                   DO  → not used

MQ-2 divider:                    Buzzer driver:
  MQ-2 AO                           ARIES GPIO2 → 1 kΩ → 2N2222 Base
    |                              2N2222 Emitter  → GND
   2.2 kΩ                          2N2222 Collector → buzzer −
    |                              Buzzer +        → external 5V
    +----→ ARIES A0                External 5V GND → common GND
    |
   2.2 kΩ
    |
   GND
```

Hard rules:
- The buzzer is **never driven directly from a GPIO** (12 mA I/O limit). The 2N2222 driver is mandatory.
- The flame sensor was **experimentally verified active-LOW**: LOW = flame detected, HIGH = no flame. Firmware uses `flame = (digitalRead(FLAME_PIN) == LOW) ? 1 : 0`.

## 3. Software environment

- Arduino IDE 1.8.19 + VEGA Arduino package (`https://gitlab.com/riscv-vega/vega-arduino/-/raw/main/package_vega_index.json`), Board: **VEGA Processor: ARIES Boards → ARIES v2**.
- Core version in use during development: **C-DAC v1.1.2**.
- `analogRead()` on ARIES v2/v3/micro talks to the onboard **ADS1015**. The **effective single-ended result used in our testing is 0–2047** — do NOT describe it as 0–4095. (The full 12-bit differential range of the ADS1015 is a different thing; our raw MQ-2 readings live in 0–2047.)
- MQ-2 analog values are **RAW ADC readings** — they are **not** calibrated gas concentration. **Never claim ppm** unless proper calibration is performed.
- Python side: pandas / numpy / scikit-learn / matplotlib / pyserial (see `requirements.txt`).

## 4. Verified sensor software (known-good reference)

DHT22 is read with a **direct bit-banged driver** (this worked reliably on ARIES v2.0; library-based reads were avoided on purpose). The known-good reference implementation lives in `firmware/sensor_tests/05_dht22_test/05_dht22_test.ino` and is reused in the final firmware (`firmware/sentinel_final/`).

Key DHT22 facts:
- ≥ 2 s between reads (hence the 2500 ms sampling interval).
- Checksum = data[0]+data[1]+data[2]+data[3], must equal data[4].
- Conversion: humidity = rawHumidity/10; temperature is negative when rawTemperature & 0x8000 (mask, then /10).
- On this BSP, `pinMode()` only honours INPUT(0)/OUTPUT(1); `INPUT_PULLUP` is a no-op — an external pull-up on the DATA line is required for the bit-banged read (present on our module/wiring).

## 5. Data format

**Firmware output (serial):**
```
timestamp_ms,temp_c,humidity_pct,mq2_adc,flame_state,dht_ok
```

**PC logger CSV** (`tools/data_logger/sentinel_logger.py`, v0.4 format):
```
laptop_timestamp,timestamp_ms,temp_c,humidity_pct,mq2_adc,flame_state,dht_ok,label,experiment,phase
```

- Labels: `normal`, `warning`, `critical`. Phases: `baseline`, `event`, `recovery`.
- **Honesty rule:** not all historical labels represent verified physical events. Some historical files contain artificial/test labels; those files are excluded from training-data claims (see `dataset/README.md`). Never fabricate or silently reuse invalid historical labels.

## 6. Sensor data collection

- Sampling interval: **2500 ms**.
- Raw CSVs live in `dataset/raw/`; the processed window dataset (v3) in `dataset/processed/`.
- Meaningful raw files (used):
  - `sentinel_data_20260911_153823.csv` (395 rows, ~16.5 min, normal baseline)
  - `sentinel_data_20260911_165323.csv` (491 rows, ~20.5 min, normal baseline)
  - `sentinel_data_20260911_182814.csv` (123 rows, ~5 min, flame_test incl. event+recovery)
  - `sentinel_data_20260911_190122.csv` (46 rows, ~1.9 min, flame_test_2)
  - `sentinel_data_20260911_190322.csv` (30 rows, ~1.2 min, flame_test_2, one flame=1 row)
- Files **NOT** treated as final training data: `sentinel_data_20260911_164627.csv`, `sentinel_data_20260911_180124.csv`, `sentinel_data_20260911_180838.csv` (short logger tests, artificial labels, or labeling problems).

## 7. Feature engineering (current)

Window = **10 samples**, sampling ≈ every 2.5 s → one window ≈ **22.5 s**.

| # | Feature | Definition |
|---|---|---|
| 1 | `humidity_mean` | mean of humidity_pct over window |
| 2 | `temp_mean` | mean of temp_c over window |
| 3 | `mq2_range` | max − min of mq2_adc over window |
| 4 | `mq2_delta` | last − first mq2_adc in window |
| 5 | `mq2_std` | sample std (n−1) of mq2_adc |
| 6 | `humidity_std` | sample std (n−1) of humidity_pct |
| 7 | `flame_fraction` | fraction of samples with flame=1 |

**Consistency requirement:** offline feature generation must match the on-device implementation exactly (n−1 std, same window/stride semantics).

## 8. Current model (Model A — development model, NOT final)

Logistic-regression-style linear classifier + sigmoid. Trained with scikit-learn `LogisticRegression(max_iter=1000, class_weight="balanced", random_state=42)` on `StandardScaler`-standardised features, using the 107-window v3 dataset (100 normal / 7 abnormal).

Feature order (must be preserved everywhere):
```
humidity_mean, temp_mean, mq2_range, mq2_delta, mq2_std, humidity_std, flame_fraction
```

| Feature | Coefficient | Scaler mean | Scaler scale |
|---|---|---|---|
| humidity_mean | −2.610233 | 73.242897 | 1.548508 |
| temp_mean | +0.422538 | 28.448692 | 0.203029 |
| mq2_range | +0.384768 | 11.616822 | 17.851226 |
| mq2_delta | −0.322069 | 0.485981 | 14.173402 |
| mq2_std | −0.751392 | 3.734761 | 5.900505 |
| humidity_std | +0.561356 | 0.115325 | 0.163725 |
| flame_fraction | +0.890513 | 0.004673 | 0.031721 |
| Intercept | −3.483560 | | |

Decision rule: `p ≥ 0.5 → ABNORMAL`, else NORMAL. **Do not change the threshold merely to force a demonstration to succeed.**

Deployment verification (math reproduction only, on ARIES):
- Reference normal window → p = 0.018949 → NORMAL ✓
- Reference abnormal window → p = 0.992752 → ABNORMAL ✓

This proves the model math executes on ARIES. **It does NOT prove real-world accuracy.**

## 9. Dataset limitations — do not overclaim

- Processed dataset: **107 windows (100 normal / 7 abnormal)** — insufficient for robust generalization claims.
- Historical CV result: **0.9446 mean accuracy** (7-fold stratified) — a **development result with overlap-leakage caveats** (random splitting of windows from the same trial). Never call it "95% accurate real-world system" or "94.5% final accuracy".
- **Documented real failure:** a controlled flame experiment produced a live window with `flame_fraction = 0.1` and AI probability ≈ 0.34 → classified NORMAL → buzzer stayed OFF. The flame sensor detected the flame; the AI did not generalize to it. Interpretation: the current model/data combination did not generalize to that event. **Do not hide this failure and do not replace the AI with `if (flame == 1) buzzer = ON`.** The correct response is improving the dataset/model.

## 10. Real-time architecture (current firmware)

1. Initial collection of 10 valid samples fills the window.
2. Every new **valid** sample: shift the 10-sample window → insert newest sample → compute features → run AI → classify NORMAL/ABNORMAL. Inference occurs ≈ every 2.5 s after the window is full.
3. **Temporal confirmation (intentional):**
   - 2 consecutive abnormal AI windows → buzzer ON
   - 3 consecutive normal AI windows while alarm active → buzzer OFF
   - This prevents a single noisy AI decision from activating/clearing the alarm.

### Fixed firmware bug (must stay fixed)

Earlier firmware shifted the window before confirming the new DHT22 reading was valid → stale data could remain as if fresh. The correct sequence (implemented in `firmware/sentinel_final/`):
1. Read all sensors into temporary variables.
2. Verify DHT validity.
3. Only after a valid sample: shift window → insert sample → run inference.
4. If DHT fails: discard that sample, do **not** shift the window, do **not** run inference, preserve the current alarm state.

Never use stale sensor values as new measurements.

## 11. What is completed

- Project concept, problem definition, VEGATHON Track 2 alignment, ARIES v2.0 identification
- Arduino/VEGA environment setup; serial communication; GPIO, LED, button testing
- MQ-2, DHT22, flame sensor integration; multi-sensor reading; CSV logging; dataset logger
- Buzzer transistor driver; physical buzzer test
- Initial feature engineering; initial ML experiments; Model A
- C++ model reproduction; model execution on ARIES
- Sliding-window inference architecture; AI-to-buzzer architecture

## 12. What is NOT finished — do not mark complete

- Final training dataset; final model; proper trial-level validation
- Robust false-positive evaluation; final physical validation
- Final enclosure/prototype packaging; final dashboard; final presentation
- Final benchmark measurements; deployment memory/inference-timing documentation

## 13. Current priority (next major task)

**AI DATASET + MODEL IMPROVEMENT** — not more hardware. Required sequence:
1. Fix COM/serial/upload issues if necessary.
2. Collect 20–30 minutes of current-environment normal data.
3. Collect multiple safe, supervised abnormal trials.
4. Record recovery after each event.
5. Regenerate training windows using the SAME sliding-window method used during deployment.
6. Avoid random leakage between windows from the same physical trial.
7. Split validation by experiment/trial where possible.
8. Retrain.
9. Evaluate false positives and abnormal recall.
10. Deploy the improved model to ARIES.
11. Repeat physical tests.
12. Only then finalize the AI model.

## 14. Safety rules (non-negotiable)

- Safety-oriented **prototype**. Do not instruct anyone to create dangerous gas concentrations, uncontrolled fire, explosive atmospheres, or hazardous experiments.
- Controlled flame experiments must be small, supervised, and physically separated from electronics.
- MQ-2 is not a calibrated gas concentration instrument. Do not claim the system detects every hazardous gas. Say "**MQ-2 response**" or "**gas/smoke-related sensor response**" — never exact ppm.

## 15. Honesty rules (non-negotiable)

Never fabricate: accuracy, sensor readings, experiments, benchmark numbers, successful abnormal detection, hardware capabilities, TinyML framework support, GitHub push success, or final model performance. If something is unfinished, write "**Not yet validated**" or "**Development result — not final**" instead of inventing a result.

Positioning (approved wording): *"A resource-constrained multi-sensor Edge-AI safety system that learns normal environmental behaviour of an enclosed space and detects abnormal temporal patterns locally on the indigenous VEGA RISC-V platform, enabling local real-time safety response without requiring cloud connectivity."*

Do NOT claim: complete fire prediction, exact gas concentration, detection of every hazardous gas, medical-grade safety, industrial certification, guaranteed fire detection, or superior performance compared with ESP32/STM32/etc. without measured evidence.

## 16. Design philosophy

Prioritize: reliability · feasibility within a 48-hour hackathon · genuine AI · physical implementation · reproducibility · explainability · low complexity · VEGA compatibility · safety · measurable performance.

Do **not** add unnecessary sensors, displays, cloud services, mobile apps, or complex AI architectures merely to look sophisticated.

## 17. Code rules (for future AI coding assistants)

- All source code: readable, commented, meaningful names, no unnecessary dependencies, no unsupported VEGA libraries, ARIES v2.0-compatible, mindful of 256 KB SRAM / 2 MB flash, handle sensor failures, avoid blocking behaviour where practical, document pin assignments.
- **Do not introduce TensorFlow Lite Micro, Edge Impulse, or any TinyML framework** unless verified to work on ARIES v2.0. Direct C/C++ inference is acceptable and preferred for this project.
- No LCD, no relay, no additional hardware in the final system unless explicitly requested.
- No dynamic allocation in firmware; fixed-size window buffers.
- Keep feature order and model constants in sync: `ml/deployment/export_model_to_cpp.py` auto-generates `firmware/sentinel_final/model.h` — never hand-edit the constants.

## 18. Repository map

```
firmware/    board_tests/ sensor_tests/ data_collection/ sentinel_final/
ml/          data_processing/ feature_analysis/ training/ validation/ deployment/
dataset/     README.md raw/ processed/ sample/
hardware/    README.md bom.md wiring/ schematics/
tools/       data_logger/ visualization/
docs/        project-overview.md system-architecture.md hardware.md sensor-wiring.md
             dataset-methodology.md tinyml-model.md testing.md hackathon-execution-plan.md
results/     experiments/ model_results/ graphs/
images/      prototype/ architecture/ testing/
```

## 19. Git conventions

- Honest history only — never fake commit history for work not performed; commit only files that exist.
- Suggested commit subjects: "Initialize VEGA Sentinel repository", "Add ARIES sensor firmware", "Add data collection pipeline", "Add ML training pipeline", "Add hardware documentation", "Add project documentation".
- `.gitignore` excludes secrets, venvs, IDE artifacts, build outputs, local COM-port configs.
- License: **MIT**, Team Abnormal Detectors / VEGA-Sentinel, SSNCE.
