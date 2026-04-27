#include <M5Cardputer.h>

M5Canvas canvas(&M5Cardputer.Display);

const int WIDTH = 240;
const int HEIGHT = 135;

static constexpr const size_t record_length = 240;
static constexpr const size_t record_samplerate = 1000; // Low sample rate to catch 50/60Hz AC hum easily
int16_t rec_data[record_length];

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);

    M5Cardputer.Display.setRotation(1);

    canvas.createSprite(WIDTH, HEIGHT);

    // Setup Microphone
    auto mic_cfg = M5Cardputer.Mic.config();
    mic_cfg.sample_rate = record_samplerate;
    M5Cardputer.Mic.config(mic_cfg);

    M5Cardputer.Speaker.end(); // Must disable speaker to use Mic
    M5Cardputer.Mic.begin();
}

void loop() {
    M5Cardputer.update();

    if (M5Cardputer.Mic.isEnabled()) {
        if (M5Cardputer.Mic.record(rec_data, record_length, record_samplerate)) {

            // Calculate Stats (Peak to Peak / Intensity)
            int32_t minVal = 32767;
            int32_t maxVal = -32768;
            int64_t sum = 0;

            for (size_t i = 0; i < record_length; i++) {
                if (rec_data[i] > maxVal) maxVal = rec_data[i];
                if (rec_data[i] < minVal) minVal = rec_data[i];
                sum += rec_data[i];
            }

            int32_t p2p = maxVal - minVal;
            int32_t avg = sum / record_length;

            // Draw UI
            canvas.fillSprite(BLACK);

            canvas.setTextColor(YELLOW);
            canvas.setTextSize(1);
            canvas.setCursor(5, 5);
            canvas.printf("NCVD - AC Hum Detector (Mic)");

            canvas.setTextColor(WHITE);
            canvas.setCursor(5, 20);
            canvas.printf("Hum Intensity: %d   ", (int)p2p);

            // Draw waveform
            int32_t range = maxVal - minVal;
            if (range < 1000) range = 1000; // Minimum range to filter room noise

            for (size_t i = 0; i < record_length - 1; i++) {
                int y1 = map(rec_data[i], avg - range/2, avg + range/2, HEIGHT - 1, 35);
                int y2 = map(rec_data[i+1], avg - range/2, avg + range/2, HEIGHT - 1, 35);

                y1 = constrain(y1, 35, HEIGHT - 1);
                y2 = constrain(y2, 35, HEIGHT - 1);

                canvas.drawLine(i, y1, i + 1, y2, GREEN);
            }

            // Warning indicator (visual only since speaker is off for mic)
            if (p2p > 2000) { // Threshold for electrical hum near a wire
                canvas.fillCircle(220, 15, 10, RED);
                canvas.setTextColor(RED);
                canvas.setCursor(5, 35);
                canvas.printf("WARNING: AC DETECTED!");
            } else {
                canvas.drawCircle(220, 15, 10, DARKGREY);
            }

            canvas.pushSprite(0, 0);
        }
    }
}
