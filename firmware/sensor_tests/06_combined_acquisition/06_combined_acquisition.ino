/*
  VEGA Sentinel bring-up 6/7 - Combined acquisition
  Board: VEGA ARIES v2.0 (ARIES v2), Serial Monitor 115200.

  WIRING (same as sketches 4 and 5):
    MQ-2 AO via 2.7k/4.7k divider -> A0 (J10_2)
    DHT22 DATA (10k pull-up to 3.3V) -> GPIO25 (J9_7)

  Reads both sensors on a common 2.5 s cadence (DHT22 minimum is 2 s) and
  prints one aligned row. This is the acquisition core that the data
  logger (sketch 7) and the final firmware will reuse.

  DHT read failure policy for bring-up: print ERR row so failures are
  visible during testing (fault-handling policy for production is not
  finalized - do not silently substitute values yet).

  Pass criteria: aligned rows every 2.5 s with all three values; MQ-2
  stable after warm-up; DHT checksum errors rare.
*/

const uint8_t MQ2_ADC  = A0;   // J10_2 / ADC_CH0
const uint8_t DHT_PIN  = 25;   // J9_7
const uint32_t PERIOD_MS = 2500;

void setup() {
  Serial.begin(115200);
  Serial.println("VEGA Sentinel combined acquisition");
  Serial.println("uptime_ms,mq2_counts,mq2_mv,temp_c,humidity");
}

void loop() {
  static uint32_t last = 0;
  uint32_t now = millis();
  if (now - last < PERIOD_MS) return;
  last = now;

  // MQ-2: average a small burst to smooth single-sample noise
  const uint8_t N = 8;
  uint32_t sum = 0;
  for (uint8_t i = 0; i < N; i++) {
    sum += analogRead(MQ2_ADC);
    delay(5);
  }
  float mq2Counts = sum / (float)N;
  float mq2Mv = mq2Counts * 4096.0 / 4095.0;

  // DHT22
  float t, h;
  int rc = dht22Read(t, h);

  Serial.print(now);
  Serial.print(",");
  Serial.print((long)mq2Counts);
  Serial.print(",");
  Serial.print((long)mq2Mv);
  Serial.print(",");
  if (rc == 0) {
    Serial.print(t, 1);
    Serial.print(",");
    Serial.println(h, 1);
  } else {
    Serial.print("ERR_");
    Serial.print(rc);
    Serial.print(",");
    Serial.println("");
  }
}

// ---- DHT22 one-wire driver (same as sketch 05) ----

int dht22Read(float &tempOut, float &humOut) {
  uint8_t data[5] = {0, 0, 0, 0, 0};

  pinMode(DHT_PIN, OUTPUT);
  digitalWrite(DHT_PIN, LOW);
  delayMicroseconds(1200);
  pinMode(DHT_PIN, INPUT);
  delayMicroseconds(30);

  if (waitLevel(DHT_PIN, HIGH, 100)) return 1;
  if (waitLevel(DHT_PIN, LOW, 100)) return 1;

  for (uint8_t i = 0; i < 40; i++) {
    if (waitLevel(DHT_PIN, HIGH, 80)) return 2;
    uint32_t t0 = micros();
    if (waitLevel(DHT_PIN, LOW, 100)) return 2;
    uint32_t highLen = micros() - t0;
    if (highLen > 1000) return 3;
    data[i / 8] <<= 1;
    if (highLen > 50) data[i / 8] |= 1;
  }

  uint8_t sum = data[0] + data[1] + data[2] + data[3];
  if (sum != data[4]) return 4;

  humOut  = ((data[0] << 8) | data[1]) * 0.1f;
  int16_t rawT = (data[2] << 8) | data[3];
  tempOut = (rawT & 0x8000) ? -(rawT & 0x7FFF) * 0.1f : rawT * 0.1f;
  return 0;
}

uint8_t waitLevel(uint8_t pin, uint8_t level, uint32_t timeoutUs) {
  uint32_t start = micros();
  while (digitalRead(pin) != level) {
    if (micros() - start > timeoutUs) return 1;
  }
  return 0;
}
