#include <M5Cardputer.h>

#define SENSOR_PIN 1 // Grove port G1 (Yellow wire)

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

    // Explicitly set pin to floating input before turning on ADC
    pinMode(SENSOR_PIN, INPUT);
    delay(50);

    for (int i = 0; i < WIDTH; i++) {
        rawBuffer[i] = 0;
    }
}

void loop() {
    M5Cardputer.update();

    // Read the sensor
    int rawValue = analogRead(SENSOR_PIN); // 0-4095

    // Artificially amplify the reading (with a baseline shift to keep it centered)
    // so that tiny fluctuations become visible
    int amplifiedValue = (rawValue - 2048) * 3 + 2048;

    rawBuffer[currentIndex] = amplifiedValue;
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
    if (range < 150) range = 150; // Adjust minimum range after amplification

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
    // Lowered threshold from 300 to 50 for much higher sensitivity
    static unsigned long lastBeep = 0;
    if (p2p > 50) {
        int beepDelay = map(p2p, 50, 2000, 500, 30);
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
