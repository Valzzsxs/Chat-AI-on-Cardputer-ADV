#include <Arduino.h>

// Undefine conflicting macros from M5Cardputer
#undef KEY_LEFT_CTRL
#undef KEY_LEFT_SHIFT
#undef KEY_LEFT_ALT
#undef KEY_LEFT_GUI
#undef KEY_RIGHT_CTRL
#undef KEY_RIGHT_SHIFT
#undef KEY_RIGHT_ALT
#undef KEY_RIGHT_GUI
#undef KEY_UP_ARROW
#undef KEY_DOWN_ARROW
#undef KEY_LEFT_ARROW
#undef KEY_RIGHT_ARROW
#undef KEY_BACKSPACE
#undef KEY_TAB
#undef KEY_RETURN
#undef KEY_ESC
#undef KEY_INSERT
#undef KEY_DELETE
#undef KEY_PAGE_UP
#undef KEY_PAGE_DOWN
#undef KEY_HOME
#undef KEY_END
#undef KEY_CAPS_LOCK
#undef KEY_F1
#undef KEY_F2
#undef KEY_F3
#undef KEY_F4
#undef KEY_F5
#undef KEY_F6
#undef KEY_F7
#undef KEY_F8
#undef KEY_F9
#undef KEY_F10
#undef KEY_F11
#undef KEY_F12
#undef KEY_F13
#undef KEY_F14
#undef KEY_F15
#undef KEY_F16
#undef KEY_F17
#undef KEY_F18
#undef KEY_F19
#undef KEY_F20
#undef KEY_F21
#undef KEY_F22
#undef KEY_F23
#undef KEY_F24

#include <BleKeyboard.h>
#include <M5Cardputer.h>

BleKeyboard bleKeyboard("Cardputer ADV", "M5Stack", 100);

void drawUI() {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.println("BLE Remote");
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.println("\nControls:");
    M5Cardputer.Display.println("Up/W      : Scroll Up");
    M5Cardputer.Display.println("Down/S    : Scroll Down");
    M5Cardputer.Display.println("Left/A    : Vol Down");
    M5Cardputer.Display.println("Right/D   : Vol Up");
    M5Cardputer.Display.println("Space/Ent : Play/Pause");
    M5Cardputer.Display.println("\nStatus:");
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);
    M5Cardputer.Display.setRotation(1);

    drawUI();
    M5Cardputer.Display.setTextColor(YELLOW);
    M5Cardputer.Display.println("Starting BLE...");

    bleKeyboard.begin();
}

bool wasConnected = false;

void loop() {
    M5Cardputer.update();

    bool isConnected = bleKeyboard.isConnected();
    if (isConnected != wasConnected) {
        drawUI();
        if (isConnected) {
            M5Cardputer.Display.setTextColor(CYAN);
            M5Cardputer.Display.println("Connected to Android!");
        } else {
            M5Cardputer.Display.setTextColor(YELLOW);
            M5Cardputer.Display.println("Waiting for connection...");
        }
        wasConnected = isConnected;
    }

    if (isConnected && M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        if (status.enter || M5Cardputer.Keyboard.isKeyPressed(' ')) {
            bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
        }

        // Up
        if (M5Cardputer.Keyboard.isKeyPressed(';') || M5Cardputer.Keyboard.isKeyPressed('w')) {
            // BLE Keyboard KEY_UP_ARROW is 0xDA
            bleKeyboard.write(0xDA);
        }
        // Down
        if (M5Cardputer.Keyboard.isKeyPressed('.') || M5Cardputer.Keyboard.isKeyPressed('s')) {
            // BLE Keyboard KEY_DOWN_ARROW is 0xD9
            bleKeyboard.write(0xD9);
        }
        // Left
        if (M5Cardputer.Keyboard.isKeyPressed(',') || M5Cardputer.Keyboard.isKeyPressed('a')) {
            bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
        }
        // Right
        if (M5Cardputer.Keyboard.isKeyPressed('/') || M5Cardputer.Keyboard.isKeyPressed('d')) {
            bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
        }
    }
    delay(20);
}
