# dataset/sample/

`sample_session.csv` — a copy of `dataset/raw/sentinel_data_20260911_190322.csv`
(the short flame_test_2 session, 30 rows) demonstrating the exact format:

```csv
laptop_timestamp,timestamp_ms,temp_c,humidity_pct,mq2_adc,flame_state,dht_ok,label,experiment,phase
2026-09-11T19:03:22.796,196299,28.20,69.10,177,0,1,normal,flame_test_2,baseline
...
2026-09-11T19:04:19.936,254001,28.20,69.10,158,1,1,critical,flame_test_2,event
```

Column semantics are documented in [../README.md](../README.md). MQ-2
values are raw ADC counts (0–2047 effective range on this core, after the
2×2.2 kΩ divider) — **not ppm**.
