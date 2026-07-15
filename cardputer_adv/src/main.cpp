#include <Arduino.h>
#include <M5Cardputer.h>

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

BleKeyboard bleKeyboard("Cardputer ADV", "M5Stack", 100);

bool wasConnected = false;
String lastAction = "None";

void drawBaseUI() {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.fillRect(0, 0, M5Cardputer.Display.width(), 30, BLUE);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(10, 8);
    M5Cardputer.Display.println("BLE Remote ADV");

    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setCursor(10, 75);
    M5Cardputer.Display.println("------------------------------");
    M5Cardputer.Display.setCursor(10, 85);
    M5Cardputer.Display.setTextColor(ORANGE);
    M5Cardputer.Display.println("[; / UpArrow]   : Scroll Up");
    M5Cardputer.Display.println("[. / DownArrow] : Scroll Down");
    M5Cardputer.Display.println("[, / LeftArrow] : Volume Down");
    M5Cardputer.Display.println("[/ / RightArrow]: Volume Up");
    M5Cardputer.Display.println("[Space / Enter] : Play / Pause");

    M5Cardputer.Display.setTextColor(LIGHTGREY);
    M5Cardputer.Display.setCursor(10, 40);
    M5Cardputer.Display.print("Status: ");

    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setCursor(10, 140);
    M5Cardputer.Display.print("Last Action: ");
}

void updateStatus(bool isConnected) {
    M5Cardputer.Display.fillRect(60, 40, 180, 30, BLACK);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(60, 40);
    if (isConnected) {
        M5Cardputer.Display.setTextColor(GREEN);
        M5Cardputer.Display.println("CONNECTED");
        M5Cardputer.Display.setTextColor(CYAN);
        M5Cardputer.Display.setCursor(10, 55);
        M5Cardputer.Display.print("Device: Cardputer ADV");
    } else {
        M5Cardputer.Display.setTextColor(RED);
        M5Cardputer.Display.println("DISCONNECTED");
        M5Cardputer.Display.setTextColor(YELLOW);
        M5Cardputer.Display.setCursor(10, 55);
        M5Cardputer.Display.print("Pairing: Cardputer ADV");
    }
}

void updateAction(String action) {
    M5Cardputer.Display.fillRect(100, 140, 140, 15, BLACK);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(YELLOW);
    M5Cardputer.Display.setCursor(100, 140);
    M5Cardputer.Display.print(action);
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg); // Fixed: Removed true flag that causes failure in this M5Cardputer version

    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setBrightness(128);

    drawBaseUI();
    updateStatus(false);
    updateAction("Waiting...");

    bleKeyboard.begin();
}

void loop() {
    M5Cardputer.update();

    bool isConnected = bleKeyboard.isConnected();

    if (isConnected != wasConnected) {
        wasConnected = isConnected;
        updateStatus(isConnected);
        lastAction = isConnected ? "Connected!" : "Waiting...";
        updateAction(lastAction);
    }

    if (isConnected && M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
        bool actionTaken = false;

        if (status.enter || M5Cardputer.Keyboard.isKeyPressed(' ')) {
            bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
            lastAction = "Play / Pause";
            actionTaken = true;
        }
        else if (M5Cardputer.Keyboard.isKeyPressed(';') || (M5Cardputer.Keyboard.isKeyPressed('/') && status.fn) || (M5Cardputer.Keyboard.isKeyPressed('/') && status.ctrl)) {
            bleKeyboard.write(0xDA);
            lastAction = "Scroll Up";
            actionTaken = true;
        }
        else if (M5Cardputer.Keyboard.isKeyPressed('.') || (M5Cardputer.Keyboard.isKeyPressed('.') && status.fn) || (M5Cardputer.Keyboard.isKeyPressed('.') && status.ctrl)) {
            bleKeyboard.write(0xD9);
            lastAction = "Scroll Down";
            actionTaken = true;
        }
        else if (M5Cardputer.Keyboard.isKeyPressed(',') || (M5Cardputer.Keyboard.isKeyPressed(',') && status.fn) || (M5Cardputer.Keyboard.isKeyPressed(',') && status.ctrl)) {
            bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
            lastAction = "Volume Down";
            actionTaken = true;
        }
        else if ((M5Cardputer.Keyboard.isKeyPressed('/') && !status.fn && !status.ctrl) || (M5Cardputer.Keyboard.isKeyPressed(';') && status.fn) || (M5Cardputer.Keyboard.isKeyPressed(';') && status.ctrl) || (M5Cardputer.Keyboard.isKeyPressed(' ') && status.fn)) {
            bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
            lastAction = "Volume Up";
            actionTaken = true;
        }

        if (actionTaken) {
            updateAction(lastAction);
            delay(100);
        }
    }

    delay(20);
}
