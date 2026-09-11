/*
  VEGA Sentinel bring-up 5/7 - DHT22 read
  Board: VEGA ARIES v2.0 (ARIES v2), Serial Monitor 115200.

  WIRING:
    DHT22 VCC  -> 3.3V
    DHT22 GND  -> GND
    DHT22 DATA -> GPIO25 (J9_7)
    10k pull-up resistor between DATA and 3.3V (omit only if your module
    board already carries one - inspect it).

  IMPORTANT BSP quirk: pinMode() on this core only honors INPUT(0) and
  OUTPUT(1). INPUT_PULLUP(0x2) is silently ignored - there is no internal
  pull-up. An external 10k pull-up on DATA is effectively mandatory.

  This is a self-contained one-wire protocol driver (no library needed):
  MCU pulls line low 1-2 ms, releases, DHT answers with 40 bits
  (16 humidity, 16 temperature, 8 checksum). Pulses are timed with
  micros(); tolerance is generous because we only classify short vs long
  high pulses around a ~50 us midpoint.

  DHT22 constraints: >= 2 s between reads, 0.1C resolution, checksum
  verified. DHT11 would give different byte layout (8.8 integers).

  Pass criteria: temperature tracks room temperature, humidity is
  plausible, checksum errors are rare.
*/

const uint8_t DHT_PIN = 25;   // J9_7

void setup() {
  Serial.begin(115200);
  Serial.println("DHT22 test - GPIO25 (J9_7)");
  Serial.println("status,temp_c,humidity");
}

void loop() {
  float t, h;
  int rc = dht22Read(t, h);
  if (rc == 0) {
    Serial.print("OK,");
    Serial.print(t, 1);
    Serial.print(",");
    Serial.println(h, 1);
  } else {
    Serial.print("ERR_");
    Serial.print(rc);
    Serial.println(",,");
  }
  delay(2500);   // DHT22 needs >= 2 s between reads
}

// Returns 0 on success; error codes: 1 no response, 2 timeout,
// 3 bad bit, 4 checksum mismatch.
int dht22Read(float &tempOut, float &humOut) {
  uint8_t data[5] = {0, 0, 0, 0, 0};

  // Start pulse: output-low 1.2 ms then release
  pinMode(DHT_PIN, OUTPUT);
  digitalWrite(DHT_PIN, LOW);
  delayMicroseconds(1200);
  pinMode(DHT_PIN, INPUT);      // external 10k pulls line high
  delayMicroseconds(30);

  // Sensor response: ~80us low, ~80us high
  if (waitLevel(DHT_PIN, HIGH, 100)) return 1;
  if (waitLevel(DHT_PIN, LOW, 100)) return 1;

  // 40 data bits: ~50us low then high of 26-28us (0) or ~70us (1)
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

// Waits until pin reaches 'level' or 'timeoutUs' elapse.
// Returns 0 if level seen, 1 on timeout.
uint8_t waitLevel(uint8_t pin, uint8_t level, uint32_t timeoutUs) {
  uint32_t start = micros();
  while (digitalRead(pin) != level) {
    if (micros() - start > timeoutUs) return 1;
  }
  return 0;
}
