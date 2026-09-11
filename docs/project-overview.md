# Project Overview — VEGA Sentinel

## Identity

| | |
|---|---|
| Project | VEGA Sentinel: Edge-AI Multi-Sensor Anomaly Detection for Enclosed Space Safety |
| Team | Abnormal Detectors (4 members) |
| College | Sri Sivasubramaniya Nadar College of Engineering (SSNCE) |
| Hackathon | EMBRIX'26 – VEGATHON 2026 |
| Track | Track 2 – Edge AI & TinyML Challenge |
| Platform | VEGA ARIES v2.0 (THEJAS32/VEGA ET1031 RISC-V @ 100 MHz, 256 KB SRAM, 2 MB flash) |

> ⚠️ The board is **ARIES v2.0** — not "ARIES IoT v2.0". All documentation refers specifically to ARIES v2.0.

## One-line pitch

A resource-constrained multi-sensor Edge-AI safety system that learns the normal environmental behaviour of an enclosed space and detects abnormal temporal patterns locally on the indigenous VEGA RISC-V platform — enabling local real-time safety response without requiring cloud connectivity.

## Problem

Enclosed spaces (labs, server rooms, storage areas, workshops, hostel rooms) can develop dangerous conditions: smoke, gas-related events, abnormal temperature/humidity drift, fire. Existing options:

- **Threshold alarms** look at one sensor against a fixed limit; they ignore how conditions change *together over time*, need manual tuning, false-alarm or miss events.
- **Cloud "smart" systems** require connectivity, add latency/cost/privacy concerns, and stop working when the network drops — unacceptable for a safety function.

## Solution

Fuse MQ-2 (gas/smoke-related response), DHT22 (temperature + humidity), and an IR flame sensor on ARIES v2.0. Every 2.5 s: sample → validate (DHT22 checksum) → 10-sample sliding window (~22.5 s) → 7 statistical features → on-board logistic-regression inference in C++ → temporal confirmation (2 consecutive abnormal windows → buzzer ON; 3 consecutive normal → OFF) → 2N2222-driven 5 V buzzer.

**The decision function is learned from collected data, not hand-coded thresholds.** This distinction is central to the project; we explicitly refuse the shortcut of hard-wiring `flame → buzzer` and calling it AI (see `tinyml-model.md` for a real failure case we keep documented).

## What the AI is genuinely doing

- Learns the joint statistical profile of a *specific environment* from data (means, ranges, deltas, stds over time).
- Detects deviations of the *temporal pattern* of multiple sensors — information single-sample threshold logic cannot represent.
- Runs entirely on a 256 KB-SRAM RISC-V MCU as plain C++ arithmetic.

## Status (honest)

- Hardware, firmware pipeline, dataset tooling, first model (Model A) trained & executed on-board: **done**.
- Model quality: **development only** — 107 windows (100 normal/7 abnormal); a documented real flame event was missed (p≈0.34 < 0.5). Next milestone: dataset expansion + retraining with trial-level validation.
- Nothing here is a certified safety device; no real-world accuracy claims are made.

See: [system-architecture.md](system-architecture.md) · [tinyml-model.md](tinyml-model.md) · [dataset-methodology.md](dataset-methodology.md)
