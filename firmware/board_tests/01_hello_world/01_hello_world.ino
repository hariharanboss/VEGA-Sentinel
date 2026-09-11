/*
  VEGA Sentinel bring-up 1/7 - Hello World
  Board: VEGA ARIES v2.0 (Tools > Board > ARIES v2), Serial Monitor 115200.
  Pass criteria: prints once per second.
*/

void setup() {
  Serial.begin(115200);
}

void loop() {
  Serial.println("VEGA Sentinel - ARIES v2.0 alive");
  delay(1000);
}
