# tools/

## data_logger/ — PC-side serial dataset logger

`sentinel_logger.py` (v0.4) — captures the firmware's CSV rows over serial,
validates each row, adds `laptop_timestamp,label,experiment,phase` columns,
and writes two files: accepted rows and rejected rows.

```bash
pip install -r requirements.txt          # needs pyserial

# Typical session (adjust COM port):
python tools/data_logger/sentinel_logger.py --port COM19 --experiment baseline --label normal

# Interactive controls while logging:
#   n / w / c + Enter   -> label normal / warning / critical
#   b / v / r + Enter   -> phase baseline / event / recovery
#   e <name> + Enter    -> change experiment name
#   q + Enter           -> quit
```

Row validation rules (matching the ARIES v2.0 hardware reality):

- 6 fields: `timestamp_ms,temp_c,humidity_pct,mq2_adc,flame_state,dht_ok`
- `dht_ok=1`: temperature within −40..80 °C, humidity within 0..100 %
- `dht_ok=0`: temp/humidity must equal −999 (the firmware's invalid marker)
- flame must be 0/1
- MQ-2 raw ADC within 0–2047 (ADS1015 single-ended range on this core)

Use together with `firmware/data_collection/07_data_logger/` (or the final
firmware's CSV output lines).

## visualization/

`visualize_dataset.py` — plots one collection session (temperature,
humidity, MQ-2 raw ADC, flame state) with abnormal-labelled regions shaded.

```bash
python tools/visualization/visualize_dataset.py --file dataset/raw/sentinel_data_20260911_182814.csv
python tools/visualization/visualize_dataset.py --file <csv> --no-show --output results/graphs/
```

## Safety reminder

When collecting abnormal-condition data: small, supervised, controlled
stimuli only (e.g. a small candle or incense stick physically separated
from the electronics, ventilated area). Never create dangerous gas
concentrations or uncontrolled fire. MQ-2 readings are raw sensor
responses — not calibrated ppm.
