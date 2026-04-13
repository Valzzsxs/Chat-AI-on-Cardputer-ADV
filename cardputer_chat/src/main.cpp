#include <M5Cardputer.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <SPIFFS.h>

Preferences preferences;
String ssid = "";
String password = "";
String openai_key = "";

String userInput = "";
bool isProcessing = false;

// Function to handle keyboard input
String readInput(String prompt, bool isPassword = false) {
    String input = "";
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.println(prompt);

    while(true) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
            if (status.enter) {
                return input;
            } else if (status.del) {
                if (input.length() > 0) {
                    input.remove(input.length() - 1);
                }
            } else {
                for (auto i : status.word) {
                    input += i;
                }
            }

            // Redraw
            M5Cardputer.Display.fillScreen(BLACK);
            M5Cardputer.Display.setCursor(0, 0);
            M5Cardputer.Display.setTextColor(GREEN);
            M5Cardputer.Display.println(prompt);
            M5Cardputer.Display.setTextColor(WHITE);
            if (isPassword) {
                String stars = "";
                for(int i=0; i<input.length(); i++) stars += "*";
                M5Cardputer.Display.println(stars);
            } else {
                M5Cardputer.Display.println(input);
            }
        }
        delay(10);
    }
}

void setupCredentials() {
    preferences.begin("aichat", false);
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    openai_key = preferences.getString("openai_key", "");

    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(YELLOW);
    M5Cardputer.Display.println("Press DEL in 3s");
    M5Cardputer.Display.println("to reset config");

    long startTime = millis();
    bool resetConfig = false;
    while(millis() - startTime < 3000) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            if (M5Cardputer.Keyboard.keysState().del) {
                resetConfig = true;
                break;
            }
        }
        delay(10);
    }

    if (ssid == "" || openai_key == "" || resetConfig) {
        ssid = readInput("Enter WiFi SSID:");
        password = readInput("Enter WiFi PASS:");
        openai_key = readInput("OpenAI API Key:", true);

        preferences.putString("ssid", ssid);
        preferences.putString("password", password);
        preferences.putString("openai_key", openai_key);
    }
}

void connectWiFi() {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(YELLOW);
    M5Cardputer.Display.println("Connecting WiFi..");
    M5Cardputer.Display.println(ssid);

    WiFi.begin(ssid.c_str(), password.c_str());
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        M5Cardputer.Display.print(".");
        attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        M5Cardputer.Display.println("\nConnected!");
    } else {
        M5Cardputer.Display.println("\nFailed! Resetting.");
        delay(2000);
        ESP.restart();
    }
    delay(1000);
}

String queryChatAPI(String text) {
    HTTPClient http;
    http.begin("https://api.openai.com/v1/chat/completions");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + openai_key);

    JsonDocument doc;
    doc["model"] = "gpt-3.5-turbo";
    JsonArray messages = doc["messages"].to<JsonArray>();
    JsonObject msg = messages.add<JsonObject>();
    msg["role"] = "user";
    msg["content"] = text;

    String requestBody;
    serializeJson(doc, requestBody);

    int httpResponseCode = http.POST(requestBody);
    String responseText = "";

    if (httpResponseCode == 200) {
        String response = http.getString();
        JsonDocument resDoc;
        deserializeJson(resDoc, response);
        const char* content = resDoc["choices"][0]["message"]["content"];
        responseText = String(content);
    } else {
        responseText = "API Error: " + String(httpResponseCode);
        String errorMsg = http.getString();
        Serial.println(errorMsg);
    }
    http.end();
    return responseText;
}

void playTTS(String text) {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(CYAN);
    M5Cardputer.Display.println("Downloading TTS...");

    HTTPClient http;
    http.begin("https://api.openai.com/v1/audio/speech");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + openai_key);

    JsonDocument doc;
    doc["model"] = "tts-1";
    doc["input"] = text;
    doc["voice"] = "alloy";
    doc["response_format"] = "wav";

    String requestBody;
    serializeJson(doc, requestBody);

    int httpResponseCode = http.POST(requestBody);
    if (httpResponseCode == 200) {
        File file = SPIFFS.open("/tts.wav", FILE_WRITE);
        if (!file) {
            M5Cardputer.Display.println("Failed to open file");
            return;
        }
        http.writeToStream(&file);
        file.close();

        M5Cardputer.Display.println("Playing audio...");
        M5.Speaker.setVolume(200);
        File readFile = SPIFFS.open("/tts.wav", FILE_READ);
        if (readFile) {
            size_t fileSize = readFile.size();
            uint8_t *buffer = (uint8_t*)malloc(fileSize);
            if (buffer) {
                readFile.read(buffer, fileSize);
                // Simple playWav (M5Unified supports this)
                M5.Speaker.playWav(buffer, fileSize);
                while(M5.Speaker.isPlaying()) {
                    delay(10);
                }
                free(buffer);
            } else {
                M5Cardputer.Display.println("Not enough RAM");
            }
            readFile.close();
        }
    } else {
        M5Cardputer.Display.println("TTS Error: " + String(httpResponseCode));
        Serial.println(http.getString());
    }
    http.end();
}

void drawUI() {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.println("--- AI Chat ---");
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.println(userInput);
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1);

    if(!SPIFFS.begin(true)){
        Serial.println("SPIFFS Mount Failed");
    }

    setupCredentials();
    connectWiFi();
    drawUI();
}

void loop() {
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
        if (status.enter) {
            if (userInput.length() > 0) {
                M5Cardputer.Display.fillScreen(BLACK);
                M5Cardputer.Display.setCursor(0, 0);
                M5Cardputer.Display.setTextColor(YELLOW);
                M5Cardputer.Display.println("Thinking...");

                String answer = queryChatAPI(userInput);

                M5Cardputer.Display.fillScreen(BLACK);
                M5Cardputer.Display.setCursor(0, 0);
                M5Cardputer.Display.setTextColor(CYAN);
                M5Cardputer.Display.println("AI:");
                M5Cardputer.Display.setTextColor(WHITE);
                M5Cardputer.Display.println(answer);

                playTTS(answer);

                delay(3000);
                userInput = "";
                drawUI();
            }
        } else if (status.del) {
            if (userInput.length() > 0) {
                userInput.remove(userInput.length() - 1);
                drawUI();
            }
        } else {
            for (auto i : status.word) {
                userInput += i;
            }
            drawUI();
        }
    }
}
