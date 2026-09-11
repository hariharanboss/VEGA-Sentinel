/*
  VEGA Sentinel bring-up 3/7 - ADC test
  Board: VEGA ARIES v2.0 (ARIES v2), Serial Monitor 115200.

  On ARIES v2.0, analogRead(A0..A3) talks to the onboard ADS1015 ADC over
  I2C (12-bit, PGA +/-4.096 V full-scale range). Board connections:
    A0 -> J10_2 (ADC_CH0)
    A1 -> J10_4 (ADC_CH1)
    A2 -> J10_6 (ADC_CH2)
    A3 -> J10_8 (ADC_CH3)

  Test WITHOUT any sensor first: leave J10 pins floating or connect a
  known voltage through a divider. Print raw counts and millivolts.

  volts = counts / 4095 * 4096 mV ... use 4.096 V full scale:
    mv = counts * 4096.0 / 4095.0

  Pass criteria: readings are stable (low jitter) and track a known input.
*/

const uint8_t ADC_PINS[4] = {A0, A1, A2, A3};

void setup() {
  Serial.begin(115200);
  Serial.println("ADC test - reading A0..A3 (ADS1015, 12-bit, 4.096V FS)");
  Serial.println("counts_avg,mv_avg,counts_min,mv_min,counts_max,mv_max");
}

void loop() {
  for (uint8_t ch = 0; ch < 4; ch++) {
    const uint8_t N = 16;
    uint32_t sum = 0;
    uint16_t mn = 4095, mx = 0;
    for (uint8_t i = 0; i < N; i++) {
      uint16_t raw = analogRead(ADC_PINS[ch]);
      sum += raw;
      if (raw < mn) mn = raw;
      if (raw > mx) mx = raw;
      delay(2);
    }
    uint16_t avg = sum / N;
    float mvAvg = avg * 4096.0 / 4095.0;
    float mvMin = mn * 4096.0 / 4095.0;
    float mvMax = mx * 4096.0 / 4095.0;

    Serial.print("A");
    Serial.print(ch);
    Serial.print(",");
    Serial.print(avg);
    Serial.print(",");
    Serial.print((long)mvAvg);
    Serial.print(",");
    Serial.print(mn);
    Serial.print(",");
    Serial.print((long)mvMin);
    Serial.print(",");
    Serial.print(mx);
    Serial.print(",");
    Serial.println((long)mvMax);
  }
  Serial.println("---");
  delay(1000);
}
