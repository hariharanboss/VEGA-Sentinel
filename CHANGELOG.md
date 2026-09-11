# Changelog — VEGA Sentinel

All notable changes to the VEGA Sentinel project are documented here.
Format loosely based on [Keep a Changelog](https://keepachangelog.com/).

**Project status:** active development — AI dataset/model refinement phase. The model is **not final**.

## [0.4.0] — 2026-09-12 (repository consolidation)

### Added
- GitHub-ready repository structure (firmware / ml / dataset / hardware / tools / docs / results / images).
- `README.md` — full hackathon-quality project documentation with verified numbers only.
- `MASTER_CONTEXT.md` — complete preserved project context for future contributors/AI assistants.
- `firmware/sentinel_final/` — complete real-time system: sensors → sliding window → on-board AI → temporal confirmation → transistor-driven buzzer, with the DHT-failure window-preservation fix (window is not shifted when the new sample is invalid).
- `ml/` — reproducible ML pipeline: window generation, feature analysis, training, leakage-aware validation, C++ export.
- `hardware/` — BOM, wiring tables, ASCII schematics for the verified hardware configuration.
- `docs/` — deep-dive documentation (architecture, dataset methodology, TinyML model, testing, execution plan).
- `dataset/` — 5 meaningful raw collection sessions + processed window datasets, with honest provenance notes.

### Fixed
- **Sliding-window stale-data bug:** earlier firmware shifted the window before confirming the new DHT22 sample was valid. The final firmware now reads all sensors to temporaries, validates, and only then shifts/inserts/infers; on DHT failure the sample is discarded, the window stays intact, and the alarm state is preserved.

### Hardware (verified on ARIES v2.0)
- MQ-2 (A0 via 2×2.2 kΩ divider, 5 V module), DHT22 (GPIO0, 3.3 V), IR flame sensor (GPIO1, active-LOW), 5 V buzzer via 2N2222 + 1 kΩ base resistor driven from GPIO2 (external 5 V rail, common ground).

### Known issues / open work
- Dataset of 107 windows (100 normal / 7 abnormal) is too small for real-world accuracy claims.
- Historical 0.9446 CV accuracy carries overlap-leakage caveats (development result only).
- Documented real failure: controlled flame trial (flame_fraction 0.1) scored p ≈ 0.34 → NORMAL → buzzer OFF. Response: more/better data + retraining, not threshold hacks.

## [0.3.0] — 2026-09-11 (sliding-window on-device inference)

### Added
- Sliding-window inference architecture on ARIES: 10-sample window, inference after each new valid sample (~2.5 s cadence).
- Safety confirmation logic: 2 consecutive abnormal windows → buzzer ON; 3 consecutive normal windows → buzzer OFF.
- Model A math reproduced in C++ and executed on ARIES (reference normal p=0.018949, abnormal p=0.992752 — both classified correctly).

## [0.2.0] — 2026-09-11 (sensors + dataset)

### Added
- MQ-2, DHT22 (bit-banged driver), flame sensor integration; combined 2.5 s acquisition.
- PC-side dataset logger with label/phase/experiment tagging and row validation.
- Window feature extraction (10-sample windows) and initial ML experiments (logistic regression, distance-based anomaly detector, decision trees).

## [0.1.0] — 2026-09-10/11 (bring-up)

### Added
- ARIES v2.0 board bring-up: serial, RGB LED, buttons, ADC (ADS1015, effective 0–2047 single-ended).
- Sensor bring-up sketches with pass criteria; data logger sketch; on-board demo.
