/*
  VEGA Sentinel bring-up 4/7 - MQ-2 raw sensor read
  Board: VEGA ARIES v2.0 (ARIES v2), Serial Monitor 115200.

  WIRING (voltage divider MANDATORY before connecting):
    MQ-2 VCC -> 5V   (module is typically a 5V module - verify yours)
    MQ-2 GND -> GND  (common ground with the board)
    MQ-2 AO  -> 2.7k resistor -> junction -> A0 (J10_2)
                        junction -> 4.7k resistor -> GND
    MQ-2 DO  -> leave unconnected for now

  The divider scales AO by 4.7/(2.7+4.7) = 0.635, so a 5.0 V AO appears
  as ~3.17 V at A0 - safely inside the ADS1015 +/-4.096 V range. This is
  the same divider recommended by C-DAC's own MQ-series examples.
  DO NOT connect a 5 V module AO directly to A0.

  MQ-2 notes:
    - Heater needs warm-up: readings drift for the first minutes (or
      longer on a brand-new sensor). Note stabilized clean-air value.
    - Raw ADS1015 counts are what the TinyML pipeline will consume; ppm
      conversion is NOT needed for anomaly detection.

  Pass criteria: value stabilizes to a steady clean-air baseline after
  warm-up, and visibly rises when a controlled stimulus (e.g. alcohol
  swab or unlit lighter gas from a safe distance) is brought near.
*/

const uint8_t MQ2_ADC = A0;   // J10_2 / ADC_CH0
const uint8_t N_SAMPLES = 8;

void setup() {
  Serial.begin(115200);
  Serial.println("MQ-2 raw read (ADS1015 counts via divider on A0)");
  Serial.println("Keep area ventilated. Sensor warm-up takes minutes.");
  Serial.println("count_avg,mv_at_adc,mv_at_module_ao");
}

void loop() {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < N_SAMPLES; i++) {
    sum += analogRead(MQ2_ADC);
    delay(5);
  }
  float avg = sum / (float)N_SAMPLES;
  float mvAdc = avg * 4096.0 / 4095.0;
  float mvModule = mvAdc * (2.7 + 4.7) / 4.7;   // undo divider ratio

  Serial.print((long)avg);
  Serial.print(",");
  Serial.print((long)mvAdc);
  Serial.print(",");
  Serial.println((long)mvModule);
  delay(500);
}
