# hardware/

Verified hardware documentation for the VEGA Sentinel final configuration.

> ⚠️ Board identity: **VEGA ARIES v2.0** (THEJAS32/VEGA ET1031). **Not** ARIES IoT v2.0.

## Contents

| File / folder | Contents |
|---|---|
| `bom.md` | Bill of materials (component, qty, purpose, voltage, interface, ARIES pin, notes) |
| `wiring/connections.md` | Complete connection tables for every subsystem |
| `schematics/buzzer_driver.md` | 2N2222 buzzer driver schematic + design math |
| `schematics/mq2_divider.md` | MQ-2 voltage divider schematic + design math |

## ON ARIES vs EXTERNAL components

**ON ARIES v2.0 (the platform):**
- THEJAS32 / VEGA ET1031 RISC-V processor @ 100 MHz
- 256 KB SRAM / 2 MB flash
- GPIO (3.3 V logic, 12 mA max per I/O)
- 4 analog inputs A0–A3 via onboard ADS1015 (effective single-ended read 0–2047 in our tests, core C-DAC v1.1.2)
- Power interfaces: 5 V and 3.3 V rails, GND, USB (programming + serial)
- Onboard RGB LED (GPIO24=R, 22=G, 23=B, active-low) — used only during board tests, not part of the final sensor set

**EXTERNAL (wired to ARIES):**
- MQ-2 gas/smoke sensor module (5 V)
- DHT22 temperature/humidity sensor (3.3 V)
- 3-pin IR flame sensor module (3.3 V, active-LOW DO)
- 2N2222 NPN transistor (buzzer driver)
- 1 kΩ resistor (transistor base)
- 5 V active buzzer (from external 5 V supply)
- Two 2.2 kΩ resistors (MQ-2 analog voltage divider)
- External 5 V supply for the buzzer (common GND with ARIES)

**Explicitly not part of the final hardware:** LCD, relay, any additional sensors. Do not reintroduce them unless explicitly requested.
