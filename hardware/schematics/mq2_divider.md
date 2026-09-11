# Schematic — MQ-2 Voltage Divider

## Why a divider is mandatory

- MQ-2 module AO is a **5 V** domain signal (module powered from 5 V) and
  can swing up to VCC.
- ARIES v2.0 analog inputs (ADS1015, ±4.096 V PGA, 3.3 V system) must not
  see the full 5 V swing.
- An equal-value divider halves the signal: 5 V → 2.5 V max at A0.

## Circuit

```
MQ-2 AO ────[ 2.2 kΩ ]────┬──── ARIES A0
                           │
                        [ 2.2 kΩ ]
                           │
                          GND
```

- Divider ratio: R_low / (R_high + R_low) = 2.2 / (2.2 + 2.2) = **0.5**
- A0 voltage = AO × 0.5 → max 2.5 V. Safe.

## Consequences for data interpretation

- The raw `analogRead(A0)` values (effective 0–2047 single-ended range on
  core C-DAC v1.1.2) reflect **AO ÷ 2**, not the module output directly.
- All training data and all firmware values share this exact arrangement,
  so the scale is internally consistent across training and deployment.
- Values are **raw ADC counts** — **not calibrated ppm**. Never convert to
  concentration without proper calibration against known standards.

## Load note

The divider draws 5 V / 4.4 kΩ ≈ 1.1 mA from the module's AO output —
well within typical op-amp drive capability on MQ-2 module boards, and the
input impedance of the ADS1015 channel is far higher (negligible loading).
