#include <math.h>

// ============================================================
// VEGA SENTINEL
// REAL SENSOR + MODEL A TEST
// VEGA ARIES v2.0
//
// Sensors:
//   DHT22  -> GPIO0
//   Flame  -> GPIO1
//   MQ-2   -> A0
//
// Collects 10 valid samples, calculates 7 features,
// then runs the trained Logistic Regression model locally.
// ============================================================

#define DHT_PIN       0
#define FLAME_PIN     1
#define MQ2_PIN       A0

#define NUM_SAMPLES   10
#define SAMPLE_INTERVAL_MS 2500

uint8_t dhtData[5];

// ============================================================
// MODEL A PARAMETERS
// ============================================================

// StandardScaler
const float MEAN_HUMIDITY   = 73.242897f;
const float SCALE_HUMIDITY  = 1.548508f;

const float MEAN_TEMP       = 28.448692f;
const float SCALE_TEMP      = 0.203029f;

const float MEAN_MQ_RANGE   = 11.616822f;
const float SCALE_MQ_RANGE  = 17.851226f;

const float MEAN_MQ_DELTA   = 0.485981f;
const float SCALE_MQ_DELTA  = 14.173402f;

const float MEAN_MQ_STD     = 3.734761f;
const float SCALE_MQ_STD    = 5.900505f;

const float MEAN_HUM_STD    = 0.115325f;
const float SCALE_HUM_STD   = 0.163725f;

const float MEAN_FLAME      = 0.004673f;
const float SCALE_FLAME     = 0.031721f;

// Logistic regression
const float INTERCEPT       = -3.483560f;

const float COEF_HUMIDITY   = -2.610233f;
const float COEF_TEMP       =  0.422538f;
const float COEF_MQ_RANGE   =  0.384768f;
const float COEF_MQ_DELTA   = -0.322069f;
const float COEF_MQ_STD     = -0.751392f;
const float COEF_HUM_STD    =  0.561356f;
const float COEF_FLAME      =  0.890513f;


// ============================================================
// DHT22 DRIVER
// ============================================================

bool waitForLevel(int level, unsigned long timeout_us)
{
    unsigned long start = micros();

    while (digitalRead(DHT_PIN) != level)
    {
        if ((micros() - start) > timeout_us)
            return false;
    }

    return true;
}


bool readDHT22()
{
    for (int i = 0; i < 5; i++)
        dhtData[i] = 0;

    // Start signal
    pinMode(DHT_PIN, OUTPUT);

    digitalWrite(DHT_PIN, LOW);
    delay(2);

    digitalWrite(DHT_PIN, HIGH);
    delayMicroseconds(30);

    pinMode(DHT_PIN, INPUT);

    // Sensor response
    if (!waitForLevel(LOW, 200))
        return false;

    if (!waitForLevel(HIGH, 200))
        return false;

    if (!waitForLevel(LOW, 200))
        return false;

    // Read 40 bits
    for (int i = 0; i < 40; i++)
    {
        if (!waitForLevel(HIGH, 100))
            return false;

        unsigned long start = micros();

        if (!waitForLevel(LOW, 150))
            return false;

        unsigned long highTime = micros() - start;

        dhtData[i / 8] <<= 1;

        if (highTime > 40)
            dhtData[i / 8] |= 1;
    }

    // Checksum
    uint8_t checksum =
        dhtData[0] +
        dhtData[1] +
        dhtData[2] +
        dhtData[3];

    if (checksum != dhtData[4])
        return false;

    return true;
}


// ============================================================
// CONVERT DHT22 DATA
// ============================================================

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


float getHumidity()
{
    uint16_t rawHumidity =
        ((uint16_t)dhtData[0] << 8) | dhtData[1];

    return rawHumidity / 10.0f;
}


// ============================================================
// STANDARD DEVIATION
//
// Uses sample standard deviation (n-1), matching pandas
// default behaviour used during dataset feature generation.
// ============================================================

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


// ============================================================
// MODEL A
// ============================================================

float modelPredict(
    float humidity_mean,
    float temp_mean,
    float mq2_range,
    float mq2_delta,
    float mq2_std,
    float humidity_std,
    float flame_fraction
)
{
    // Standardization

    float z_humidity =
        (humidity_mean - MEAN_HUMIDITY) / SCALE_HUMIDITY;

    float z_temp =
        (temp_mean - MEAN_TEMP) / SCALE_TEMP;

    float z_mq_range =
        (mq2_range - MEAN_MQ_RANGE) / SCALE_MQ_RANGE;

    float z_mq_delta =
        (mq2_delta - MEAN_MQ_DELTA) / SCALE_MQ_DELTA;

    float z_mq_std =
        (mq2_std - MEAN_MQ_STD) / SCALE_MQ_STD;

    float z_hum_std =
        (humidity_std - MEAN_HUM_STD) / SCALE_HUM_STD;

    float z_flame =
        (flame_fraction - MEAN_FLAME) / SCALE_FLAME;


    // Logistic regression score

    float score =
        INTERCEPT
        + COEF_HUMIDITY * z_humidity
        + COEF_TEMP     * z_temp
        + COEF_MQ_RANGE * z_mq_range
        + COEF_MQ_DELTA * z_mq_delta
        + COEF_MQ_STD   * z_mq_std
        + COEF_HUM_STD  * z_hum_std
        + COEF_FLAME    * z_flame;


    // Sigmoid

    float probability =
        1.0f / (1.0f + expf(-score));

    return probability;
}


// ============================================================
// SETUP
// ============================================================

void setup()
{
    Serial.begin(115200);

    delay(2000);

    pinMode(FLAME_PIN, INPUT);

    Serial.println();
    Serial.println("========================================");
    Serial.println("VEGA SENTINEL");
    Serial.println("REAL SENSOR + MODEL A");
    Serial.println("VEGA ARIES v2.0");
    Serial.println("========================================");

    Serial.println();
    Serial.println("Sensors:");
    Serial.println("DHT22  -> GPIO0");
    Serial.println("Flame  -> GPIO1");
    Serial.println("MQ-2   -> A0");

    Serial.println();
    Serial.println("Collecting 10 valid samples...");
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop()
{
    float temperatures[NUM_SAMPLES];
    float humidities[NUM_SAMPLES];
    float mq2Values[NUM_SAMPLES];

    int flameValues[NUM_SAMPLES];

    int sampleCount = 0;


    // --------------------------------------------------------
    // Collect 10 valid samples
    // --------------------------------------------------------

    while (sampleCount < NUM_SAMPLES)
    {
        float temperature;
        float humidity;

        bool dhtOK = readDHT22();

        if (!dhtOK)
        {
            Serial.println("DHT22 read failed - retrying...");
            delay(SAMPLE_INTERVAL_MS);
            continue;
        }

        temperature = getTemperature();
        humidity = getHumidity();

        int mq2 = analogRead(MQ2_PIN);

        int flameRaw = digitalRead(FLAME_PIN);

        // Flame module is active LOW
        int flame = (flameRaw == LOW) ? 1 : 0;


        temperatures[sampleCount] = temperature;
        humidities[sampleCount] = humidity;
        mq2Values[sampleCount] = mq2;
        flameValues[sampleCount] = flame;


        sampleCount++;


        Serial.print("Sample ");
        Serial.print(sampleCount);
        Serial.print("/");
        Serial.print(NUM_SAMPLES);

        Serial.print(" | Temp=");
        Serial.print(temperature, 2);

        Serial.print(" | Hum=");
        Serial.print(humidity, 2);

        Serial.print(" | MQ2=");
        Serial.print(mq2);

        Serial.print(" | Flame=");
        Serial.println(flame);


        if (sampleCount < NUM_SAMPLES)
            delay(SAMPLE_INTERVAL_MS);
    }


    // ========================================================
    // CALCULATE 7 FEATURES
    // ========================================================

    float tempMean = 0.0f;
    float humidityMean = 0.0f;
    float mq2Mean = 0.0f;

    float tempStd;
    float humidityStd;
    float mq2Std;

    float mq2Min = mq2Values[0];
    float mq2Max = mq2Values[0];

    int flameCount = 0;


    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        tempMean += temperatures[i];
        humidityMean += humidities[i];
        mq2Mean += mq2Values[i];

        if (mq2Values[i] < mq2Min)
            mq2Min = mq2Values[i];

        if (mq2Values[i] > mq2Max)
            mq2Max = mq2Values[i];

        flameCount += flameValues[i];
    }


    tempMean /= NUM_SAMPLES;
    humidityMean /= NUM_SAMPLES;
    mq2Mean /= NUM_SAMPLES;


    tempStd =
        calculateStd(temperatures, NUM_SAMPLES);

    humidityStd =
        calculateStd(humidities, NUM_SAMPLES);

    mq2Std =
        calculateStd(mq2Values, NUM_SAMPLES);


    // 7 MODEL FEATURES

    float mq2Range =
        mq2Max - mq2Min;

    float mq2Delta =
        mq2Values[NUM_SAMPLES - 1] - mq2Values[0];

    float flameFraction =
        (float)flameCount / NUM_SAMPLES;


    // ========================================================
    // RUN MODEL
    // ========================================================

    float probability = modelPredict(
        humidityMean,
        tempMean,
        mq2Range,
        mq2Delta,
        mq2Std,
        humidityStd,
        flameFraction
    );


    // ========================================================
    // DISPLAY FEATURES
    // ========================================================

    Serial.println();
    Serial.println("========================================");
    Serial.println("WINDOW FEATURES");
    Serial.println("========================================");

    Serial.print("humidity_mean   = ");
    Serial.println(humidityMean, 4);

    Serial.print("temp_mean       = ");
    Serial.println(tempMean, 4);

    Serial.print("mq2_range       = ");
    Serial.println(mq2Range, 4);

    Serial.print("mq2_delta       = ");
    Serial.println(mq2Delta, 4);

    Serial.print("mq2_std         = ");
    Serial.println(mq2Std, 4);

    Serial.print("humidity_std    = ");
    Serial.println(humidityStd, 4);

    Serial.print("flame_fraction  = ");
    Serial.println(flameFraction, 4);


    // ========================================================
    // MODEL RESULT
    // ========================================================

    Serial.println();
    Serial.println("========================================");
    Serial.println("EDGE AI RESULT");
    Serial.println("========================================");

    Serial.print("Abnormal probability = ");
    Serial.println(probability, 4);


    if (probability >= 0.5f)
    {
        Serial.println("STATUS = ABNORMAL");
    }
    else
    {
        Serial.println("STATUS = NORMAL");
    }


    Serial.println();
    Serial.println("========================================");
    Serial.println("WINDOW COMPLETE");
    Serial.println("========================================");

    Serial.println();

    // Wait before starting next window
    delay(2000);
}
