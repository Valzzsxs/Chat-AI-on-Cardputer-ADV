#include <M5Cardputer.h>

M5Canvas canvas(&M5Cardputer.Display);

const int WIDTH = 240;
const int HEIGHT = 135;

// Use GPIO 4 (ADC1 pin on ESP32-S3)
const int ANTENNA_PIN = 4;

const int NUM_SAMPLES = 240;
int16_t samples[NUM_SAMPLES];

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);

    M5Cardputer.Display.setRotation(1);
    canvas.createSprite(WIDTH, HEIGHT);

    // Revert to analog input to get amplitude data for auto-gain and distance control
    pinMode(ANTENNA_PIN, ANALOG);
}

void loop() {
    M5Cardputer.update();

    int32_t minVal = 4095;
    int32_t maxVal = 0;
    int64_t sum = 0;

    // Collect analog samples over ~48ms window
    for (int i = 0; i < NUM_SAMPLES; i++) {
        samples[i] = analogRead(ANTENNA_PIN);
        if (samples[i] > maxVal) maxVal = samples[i];
        if (samples[i] < minVal) minVal = samples[i];
        sum += samples[i];
        delayMicroseconds(200); // 200us * 240 = 48ms total
    }

    int32_t avg = sum / NUM_SAMPLES;
    int32_t p2p = maxVal - minVal;

    // "Zero-crossing" logic to detect 50/60Hz frequency and filter out static noise
    int zero_crossings = 0;
    bool is_above = samples[0] > avg;

    // 10% hysteresis threshold around the average to ignore tiny micro-noise
    int hysteresis = p2p * 0.1;

    for (int i = 1; i < NUM_SAMPLES; i++) {
        if (is_above && samples[i] < (avg - hysteresis)) {
            zero_crossings++;
            is_above = false;
        } else if (!is_above && samples[i] > (avg + hysteresis)) {
            zero_crossings++;
            is_above = true;
        }
    }

    // Draw UI
    canvas.fillSprite(BLACK);

    canvas.setTextColor(YELLOW);
    canvas.setTextSize(1);
    canvas.setCursor(5, 5);
    canvas.printf("NCVD - Hybrid Mode");

    canvas.setTextColor(WHITE);
    canvas.setCursor(5, 20);
    // Intensity indicates distance. Crossings indicate frequency filter status.
    canvas.printf("Intensity: %d | Cross: %d   ", (int)p2p, zero_crossings);

    // Auto-gain waveform logic
    int32_t range = p2p;
    if (range < 200) range = 200; // Min range for stable visuals when quiet

    for (int i = 0; i < NUM_SAMPLES - 1; i++) {
        int y1 = map(samples[i], avg - range/2, avg + range/2, HEIGHT - 1, 35);
        int y2 = map(samples[i+1], avg - range/2, avg + range/2, HEIGHT - 1, 35);

        y1 = constrain(y1, 35, HEIGHT - 1);
        y2 = constrain(y2, 35, HEIGHT - 1);

        canvas.drawLine(i, y1, i + 1, y2, GREEN);
    }

    // Detection Logic
    // 1. Frequency check: 50/60Hz wave over 48ms will have about 8-12 zero-crossings.
    //    Allow 4 to 25 to account for phase shift and minor distortions.
    bool valid_frequency = (zero_crossings >= 4 && zero_crossings <= 25);

    // 2. Distance check:
    //    Higher P2P value means closer to the AC source. Max is 4095.
    //    Setting threshold to > 1500 reduces range from ~30cm to roughly 5-10cm depending on cable.
    //    You can lower this to 1000 to increase range, or raise to 2500 to decrease range.
    bool close_enough = (p2p > 1500);

    if (valid_frequency && close_enough) {
        canvas.fillCircle(220, 15, 10, RED);
        canvas.setTextColor(RED);
        canvas.setCursor(5, 35);
        canvas.printf("WARNING: AC DETECTED!");
        M5Cardputer.Speaker.tone(2000, 50); // Beep
    } else {
        canvas.drawCircle(220, 15, 10, DARKGREY);
    }

    canvas.pushSprite(0, 0);
    delay(30);
}
