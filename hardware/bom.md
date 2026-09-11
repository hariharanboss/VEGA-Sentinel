# Bill of Materials — VEGA Sentinel

| # | Component | Qty | Purpose | Voltage | Interface | ARIES v2.0 pin | Notes / external requirements |
|---|---|---|---|---|---|---|---|
| 1 | VEGA ARIES v2.0 board | 1 | Edge-AI platform: acquisition, windowing, inference, alarm logic | 5 V USB / onboard 3.3 V logic | — | — | THEJAS32 RISC-V @ 100 MHz, 256 KB SRAM, 2 MB flash. Programmed via Arduino IDE 1.8.19 (VEGA core C-DAC v1.1.2) |
| 2 | MQ-2 gas/smoke sensor module | 1 | Gas/smoke-related sensor response (raw analog) | 5 V (module), heater always on | Analog (AO) | A0 (via divider) | DO not used. AO passes through 2×2.2 kΩ divider before A0. Warm-up required. **Not a calibrated ppm instrument** |
| 3 | DHT22 temperature/humidity sensor | 1 | Temperature (°C) + relative humidity (%) | 3.3 V | 1-wire digital (bit-banged) | GPIO0 | ≥2 s between reads; checksum-verified driver; external pull-up on DATA required (module provides it) |
| 4 | 3-pin IR flame sensor module | 1 | Flame indication (binary) | 3.3 V | Digital (DO) | GPIO1 | **Active-LOW** (verified): LOW = flame detected |
| 5 | 2N2222 NPN transistor | 1 | Buzzer switching driver — protects the 12 mA-limited GPIO | — | — | (driven via GPIO2) | Emitter→GND, Collector→buzzer(−), Base via 1 kΩ from GPIO2 |
| 6 | Resistor 1 kΩ | 1 | 2N2222 base current limit | — | — | GPIO2 → base | Keeps GPIO current ≈ 2.6 mA (well under 12 mA) |
| 7 | 5 V active buzzer | 1 | Audible alarm output | 5 V (external) | Via 2N2222 | GPIO2 (control) | Positive→external 5 V; negative→2N2222 collector; **never** driven directly by a GPIO |
| 8 | Resistor 2.2 kΩ | 2 | MQ-2 analog voltage divider (halves AO) | — | — | feeds A0 | AO→2.2 kΩ→A0 node→2.2 kΩ→GND; A0 sees AO/2, keeping the 5 V module output inside the ADC's safe range |
| 9 | External 5 V supply | 1 | Buzzer power (independent of ARIES 5 V rail limits) | 5 V | — | — | **Common ground with ARIES GND is mandatory** |
| 10 | USB cable | 1 | Programming + serial data logging | — | USB | — | Serial 115200 baud |

## Design rules encoded in this BOM

1. **No GPIO ever drives the buzzer directly** — 12 mA per-I/O limit on ARIES v2.0; the 2N2222 does the switching.
2. **No 5 V module output ever reaches an ADC pin directly** — the 2×2.2 kΩ divider halves MQ-2 AO before A0.
3. **3.3 V sensors (DHT22, flame) match the ARIES I/O voltage** — no level shifting needed.
4. **Common ground** across ARIES, MQ-2, DHT22, flame sensor, buzzer supply.
