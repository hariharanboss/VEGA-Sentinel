#include <iostream>
#include <cmath>
#include <iomanip>

using namespace std;

// ============================================================
// VEGA SENTINEL - MODEL A
// PC-SIDE C++ VERIFICATION
// ============================================================

// StandardScaler parameters
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

// Logistic regression parameters
const float INTERCEPT = -3.483560f;

const float COEF_HUMIDITY   = -2.610233f;
const float COEF_TEMP       =  0.422538f;
const float COEF_MQ_RANGE   =  0.384768f;
const float COEF_MQ_DELTA   = -0.322069f;
const float COEF_MQ_STD     = -0.751392f;
const float COEF_HUM_STD    =  0.561356f;
const float COEF_FLAME      =  0.890513f;


// ------------------------------------------------------------
// Logistic function
// ------------------------------------------------------------

float sigmoid(float x)
{
    return 1.0f / (1.0f + expf(-x));
}


// ------------------------------------------------------------
// Model inference
// ------------------------------------------------------------

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
    // Standardize exactly like Python StandardScaler

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


    // Logistic regression

    float score =
        INTERCEPT
        + COEF_HUMIDITY * z_humidity
        + COEF_TEMP     * z_temp
        + COEF_MQ_RANGE * z_mq_range
        + COEF_MQ_DELTA * z_mq_delta
        + COEF_MQ_STD   * z_mq_std
        + COEF_HUM_STD  * z_hum_std
        + COEF_FLAME    * z_flame;


    return sigmoid(score);
}


// ------------------------------------------------------------
// Test
// ------------------------------------------------------------

int main()
{
    cout << fixed << setprecision(6);

    cout << "========================================\n";
    cout << "VEGA SENTINEL - MODEL A C++ TEST\n";
    cout << "========================================\n";


    // Example NORMAL window
    float normal_probability = modelPredict(
        73.5f,    // humidity_mean
        28.5f,    // temp_mean
        10.0f,    // mq2_range
        1.0f,     // mq2_delta
        3.0f,     // mq2_std
        0.10f,    // humidity_std
        0.0f      // flame_fraction
    );


    // Example ABNORMAL window
    float abnormal_probability = modelPredict(
        69.8f,    // humidity_mean
        28.2f,    // temp_mean
        34.0f,    // mq2_range
        -14.0f,   // mq2_delta
        10.0f,    // mq2_std
        0.24f,    // humidity_std
        0.10f     // flame_fraction
    );


    cout << "\nNormal example probability: "
         << normal_probability << "\n";

    cout << "Abnormal example probability: "
         << abnormal_probability << "\n";


    cout << "\n========================================\n";
    cout << "INTERPRETATION\n";
    cout << "========================================\n";

    if (normal_probability >= 0.5f)
        cout << "Normal example -> ABNORMAL\n";
    else
        cout << "Normal example -> NORMAL\n";

    if (abnormal_probability >= 0.5f)
        cout << "Abnormal example -> ABNORMAL\n";
    else
        cout << "Abnormal example -> NORMAL\n";


    cout << "\n========================================\n";
    cout << "TEST COMPLETE\n";
    cout << "========================================\n";

    return 0;
}