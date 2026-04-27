#include <M5Cardputer.h>

M5Canvas canvas(&M5Cardputer.Display);

const int WIDTH = 240;
const int HEIGHT = 135;

// We will use G9 (GPIO 9) from the StampS3 header as our antenna pin.
// It is an ADC1 pin and exposed on the header block under the Cardputer cover.
const int ANTENNA_PIN = 9;

const int NUM_SAMPLES = 240;
int16_t samples[NUM_SAMPLES];

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);

    M5Cardputer.Display.setRotation(1);
    canvas.createSprite(WIDTH, HEIGHT);

    // Configure pin as analog input
    pinMode(ANTENNA_PIN, INPUT);
    // Note: ADC is configured automatically on first analogRead
}

void loop() {
    M5Cardputer.update();

    int32_t minVal = 4095;
    int32_t maxVal = 0;
    int64_t sum = 0;

    // Collect samples
    for (int i = 0; i < NUM_SAMPLES; i++) {
        samples[i] = analogRead(ANTENNA_PIN);
        if (samples[i] > maxVal) maxVal = samples[i];
        if (samples[i] < minVal) minVal = samples[i];
        sum += samples[i];
        delayMicroseconds(2000); // 2ms per sample to capture 50/60Hz waveform effectively
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

    // Draw waveform
    // Set a minimum range for visual stability when there is no signal
    int32_t range = maxVal - minVal;
    if (range < 200) range = 200;

    for (int i = 0; i < NUM_SAMPLES - 1; i++) {
        int y1 = map(samples[i], avg - range/2, avg + range/2, HEIGHT - 1, 35);
        int y2 = map(samples[i+1], avg - range/2, avg + range/2, HEIGHT - 1, 35);

        y1 = constrain(y1, 35, HEIGHT - 1);
        y2 = constrain(y2, 35, HEIGHT - 1);

        canvas.drawLine(i, y1, i + 1, y2, GREEN);
    }

    // Warning indicator and beeper
    if (p2p > 300) { // Threshold for electrical hum near a wire
        canvas.fillCircle(220, 15, 10, RED);
        canvas.setTextColor(RED);
        canvas.setCursor(5, 35);
        canvas.printf("WARNING: AC DETECTED!");
        M5Cardputer.Speaker.tone(2000, 50); // Beep
    } else {
        canvas.drawCircle(220, 15, 10, DARKGREY);
    }

    canvas.pushSprite(0, 0);
    delay(50); // Small delay to prevent flickering and excessive beeping
}
