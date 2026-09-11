# Sensor Wiring — Verified on ARIES v2.0

> All connections below were physically verified on our build. Common GND across all subsystems is mandatory.

## DHT22 (temperature + humidity)

| DHT22 pin | Connects to |
|---|---|
| VCC | ARIES 3.3 V |
| DATA | ARIES **GPIO0** |
| GND | ARIES GND |

- 3.3 V sensor → matches ARIES I/O voltage, no level shifting.
- DATA requires a pull-up to 3.3 V (present on our module). The VEGA core's `pinMode()` ignores `INPUT_PULLUP` — there is no internal pull-up.
- Driven by the known-good bit-banged driver (see `firmware/sensor_tests/05_dht22_test/`); ≥2 s between reads; checksum-verified.

## MQ-2 (gas/smoke-related sensor response)

| MQ-2 pin | Connects to |
|---|---|
| VCC | 5 V |
| GND | GND |
| AO | voltage divider → ARIES **A0** |
| DO | **not used** |

Divider (two 2.2 kΩ):

```
MQ-2 AO ──── 2.2 kΩ ────┬──── ARIES A0
                       2.2 kΩ
                        │
                       GND
```

- A0 sees AO ÷ 2 → max ~2.5 V. Never connect the 5 V module AO directly to an ADC pin.
- Raw `analogRead` counts (effective 0–2047 single-ended on core C-DAC v1.1.2); **not ppm**.
- Allow warm-up minutes after power-on before collecting training data.

## IR flame sensor (3-pin module)

| Flame pin | Connects to |
|---|---|
| VCC | ARIES 3.3 V |
| GND | ARIES GND |
| DO | ARIES **GPIO1** |

**Verified active-LOW:** DO = LOW when flame detected, HIGH otherwise.
Firmware normalises: `flame = (digitalRead(PIN_FLAME) == LOW) ? 1 : 0;`

## Buzzer (5 V active) via 2N2222

```
ARIES GPIO2 ──── 1 kΩ ──── 2N2222 Base
                         2N2222 Emitter ──── GND (common)
                         2N2222 Collector ── buzzer (−)
                         buzzer (+) ──────── external 5 V
                         external 5 V GND ── common GND
```

- The GPIO **never** drives the buzzer directly (12 mA I/O limit vs ~30 mA buzzer current).
- GPIO2 HIGH → transistor saturated → buzzer ON.

## Full pin map (final system)

| ARIES v2.0 pin | Function |
|---|---|
| GPIO0 | DHT22 DATA |
| GPIO1 | Flame sensor DO (active-LOW) |
| GPIO2 | Buzzer driver control (via 1 kΩ → 2N2222) |
| A0 | MQ-2 AO via 2×2.2 kΩ divider |
| 3.3 V | DHT22 VCC, Flame VCC |
| 5 V | MQ-2 VCC |
| GND | Common ground (board, sensors, buzzer supply) |
