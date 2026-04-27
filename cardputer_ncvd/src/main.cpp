#include <M5Cardputer.h>
#include <driver/adc.h>

#define SENSOR_PIN 1 // Grove port G1 (Yellow wire is ADC1_CH0 on ESP32-S3)

M5Canvas canvas(&M5Cardputer.Display);

const int WIDTH = 240;
const int HEIGHT = 135;
int rawBuffer[WIDTH];
int currentIndex = 0;

void setup() {
    auto cfg = M5.config();
    // Disable external I2C (Port A) so it doesn't interfere with our Analog pin
    cfg.external_rtc = false;
    cfg.external_imu = false;
    M5Cardputer.begin(cfg, true);

    // Force the external I2C port to release the pins if it grabbed them
    if (M5.Ex_I2C.isEnabled()) {
        M5.Ex_I2C.release();
    }

    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Speaker.setVolume(128);

    canvas.createSprite(WIDTH, HEIGHT);

    // Explicitly configure the raw ADC driver for maximum sensitivity
    // SENSOR_PIN 1 corresponds to ADC1_CHANNEL_0 on ESP32-S3
    adc1_config_width(ADC_WIDTH_BIT_12); // 0-4095
    // Use 11dB attenuation. We want to read max voltage range (up to 3.3V)
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);

    // Explicitly set pin to high impedance input
    gpio_set_direction((gpio_num_t)SENSOR_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)SENSOR_PIN, GPIO_FLOATING);

    for (int i = 0; i < WIDTH; i++) {
        rawBuffer[i] = 0;
    }
}

void loop() {
    M5Cardputer.update();

    // Read the sensor using low-level API
    int rawValue = adc1_get_raw(ADC1_CHANNEL_0); // 0-4095

    rawBuffer[currentIndex] = rawValue;
    currentIndex = (currentIndex + 1) % WIDTH;

    // Calculate stats
    int minVal = 4095;
    int maxVal = 0;
    long sum = 0;
    for (int i = 0; i < WIDTH; i++) {
        if (rawBuffer[i] > maxVal) maxVal = rawBuffer[i];
        if (rawBuffer[i] < minVal) minVal = rawBuffer[i];
        sum += rawBuffer[i];
    }
    int p2p = maxVal - minVal;
    int avg = sum / WIDTH;

    canvas.fillSprite(BLACK);

    canvas.setTextColor(YELLOW);
    canvas.setTextSize(1);
    canvas.setCursor(5, 5);
    canvas.printf("NCVD - AC Voltage Detector");

    canvas.setTextColor(WHITE);
    canvas.setCursor(5, 20);
    canvas.printf("Intensity (P2P): %d   ", p2p);

    // Draw waveform
    int range = maxVal - minVal;
    if (range < 400) range = 400; // minimum range to avoid ambient noise filling screen

    for (int i = 0; i < WIDTH - 1; i++) {
        int idx = (currentIndex + i) % WIDTH;
        int nextIdx = (currentIndex + i + 1) % WIDTH;

        int val1 = rawBuffer[idx];
        int val2 = rawBuffer[nextIdx];

        int y1 = map(val1, avg - range/2, avg + range/2, HEIGHT - 1, 35);
        int y2 = map(val2, avg - range/2, avg + range/2, HEIGHT - 1, 35);

        y1 = constrain(y1, 35, HEIGHT - 1);
        y2 = constrain(y2, 35, HEIGHT - 1);

        canvas.drawLine(i, y1, i + 1, y2, GREEN);
    }

    // Warning indicator & Beep
    static unsigned long lastBeep = 0;
    if (p2p > 400) {
        int beepDelay = map(p2p, 400, 4000, 500, 30);
        beepDelay = constrain(beepDelay, 30, 500);

        if (millis() - lastBeep > beepDelay) {
            M5Cardputer.Speaker.tone(2000, 20);
            lastBeep = millis();
        }
        canvas.fillCircle(220, 15, 10, RED);
    } else {
        canvas.drawCircle(220, 15, 10, DARKGREY);
    }

    canvas.pushSprite(0, 0);

    // Small delay to make the sample rate suitable for 50/60Hz AC
    // ~2ms delay + drawing time gives roughly 200-400Hz sample rate.
    // 240 samples = ~0.6 to 1.2 seconds of history.
    delay(2);
}
