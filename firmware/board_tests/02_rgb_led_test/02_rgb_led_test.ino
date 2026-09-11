/*
  VEGA Sentinel bring-up 2/7 - Onboard RGB LED test
  Board: VEGA ARIES v2.0 (ARIES v2), Serial Monitor 115200.

  Verified onboard RGB LED mapping (ARIES v2.0 datasheet + C-DAC Blink example):
    GPIO22 -> LD1 green
    GPIO23 -> LD1 blue
    GPIO24 -> LD1 red
  Polarity: ACTIVE-LOW. digitalWrite(pin, LOW) = LED ON, HIGH = OFF.

  Pass criteria: distinct R, G, B for 1 s each, then all off.
*/

const uint8_t PIN_RGB_R = 24;
const uint8_t PIN_RGB_G = 22;
const uint8_t PIN_RGB_B = 23;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_RGB_R, OUTPUT);
  pinMode(PIN_RGB_G, OUTPUT);
  pinMode(PIN_RGB_B, OUTPUT);
  Serial.println("RGB LED test start");
}

void loop() {
  Serial.println("RED for 1s");
  setColor(1, 0, 0);
  delay(1000);
  Serial.println("GREEN for 1s");
  setColor(0, 1, 0);
  delay(1000);
  Serial.println("BLUE for 1s");
  setColor(0, 0, 1);
  delay(1000);
  Serial.println("ALL OFF for 1s");
  setColor(0, 0, 0);
  delay(1000);
}

// r/g/b = 1 means channel ON (inverted for active-low LED)
void setColor(int r, int g, int b) {
  digitalWrite(PIN_RGB_R, r ? HIGH : LOW);  // flip if inverted
  digitalWrite(PIN_RGB_G, g ? LOW : HIGH);
  digitalWrite(PIN_RGB_B, b ? LOW : HIGH);
}
