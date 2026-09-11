# Wiring — Verified Connections

All connections below were physically verified on our VEGA ARIES v2.0 board.
Common GND across all subsystems is mandatory.

## Summary diagram

```
                         ┌──────────────────────────────┐
   DHT22 DATA ──────────►│ GPIO0                         │
                         │                               │
   Flame DO ────────────►│ GPIO1          VEGA ARIES     │
                         │                v2.0           │
                         │ GPIO2 ──────────┐              │
   MQ-2 AO ──2.2k──┬────►│ A0             │              │
                 2.2k    │                1k             │
                  └─GND  │ 3.3V ──► DHT22 VCC, Flame VCC  │
                         │ 5V   ──► MQ-2 VCC              │
                         │ GND  ◄── all grounds           │
                         └───────────────┼───────────────┘
                                         │
                                      2N2222 Base
                                    2N2222 Emitter → GND
                                    2N2222 Collector → Buzzer (−)
                                    Buzzer (+) → external 5 V
```

## DHT22 (temperature + humidity)

| From | To |
|---|---|
| DHT22 VCC | ARIES 3.3 V |
| DHT22 DATA | ARIES GPIO0 |
| DHT22 GND | ARIES GND |

Notes: 3.3 V sensor matches I/O voltage. The DATA line needs a pull-up to
3.3 V (present on our module); the VEGA core's `pinMode()` does not provide
an internal pull-up (`INPUT_PULLUP` is a no-op).

## MQ-2 (gas/smoke-related response, raw ADC)

| From | To |
|---|---|
| MQ-2 VCC | 5 V |
| MQ-2 GND | GND |
| MQ-2 AO | Voltage divider → ARIES A0 |
| MQ-2 DO | **not used** |

Divider (identical 2.2 kΩ resistors):

```
MQ-2 AO ──── 2.2 kΩ ────┬──── ARIES A0
                       2.2 kΩ
                        │
                       GND
```

- A0 sees AO ÷ 2, so a 5 V full-scale module output appears as ≤ 2.5 V —
  inside the ADS1015's ±4.096 V PGA range and below the 3.3 V logic domain.
- `analogRead()` values are **raw ADC counts** (effective 0–2047 single-ended
  on core C-DAC v1.1.2) — **not ppm, not calibrated**.
- The MQ-2 heater warms up over minutes; readings drift until stabilised.

## Flame sensor (3-pin IR module)

| From | To |
|---|---|
| Flame VCC | ARIES 3.3 V |
| Flame GND | ARIES GND |
| Flame DO | ARIES GPIO1 |

**Experimentally verified active-LOW:** DO reads LOW when flame is
detected, HIGH otherwise. Firmware normalises this:
`flame = (digitalRead(PIN_FLAME) == LOW) ? 1 : 0;`

## Buzzer driver (2N2222 + 1 kΩ)

| From | To |
|---|---|
| ARIES GPIO2 | 1 kΩ → 2N2222 Base |
| 2N2222 Emitter | GND (common) |
| 2N2222 Collector | Buzzer negative (−) |
| Buzzer positive (+) | External 5 V |
| External 5 V GND | Common GND with ARIES |

- The GPIO never drives the buzzer directly: ARIES I/O is limited to
  **12 mA**, an active buzzer typically wants 20–30 mA.
- Base current at GPIO HIGH: (3.3 V − 0.7 V) / 1 kΩ ≈ 2.6 mA — safe.
- See `hardware/schematics/buzzer_driver.md` for the design math.
