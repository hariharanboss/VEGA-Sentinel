# Schematic — Buzzer Driver (2N2222)

## Why a driver is mandatory

- ARIES v2.0 GPIO: 3.3 V logic, **12 mA max per I/O**.
- 5 V active buzzer: typically 20–30 mA.
- Therefore the GPIO cannot (and must never) drive the buzzer directly.

## Circuit

```
ARIES GPIO2 ────[ 1 kΩ ]────┬── 2N2222 Base (B)
                            │
                     2N2222 (NPN)
                     E (Emitter) ──── GND (common)
                     C (Collector) ──┬──── Buzzer (−)
                                     │
                            Buzzer (active, 5 V)
                                     │
                          External +5 V rail
                 External 5 V GND ─── Common GND
```

## Operation

| GPIO2 | Transistor | Buzzer |
|---|---|---|
| LOW (0 V) | Cut off | OFF |
| HIGH (3.3 V) | Saturated (base ≈ 2.6 mA) | ON |

## Design math

- Base current: (3.3 V − V_BE 0.7 V) / 1 kΩ ≈ **2.6 mA** — well inside the
  12 mA GPIO limit.
- 2N2222 collector current capability (≥ 100 mA continuous class) covers
  the buzzer's ~30 mA with large margin; with a typical β ≥ 50 the 2.6 mA
  base current comfortably saturates the switch.
- The buzzer is an *active* (internally driven) type — no PWM or tone
  generation needed; a DC level is enough.

## Verified

Physically tested on our ARIES v2.0 build: GPIO2 HIGH → buzzer sounds;
GPIO2 LOW → silent. The ARIES board remained within safe operating limits.
