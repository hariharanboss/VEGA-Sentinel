# Hardware — VEGA ARIES v2.0 Platform Notes

## Board identity

**VEGA ARIES v2.0** — developed by C-DAC. **Do not confuse with ARIES IoT v2.0.**

Verified specifications (official documentation):

| Item | Value |
|---|---|
| Processor | THEJAS32 / VEGA ET1031 (RISC-V) |
| Clock | 100 MHz |
| SRAM | 256 KB |
| Flash | 2 MB |
| I/O voltage | 3.3 V |
| Max I/O current | 12 mA per I/O |
| Analog inputs | 4 (A0–A3) |
| Arduino IDE | 1.8.19 (VEGA core C-DAC v1.1.2 used in development) |
| Serial baud | 115200 |

Official sources:

- C-DAC product page: https://www.cdac.in/index.aspx?id=product_details&productId=ARIESv2.0
- VEGA Processors: https://vegaprocessors.in/ariesv2.php
- Datasheet: https://vegaprocessors.in/files/ARIESv2%200_Datasheet_v2.pdf
- Pinout: https://vegaprocessors.in/files/PINOUT_ARIES%20V2.0_.pdf

## Development environment setup

1. Install Arduino IDE 1.8.19.
2. Preferences → Additional Board Manager URLs:
   `https://gitlab.com/riscv-vega/vega-arduino/-/raw/main/package_vega_index.json`
3. Boards Manager → install **VEGA ARIES Boards**.
4. Tools → Board → **VEGA Processor: ARIES Boards → ARIES v2**.
5. Select the board's COM port; Serial Monitor 115200.
6. J12 (BOOT-SEL): shorted = Flash Mode Enabled / Programmer "VEGA FLASHER"; open = VEGA XMODEM (per C-DAC documentation).

## Analog inputs — what analogRead() really returns

On ARIES v2/v3/micro with the VEGA core (C-DAC v1.1.2), `analogRead(A0..A3)`
is served by the **onboard ADS1015** (I²C, 12-bit, ±4.096 V PGA). In our
single-ended testing the effective reading is approximately **0–2047**
(not 0–4095). All MQ-2 data in this repository is raw ADC counts in this
range, after the 2×2.2 kΩ divider — **never** treat them as calibrated
concentration (ppm).

## GPIO behaviour verified on this core

- `pinMode()` honours INPUT(0)/OUTPUT(1) only; **`INPUT_PULLUP` is a no-op** → external pull-ups are required (DHT22 DATA needs one; our module provides it).
- Onboard RGB LED (LD1): GPIO24=R, GPIO22=G, GPIO23=B — **active-low**; yellow LEDs GPIO20/21 are active-high (not used).
- Onboard buttons BTN0/BTN1 on GPIO19/18 read LOW when pressed (used only in board tests).
- `millis()`, `micros()`, `delayMicroseconds()` available (mcycle-based) — used by the bit-banged DHT22 driver.

## Constraints that shaped the design

| Constraint | Consequence |
|---|---|
| 12 mA per I/O | Buzzer switched via 2N2222; GPIO2 only sources ~2.6 mA base current |
| 3.3 V I/O, 5 V MQ-2 module | 2×2.2 kΩ divider halves AO before A0 |
| 256 KB SRAM / 2 MB flash | Static buffers only; 7-feature linear model (~100 bytes); no framework |
| No verified TFLite Micro/EI support on ARIES v2.0 | Direct C/C++ inference (scaler + dot product + sigmoid) |

## Safety constraints for experiments

- MQ-2 heater is always on; the sensor gets warm — normal.
- Use only small, supervised, controlled stimuli, physically separated from electronics; ventilated area.
- Never create dangerous gas concentrations, uncontrolled fire, or hazardous atmospheres.
- Common ground across ARIES, sensors, and the buzzer's external 5 V supply is mandatory.

## External hardware

Full connection tables: [sensor-wiring.md](sensor-wiring.md) · BOM: [../hardware/bom.md](../hardware/bom.md) · Schematics: [../hardware/schematics/](../hardware/schematics/)
