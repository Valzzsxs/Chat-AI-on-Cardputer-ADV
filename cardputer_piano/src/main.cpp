#include <M5Cardputer.h>

M5Canvas canvas(&M5Cardputer.Display);

const int WIDTH = 240;
const int HEIGHT = 135;

// Frequencies for a simple C Major scale starting from C4
// Notes: C4, D4, E4, F4, G4, A4, B4, C5, D5, E5
const int tones[] = {261, 293, 329, 349, 392, 440, 493, 523, 587, 659};
// Corresponding keys on the bottom row of M5Cardputer
const char keys[] = {'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/'};
const int num_keys = 10;

int active_key_index = -1;

void drawPiano() {
    canvas.fillSprite(BLACK);

    // Header
    canvas.setTextColor(YELLOW);
    canvas.setTextSize(1);
    canvas.setCursor(5, 5);
    canvas.printf("Pocket Piano Synthesizer");

    // Calculate dimensions for the keys
    int key_width = WIDTH / num_keys;
    int key_height = 80;
    int start_y = (HEIGHT - key_height) / 2 + 10;

    for (int i = 0; i < num_keys; i++) {
        int x = i * key_width;

        // Draw the key background
        uint16_t key_color = (i == active_key_index) ? CYAN : WHITE;
        canvas.fillRect(x + 2, start_y, key_width - 4, key_height, key_color);

        // Draw the key character
        canvas.setTextColor(BLACK);
        canvas.setTextSize(1);
        canvas.setCursor(x + (key_width / 2) - 3, start_y + key_height - 15);
        canvas.printf("%c", keys[i]);
    }

    canvas.pushSprite(0, 0);
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);

    // Set high volume, but avoid absolute max to prevent hardware clipping/distortion
    M5Cardputer.Speaker.setVolume(180);

    M5Cardputer.Display.setRotation(1);
    canvas.createSprite(WIDTH, HEIGHT);

    drawPiano();
}

void loop() {
    M5Cardputer.update();

    bool key_pressed = false;
    int new_active_index = -1;

    if (M5Cardputer.Keyboard.isPressed()) {
        for (int i = 0; i < num_keys; i++) {
            if (M5Cardputer.Keyboard.isKeyPressed(keys[i])) {
                key_pressed = true;
                new_active_index = i;
                break;
            }
        }
    }

    if (key_pressed) {
        if (active_key_index != new_active_index) {
            // New key pressed, update display and start tone continuously
            active_key_index = new_active_index;
            drawPiano();
            M5Cardputer.Speaker.tone(tones[active_key_index]);
        }
        // If the same key is held, do nothing (don't re-trigger tone to avoid distortion)
    } else {
        if (active_key_index != -1) {
            // Key was released, stop the tone
            active_key_index = -1;
            drawPiano();
            M5Cardputer.Speaker.stop();
        }
    }

    delay(10); // Small delay for debouncing and loop control
}
