#include <M5Cardputer.h>

M5Canvas canvas(&M5Cardputer.Display);

const int WIDTH = 240;
const int HEIGHT = 135;

// Let's use GPIO 4. It's an ADC1 pin on the ESP32-S3 and is exposed on the header.
// It is safely away from the strapping pins and SD card/I2C pins.
const int ANTENNA_PIN = 4;

const int NUM_SAMPLES = 240;
int16_t samples[NUM_SAMPLES];

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);

    M5Cardputer.Display.setRotation(1);
    canvas.createSprite(WIDTH, HEIGHT);

    // We explicitly set the pin as an analog input to remove any digital pull-up/down
    pinMode(ANTENNA_PIN, ANALOG);
}

void loop() {
    M5Cardputer.update();

    int32_t minVal = 4095;
    int32_t maxVal = 0;
    int64_t sum = 0;

    // To read the weak electromagnetic field, we sample as fast as possible for a short burst
    // A 50Hz AC wave completes a cycle in 20ms. 60Hz in 16.6ms.
    for (int i = 0; i < NUM_SAMPLES; i++) {
        samples[i] = analogRead(ANTENNA_PIN);
        if (samples[i] > maxVal) maxVal = samples[i];
        if (samples[i] < minVal) minVal = samples[i];
        sum += samples[i];
        // 100us delay * 240 samples = 24ms total sample time.
        // This is enough to capture at least one full cycle of 50Hz or 60Hz.
        delayMicroseconds(100);
    }

    int32_t p2p = maxVal - minVal;
    int32_t avg = sum / NUM_SAMPLES;

    // Draw UI
    canvas.fillSprite(BLACK);

    canvas.setTextColor(YELLOW);
    canvas.setTextSize(1);
    canvas.setCursor(5, 5);
    canvas.printf("NCVD - AC Voltage Detector");

    canvas.setTextColor(WHITE);
    canvas.setCursor(5, 20);
    canvas.printf("Intensity (P2P): %d   ", (int)p2p);

    // Autoscale logic
    // Set a minimum range for visual stability when there is no signal
    int32_t range = maxVal - minVal;
    if (range < 50) range = 50;

    for (int i = 0; i < NUM_SAMPLES - 1; i++) {
        int y1 = map(samples[i], avg - range/2, avg + range/2, HEIGHT - 1, 35);
        int y2 = map(samples[i+1], avg - range/2, avg + range/2, HEIGHT - 1, 35);

        y1 = constrain(y1, 35, HEIGHT - 1);
        y2 = constrain(y2, 35, HEIGHT - 1);

        canvas.drawLine(i, y1, i + 1, y2, GREEN);
    }

    // Warning indicator and beeper
    // A floating pin will pick up noise, so we need a reasonable threshold
    if (p2p > 400) {
        canvas.fillCircle(220, 15, 10, RED);
        canvas.setTextColor(RED);
        canvas.setCursor(5, 35);
        canvas.printf("WARNING: AC DETECTED!");
        M5Cardputer.Speaker.tone(2000, 50); // Beep
    } else {
        canvas.drawCircle(220, 15, 10, DARKGREY);
    }

    canvas.pushSprite(0, 0);
    // Don't delay too long so we remain responsive
}
