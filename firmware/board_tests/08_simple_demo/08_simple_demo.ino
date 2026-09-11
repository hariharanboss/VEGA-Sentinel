/*
  VEGA Sentinel - Simple onboard demo
  Board: VEGA ARIES v2.0 (Tools > Board > ARIES v2), Serial Monitor 115200.
  No external wiring required - uses only onboard RGB LED + push buttons.

  Hardware facts (verified from ARIES v2.0 datasheet + official BSP examples):
    GPIO24 -> Red LED    (ACTIVE-LOW: LOW = ON, HIGH = OFF)
    GPIO22 -> Green LED  (ACTIVE-LOW)
    GPIO23 -> Blue LED   (ACTIVE-LOW)
    GPIO19 -> BTN0       (reads LOW when pressed)
    GPIO18 -> BTN1       (reads LOW when pressed)
    Yellow LEDs GPIO20/21 are ACTIVE-HIGH (not used here).

  What it does (sentinel idle behavior):
    - GREEN LED breathes (pwm.cpp fade) to show "all normal".
    - BTN0 pressed  -> BLUE blinks 3x, serial logs "CHECK-IN acknowledged".
    - BTN1 held     -> RED steady + serial "SIMULATED ALERT" until released.
    - Prints uptime every 5 s.
*/

const uint8_t PIN_R = 24;
const uint8_t PIN_G = 22;
const uint8_t PIN_B = 23;
const uint8_t BTN0  = 19;
const uint8_t BTN1  = 18;

uint32_t lastUptimeMs = 0;
uint32_t lastBtn0Ms  = 0;
bool inAlert = false;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  pinMode(BTN0, INPUT);
  pinMode(BTN1, INPUT);
  allOff();
  Serial.println("VEGA Sentinel simple demo - onboard hardware only");
}

void loop() {
  uint32_t now = millis();

  // --- BTN1: simulated alert (level-triggered) ---
  if (digitalRead(BTN1) == LOW) {
    if (!inAlert) {
      inAlert = true;
      Serial.println("SIMULATED ALERT - BTN1 held");
    }
    red(true);
    green(false);
    return;                    // alert overrides everything else
  }
  if (inAlert) {
    inAlert = false;
    Serial.println("Alert cleared");
    allOff();
  }

  // --- BTN0: check-in (edge-triggered, 250 ms debounce) ---
  if (digitalRead(BTN0) == LOW && now - lastBtn0Ms > 250) {
    lastBtn0Ms = now;
    Serial.println("CHECK-IN acknowledged");
    blinkBlue(3);
  }

  // --- Green heartbeat every 5 s ---
  if (now - lastUptimeMs >= 5000) {
    lastUptimeMs = now;
    Serial.print("uptime_s,");
    Serial.println(now / 1000);
    pulseGreen();
  }

  delay(10);
}

// ---- LED helpers (active-low LED: LOW = ON) ----
void allOff()   { red(false); green(false); blue(false); }
void red(bool on)    { digitalWrite(PIN_R, on ? LOW : HIGH); }
void green(bool on)  { digitalWrite(PIN_G, on ? LOW : HIGH); }
void blue(bool on)   { digitalWrite(PIN_B, on ? LOW : HIGH); }

void blinkBlue(uint8_t n) {
  for (uint8_t i = 0; i < n; i++) {
    blue(true);  delay(120);
    blue(false); delay(120);
  }
}

void pulseGreen() {
  green(true);
  delay(80);
  green(false);
}
