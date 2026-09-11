# Hackathon Execution Plan — EMBRIX'26 VEGATHON 2026

**Team:** Abnormal Detectors (4 members) · SSNCE
**Track:** Track 2 – Edge AI & TinyML Challenge
**Platform:** VEGA ARIES v2.0
**Constraint:** 48-hour hackathon window (design feasibility is a first-class requirement)

## Guiding principles

1. **Reliability over sophistication** — a working system beats an impressive paper design.
2. **Genuine AI** — a learned decision function; never `if flame → buzzer` disguised as ML; never a tuned-to-demo threshold.
3. **Physical implementation** — real sensors, real alarm, real board.
4. **Reproducibility** — every result comes from committed scripts + committed data.
5. **Explainability** — 7 interpretable features; coefficients a judge can read.
6. **VEGA compatibility** — only verified board capabilities; no unverified TinyML frameworks.
7. **Safety** — small supervised stimuli only.
8. **Measurable performance** — claims only for measured things; "Not yet validated" for the rest.

## Phase timeline (as actually executed + remaining)

| Phase | Work | Status |
|---|---|---|
| Setup | Arduino IDE 1.8.19 + VEGA core; board identity pinned to ARIES v2.0 | ✅ done |
| Bring-up | hello world, RGB LED, buttons, ADC (ADS1015) | ✅ done |
| Sensors | MQ-2 (+ divider), DHT22 (bit-banged driver), flame (active-LOW verified) | ✅ done |
| Output | 2N2222 buzzer driver built + physically tested | ✅ done |
| Data | multi-sensor CSV logging; PC logger with labels/phase/experiment; sessions collected | ✅ done (more data needed) |
| ML v0 | window features, feature analysis, initial experiments (distance-based, trees) | ✅ done |
| ML Model A | logistic regression trained; C++ reproduction; on-board execution | ✅ done (development model) |
| Real-time system | sliding-window inference + temporal confirmation + alarm | ✅ done; DHT-validity window fix applied |
| Failure documented | real flame trial missed (p ≈ 0.34) — recorded honestly | ✅ documented |
| **Dataset expansion** | 20–30 min normal + multiple supervised abnormal trials + recovery | ⏳ **current priority** |
| **Retrain + validate** | trial-level splits; FP rate + abnormal recall; deploy | ⏳ pending |
| Physical re-test | repeat controlled trials with improved model | ⏳ pending |
| Benchmarks | on-device inference timing + memory | ⏳ pending |
| Packaging | enclosure/prototype | ⏳ pending |
| Presentation | final demo script + deck | ⏳ pending |

## Demo script (planned, post-retrain)

1. Show serial CSV stream + live AI decision lines on a projector.
2. Normal environment → NORMAL decisions, buzzer silent.
3. Supervised controlled stimulus (small flame/smoke source, physically separated) → AI windows flip → 2 consecutive abnormal → buzzer.
4. Remove stimulus → recovery → 3 consecutive normal → buzzer clears.
5. Explain the model: 7 features, coefficients, why temporal confirmation exists.
6. Honest Q&A: dataset scale, the documented failure case, validation status.

## Fallbacks

- If fresh training data cannot be collected in time: demonstrate Model A as
  a development model with the documented failure case, and show the
  reproducible training pipeline — **never** claim unvalidated accuracy.
- If serial/upload fails: bring-up fixes first (J12 BOOT-SEL, correct
  programmer, COM port) — documented in `docs/hardware.md`.

## Division of work (4 members)

| Member focus | Owns |
|---|---|
| Firmware/embedded | sketches, final firmware, board bring-up |
| ML | dataset pipeline, training, validation, export |
| Hardware | wiring, divider, buzzer driver, safety of trials |
| Docs/testing/integration | test records, README/docs, demo logistics |

## Risk register (top risks and mitigations)

| Risk | Mitigation |
|---|---|
| DHT22 read failures corrupt windows | validity gate before window shift (implemented + tested pattern) |
| MQ-2 baseline drift across days | re-baseline before sessions; drift features in future model |
| Dataset too small to demo reliably | prioritize data collection; demo with honest framing |
| Upload/COM issues | documented recovery steps; test early each morning |
| Overfitting to demo environment | trial-level validation; report numbers honestly |
