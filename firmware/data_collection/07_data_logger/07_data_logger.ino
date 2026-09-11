/*
  VEGA Sentinel bring-up 7/7 - CSV data logger for TinyML training
  Board: VEGA ARIES v2.0 (ARIES v2), Serial Monitor 115200.

  WIRING (same as sketches 4-6):
    MQ-2 AO via 2.7k/4.7k divider -> A0 (J10_2)
    DHT22 DATA (10k pull-up to 3.3V) -> GPIO25 (J9_7)

  Logs one CSV row per period to Serial. Capture with a serial logger
  (e.g. PuTTY logging, or Arduino IDE select-all+copy after a session)
  into data/normal.csv, data/warning.csv, data/critical.csv etc.

  Suggested protocol (one file per session):
    normal    - plain room environment, sensor warmed up, 30+ min
    warning   - mild controlled stimulus (e.g. warm hair-dryer air from
                a distance, brief alcohol swab near MQ-2), repeated
    critical  - stronger sustained stimulus (hair-dryer close, longer
                gas exposure) - always safe, ventilated, supervised
    recovery  - post-stimulus return to normal (also label normal)

  Label column is a single character (n/w/c) typed INTO the serial
  monitor BEFORE the session; it is echoed into every row. Default n.
  Keep labels consistent: anomaly states w/c only while stimulus active.

  DHT read failure policy during logging: row is written with the error
  code in place of values. Post-process to drop or interpolate.

  Row format:
    ms,mq2_counts,temp_c,hum,label
*/

const uint8_t MQ2_ADC  = A0;   // J10_2 / ADC_CH0
const uint8_t DHT_PIN  = 25;   // J9_7
const uint32_t PERIOD_MS = 2500;

char label = 'n';

void setup() {
  Serial.begin(115200);
  Serial.println("VEGA Sentinel data logger");
  Serial.println("Type n / w / c in serial monitor to set label");
  Serial.println("ms,mq2_counts,temp_c,hum,label");
}

void loop() {
  // Optional label update from serial monitor
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'n' || c == 'w' || c == 'c') {
      label = c;
      Serial.print("# label=");
      Serial.println(label);
    }
  }

  static uint32_t last = 0;
  uint32_t now = millis();
  if (now - last < PERIOD_MS) return;
  last = now;

  const uint8_t N = 8;
  uint32_t sum = 0;
  for (uint8_t i = 0; i < N; i++) {
    sum += analogRead(MQ2_ADC);
    delay(5);
  }
  uint16_t mq2Counts = sum / N;

  float t, h;
  int rc = dht22Read(t, h);

  Serial.print(now);
  Serial.print(",");
  Serial.print(mq2Counts);
  Serial.print(",");
  if (rc == 0) {
    Serial.print(t, 1);
    Serial.print(",");
    Serial.print(h, 1);
  } else {
    Serial.print("ERR_");
    Serial.print(rc);
    Serial.print(",");
    Serial.print(0);
  }
  Serial.print(",");
  Serial.println(label);
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
