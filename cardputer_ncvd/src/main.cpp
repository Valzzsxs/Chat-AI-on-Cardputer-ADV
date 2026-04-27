#include <M5Cardputer.h>

M5Canvas canvas(&M5Cardputer.Display);

const int WIDTH = 240;
const int HEIGHT = 135;

// Use GPIO 4 as it is exposed and away from internal conflicts.
const int ANTENNA_PIN = 4;

const int NUM_SAMPLES = 240;
uint8_t samples[NUM_SAMPLES];

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);

    M5Cardputer.Display.setRotation(1);
    canvas.createSprite(WIDTH, HEIGHT);

    // Configure pin as digital input
    // The ESP32's digital input is extremely sensitive when floating.
    // The AC electric field will cause the floating pin to toggle high/low at 50/60Hz.
    pinMode(ANTENNA_PIN, INPUT);
}

void loop() {
    M5Cardputer.update();

    int transition_count = 0;
    uint8_t last_val = 2; // Invalid initial value
    int high_count = 0;

    // We sample quickly over ~48ms (approx 2.5 cycles of 50Hz AC)
    for (int i = 0; i < NUM_SAMPLES; i++) {
        samples[i] = digitalRead(ANTENNA_PIN);

        if (samples[i] == HIGH) {
            high_count++;
        }

        if (i > 0 && samples[i] != last_val) {
            transition_count++;
        }
        last_val = samples[i];

        delayMicroseconds(200); // 200us * 240 = 48ms
    }

    // Draw UI
    canvas.fillSprite(BLACK);

    canvas.setTextColor(YELLOW);
    canvas.setTextSize(1);
    canvas.setCursor(5, 5);
    canvas.printf("NCVD - AC Detector (Digital)");

    canvas.setTextColor(WHITE);
    canvas.setCursor(5, 20);
    // Print transitions for debugging/info. A 50Hz wave should have ~5-10 transitions in 48ms.
    canvas.printf("Transitions: %d | Highs: %d   ", transition_count, high_count);

    // Draw digital square waveform
    int wave_y_low = HEIGHT - 20;
    int wave_y_high = 40;

    for (int i = 0; i < NUM_SAMPLES - 1; i++) {
        int y1 = (samples[i] == HIGH) ? wave_y_high : wave_y_low;
        int y2 = (samples[i+1] == HIGH) ? wave_y_high : wave_y_low;

        if (y1 != y2) {
            // Draw vertical line for transition
            canvas.drawLine(i, y1, i, y2, GREEN);
        } else {
            // Draw horizontal line
            canvas.drawLine(i, y1, i + 1, y2, GREEN);
        }
    }

    // AC Detection Logic
    // If the wire is near AC, it should toggle somewhat regularly.
    // If it's too high (noise/static) or too low (nothing), ignore.
    // We expect roughly 4 to 15 transitions in a 48ms window for 50/60Hz AC.
    // Also, the signal should not be completely stuck HIGH or LOW.
    bool ac_detected = (transition_count >= 4 && transition_count <= 25) &&
                       (high_count > 20 && high_count < 220);

    if (ac_detected) {
        canvas.fillCircle(220, 15, 10, RED);
        canvas.setTextColor(RED);
        canvas.setCursor(5, 35);
        canvas.printf("WARNING: AC DETECTED!");
        M5Cardputer.Speaker.tone(2000, 50); // Beep
    } else {
        canvas.drawCircle(220, 15, 10, DARKGREY);
    }

    canvas.pushSprite(0, 0);
    delay(30); // Prevent screen tearing and crazy fast beeps
}
