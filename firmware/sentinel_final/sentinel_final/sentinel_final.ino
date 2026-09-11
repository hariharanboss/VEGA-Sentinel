// ============================================================
// VEGA SENTINEL - FINAL FIRMWARE
// Edge-AI Multi-Sensor Anomaly Detection for Enclosed Space
// Safety
//
// Board: VEGA ARIES v2.0 (THEJAS32 RISC-V, 100 MHz)
//   Tools > Board > VEGA Processor: ARIES Boards > ARIES v2
//   Serial Monitor: 115200 baud
//
// ------------------------------------------------------------
// HARDWARE / PIN ASSIGNMENTS (verified wiring)
// ------------------------------------------------------------
//   DHT22 DATA -> GPIO0   (VCC=3.3V, GND->GND)
//   Flame DO   -> GPIO1   (VCC=3.3V; module is ACTIVE-LOW:
//                          LOW = flame detected)
//   MQ-2 AO    -> A0      via 2x 2.2k voltage divider
//                          (VCC=5V, GND->GND, DO unused)
//   Buzzer     -> GPIO2   via 1k -> 2N2222 base;
//                          emitter->GND; collector->buzzer(-);
//                          buzzer(+) -> external 5V.
//                          The GPIO NEVER drives the buzzer
//                          directly (12 mA max per I/O).
//
// ------------------------------------------------------------
// PIPELINE (every 2500 ms)
// ------------------------------------------------------------
//   1. Read sensors into TEMPORARY variables.
//   2. Validate the DHT22 sample (checksum).
//   3. Only if valid: shift the 10-sample window,
//      insert the sample, compute 7 features,
//      run Model A, update the confirmation state.
//   4. If INVALID: discard the sample. The window is NOT
//      shifted, no inference runs, alarm state preserved.
//      (This is the fixed version of the earlier firmware
//      which shifted the window before validity check.)
//
//   Temporal confirmation (deliberate, anti-noise):
//     2 consecutive ABNORMAL windows -> buzzer ON
//     3 consecutive NORMAL windows   -> buzzer OFF
//
// ------------------------------------------------------------
// MODEL
// ------------------------------------------------------------
//   Model A parameters live in model.h and are generated
//   by ml/deployment/export_model_to_cpp.py. Inference is
//   direct C/C++ (scaler + dot product + sigmoid). No
//   TinyML framework - none verified on ARIES v2.0.
//
//   MQ-2 values are RAW ADC counts (ADS1015 single-ended,
//   effective 0-2047 on this core). NOT ppm.
// ============================================================

#include "model.h"

// ---------------- Pin assignments ----------------
const uint8_t PIN_DHT22   = 0;    // DHT22 DATA
const uint8_t PIN_FLAME   = 1;    // Flame sensor DO (active LOW)
const uint8_t PIN_BUZZER  = 2;    // 2N2222 base via 1k resistor
const uint8_t PIN_MQ2     = A0;   // MQ-2 AO via divider

// ---------------- Timing ----------------
const uint32_t SAMPLE_INTERVAL_MS = 2500;  // DHT22 needs >= 2 s
const uint32_t DHT_RETRY_DELAY_MS = 2500;  // wait before retry

// ---------------- Sliding window ----------------
const uint8_t WINDOW_SIZE = 10;            // 10 samples ~ 22.5 s
const uint8_t NUM_FEATURES = MODEL_NUM_FEATURES;

// Circular buffers for the current window
float winTemp[WINDOW_SIZE];
float winHumidity[WINDOW_SIZE];
float winMq2[WINDOW_SIZE];
uint8_t winFlame[WINDOW_SIZE];
uint8_t windowCount = 0;   // valid samples collected so far

// ---------------- AI decision state ----------------
uint8_t consecutiveAbnormal = 0;
uint8_t consecutiveNormal = 0;
bool alarmActive = false;

// ---------------- DHT22 raw data ----------------
uint8_t dhtData[5];

// ============================================================
// DHT22 bit-banged one-wire driver (verified on ARIES v2.0)
// ============================================================

// Wait until the DHT22 data pin reaches 'level' or timeout.
bool waitForLevel(int level, unsigned long timeout_us)
{
    unsigned long start = micros();

    while (digitalRead(PIN_DHT22) != level)
    {
        if ((micros() - start) > timeout_us)
            return false;
    }

    return true;
}

// Read 40 bits from the DHT22 into dhtData[].
// Returns true only when the checksum is valid.
bool readDHT22()
{
    for (int i = 0; i < 5; i++)
        dhtData[i] = 0;

    // MCU start signal: pull low >= 1 ms, release
    pinMode(PIN_DHT22, OUTPUT);
    digitalWrite(PIN_DHT22, LOW);
    delay(2);
    digitalWrite(PIN_DHT22, HIGH);
    delayMicroseconds(30);
    pinMode(PIN_DHT22, INPUT);

    // Sensor response: low, high, low
    if (!waitForLevel(LOW, 200))  return false;
    if (!waitForLevel(HIGH, 200)) return false;
    if (!waitForLevel(LOW, 200))  return false;

    // 40 data bits: long HIGH pulse = 1, short = 0
    for (int i = 0; i < 40; i++)
    {
        if (!waitForLevel(HIGH, 100)) return false;

        unsigned long start = micros();
        if (!waitForLevel(LOW, 150))  return false;
        unsigned long highTime = micros() - start;

        dhtData[i / 8] <<= 1;
        if (highTime > 40)
            dhtData[i / 8] |= 1;
    }

    // Checksum: sum of first 4 bytes must equal the 5th
    uint8_t checksum =
        dhtData[0] + dhtData[1] + dhtData[2] + dhtData[3];

    return (checksum == dhtData[4]);
}

// Convert last raw DHT22 bytes to temperature (Celsius).
float getTemperature()
{
    uint16_t rawTemperature =
        ((uint16_t)dhtData[2] << 8) | dhtData[3];

    if (rawTemperature & 0x8000)
    {
        rawTemperature &= 0x7FFF;
        return -(rawTemperature / 10.0f);
    }

    return rawTemperature / 10.0f;
}

// Convert last raw DHT22 bytes to relative humidity (%).
float getHumidity()
{
    uint16_t rawHumidity =
        ((uint16_t)dhtData[0] << 8) | dhtData[1];

    return rawHumidity / 10.0f;
}

// ============================================================
// FEATURE EXTRACTION over the 10-sample window
// ============================================================

// Sample standard deviation (n-1), matching pandas .std()
// used during offline feature generation.
float calculateStd(float values[], int n)
{
    if (n < 2)
        return 0.0f;

    float mean = 0.0f;
    for (int i = 0; i < n; i++)
        mean += values[i];
    mean /= n;

    float sumSquared = 0.0f;
    for (int i = 0; i < n; i++)
    {
        float difference = values[i] - mean;
        sumSquared += difference * difference;
    }

    return sqrtf(sumSquared / (n - 1));
}

// Compute the 7 Model A features from the current window.
void computeFeatures(float features[])
{
    float tempMean = 0.0f;
    float humidityMean = 0.0f;

    float mq2Min = winMq2[0];
    float mq2Max = winMq2[0];
    int flameCount = 0;

    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        tempMean     += winTemp[i];
        humidityMean += winHumidity[i];

        if (winMq2[i] < mq2Min) mq2Min = winMq2[i];
        if (winMq2[i] > mq2Max) mq2Max = winMq2[i];

        flameCount += winFlame[i];
    }

    tempMean     /= WINDOW_SIZE;
    humidityMean /= WINDOW_SIZE;

    // Feature order must match model.h
    features[0] = humidityMean;                       // humidity_mean
    features[1] = tempMean;                           // temp_mean
    features[2] = mq2Max - mq2Min;                   // mq2_range
    features[3] = winMq2[WINDOW_SIZE-1] - winMq2[0];  // mq2_delta
    features[4] = calculateStd(winMq2, WINDOW_SIZE);  // mq2_std
    features[5] =
        calculateStd(winHumidity, WINDOW_SIZE);       // humidity_std
    features[6] =
        (float)flameCount / (float)WINDOW_SIZE;       // flame_fraction
}

// ============================================================
// MODEL A INFERENCE (direct C/C++, no framework)
// ============================================================

// Returns P(abnormal) via standardisation + linear score +
// sigmoid, using the constants in model.h.
float modelPredict(float features[])
{
    float score = MODEL_INTERCEPT;

    for (int i = 0; i < MODEL_NUM_FEATURES; i++)
    {
        float standardized =
            (features[i] - MODEL_MEANS[i]) / MODEL_SCALES[i];

        score += MODEL_COEFFS[i] * standardized;
    }

    // Sigmoid - numerically safe form for large |score|
    if (score > 30.0f)  return 1.0f;
    if (score < -30.0f) return 0.0f;

    return 1.0f / (1.0f + expf(-score));
}

// ============================================================
// SAFETY CONFIRMATION LOGIC
// ============================================================

// Feed one window classification into the temporal
// confirmation state machine and update the buzzer.
void processWindowClassification(bool abnormal)
{
    if (abnormal)
    {
        consecutiveAbnormal++;
        consecutiveNormal = 0;

        // 2 consecutive abnormal windows -> alarm ON
        if (consecutiveAbnormal >= 2 && !alarmActive)
        {
            alarmActive = true;
            Serial.println(">>> ALARM ACTIVATED (2 consecutive ABNORMAL)");
        }
    }
    else
    {
        consecutiveNormal++;
        consecutiveAbnormal = 0;

        // 3 consecutive normal windows -> alarm OFF
        if (consecutiveNormal >= 3 && alarmActive)
        {
            alarmActive = false;
            Serial.println(">>> ALARM CLEARED (3 consecutive NORMAL)");
        }
    }

    digitalWrite(PIN_BUZZER, alarmActive ? HIGH : LOW);
}

// ============================================================
// WINDOW MANAGEMENT
// ============================================================

// Shift the window one position left and insert the newest
// valid sample. Called only AFTER the sample was validated.
void shiftAndInsert(float temp, float humidity,
                    float mq2, uint8_t flame)
{
    for (int i = 0; i < WINDOW_SIZE - 1; i++)
    {
        winTemp[i]     = winTemp[i + 1];
        winHumidity[i] = winHumidity[i + 1];
        winMq2[i]      = winMq2[i + 1];
        winFlame[i]    = winFlame[i + 1];
    }

    winTemp[WINDOW_SIZE - 1]     = temp;
    winHumidity[WINDOW_SIZE - 1] = humidity;
    winMq2[WINDOW_SIZE - 1]      = mq2;
    winFlame[WINDOW_SIZE - 1]    = flame;
}

// ============================================================
// SETUP
// ============================================================

void setup()
{
    Serial.begin(115200);
    delay(2000);

    pinMode(PIN_FLAME, INPUT);
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);

    Serial.println();
    Serial.println("========================================");
    Serial.println("        VEGA SENTINEL - FINAL SYSTEM");
    Serial.println("     VEGA ARIES v2.0 | Model A (dev)");
    Serial.println("========================================");
    Serial.println("DHT22->GPIO0  Flame->GPIO1  MQ2->A0  Buzzer->GPIO2 (2N2222)");
    Serial.println();
    Serial.println("timestamp_ms,temp_c,humidity_pct,mq2_adc,flame_state,dht_ok");

    // Pre-fill the window with zeros; windowCount tracks how
    // many real samples we have until the window is full.
    windowCount = 0;
}

// ============================================================
// MAIN LOOP - one acquisition cycle per SAMPLE_INTERVAL_MS
// ============================================================

void loop()
{
    static uint32_t lastSampleMs = 0;
    uint32_t now = millis();

    if (now - lastSampleMs < SAMPLE_INTERVAL_MS)
        return;
    lastSampleMs = now;

    // --------------------------------------------------------
    // 1. READ ALL SENSORS INTO TEMPORARY VARIABLES
    //    (nothing is written into the window yet)
    // --------------------------------------------------------
    bool dhtOK = readDHT22();

    float temperature = 0.0f;
    float humidity = 0.0f;

    if (dhtOK)
    {
        temperature = getTemperature();
        humidity = getHumidity();
    }

    int mq2 = analogRead(PIN_MQ2);          // raw ADC, 0-2047

    int flameRaw = digitalRead(PIN_FLAME);  // module is active LOW
    uint8_t flame = (flameRaw == LOW) ? 1 : 0;

    // Always emit the CSV row (matches dataset format)
    Serial.print(now);
    Serial.print(",");
    Serial.print(dhtOK ? temperature : -999.0f, 2);
    Serial.print(",");
    Serial.print(dhtOK ? humidity : -999.0f, 2);
    Serial.print(",");
    Serial.print(mq2);
    Serial.print(",");
    Serial.print(flame);
    Serial.print(",");
    Serial.println(dhtOK ? 1 : 0);

    // --------------------------------------------------------
    // 2. VALIDATE: if DHT22 failed, DISCARD the sample.
    //    Window NOT shifted. No inference. Alarm state kept.
    // --------------------------------------------------------
    if (!dhtOK)
    {
        Serial.println("DHT22 invalid - sample discarded (window preserved)");
        return;
    }

    // Plausibility check on DHT22 values (guard against
    // checksum-passing garbage)
    if (temperature < -40.0f || temperature > 80.0f ||
        humidity < 0.0f   || humidity > 100.0f)
    {
        Serial.println("DHT22 out of range - sample discarded");
        return;
    }

    // --------------------------------------------------------
    // 3. VALID SAMPLE: shift window, insert, maybe infer
    // --------------------------------------------------------
    shiftAndInsert(temperature, humidity, (float)mq2, flame);
    windowCount++;

    if (windowCount < WINDOW_SIZE)
    {
        // Still filling the initial window
        return;
    }

    // --------------------------------------------------------
    // 4. WINDOW FULL: extract features and run the AI
    // --------------------------------------------------------
    float features[NUM_FEATURES];
    computeFeatures(features);

    float probability = modelPredict(features);
    bool abnormal = (probability >= MODEL_THRESHOLD);

    processWindowClassification(abnormal);

    // Diagnostic line for the AI decision
    Serial.print("AI,p=");
    Serial.print(probability, 4);
    Serial.print(",status=");
    Serial.println(abnormal ? "ABNORMAL" : "NORMAL");
}
