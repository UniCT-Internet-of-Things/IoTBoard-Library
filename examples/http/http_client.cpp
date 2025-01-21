/*
HTTPs Example, bblanchon/ArduinoJson@^7.3.0 library is required!
*/

#include <iot_board.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

const char *ssid = "";
const char *password = "";

String serverName = "https://api.coindesk.com/v1/bpi/currentprice.json";
WiFiClientSecure client;
HTTPClient httpClient;
String response = "";

JsonDocument doc;

boolean runEvery(unsigned long interval)
{
    static unsigned long previousMillis = 0;
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;
        return true;
    }
    return false;
}

void setup()
{

    IoTBoard::init_serial();
    IoTBoard::init_buttons();
    IoTBoard::init_leds();
    IoTBoard::init_display();
    display->println("Display enabled");
    display->display();

    WiFi.begin(ssid, password);
    display->println("Connecting to WiFi");
    display->display();
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }
    display->clearDisplay();
    display->setCursor(0, 0);
    display->println("Connected");
    display->println(WiFi.localIP());
    display->display();

    client.setInsecure();
}

void loop()
{

    buttons->update();
    if (runEvery(2000))
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            httpClient.begin(client, serverName);
            httpClient.addHeader("accept", "application/json");
            httpClient.addHeader("Content-Type", "application/json");
            int httpResponseCode = httpClient.GET();

            if (httpResponseCode > 0)
            {
                Serial.print("HTTP Response code: ");
                Serial.println(httpResponseCode);
                response = httpClient.getString();
                DeserializationError error = deserializeJson(doc, response);
                if (error)
                {
                    Serial.print(F("invalid json"));
                    Serial.println(error.f_str());
                    httpClient.end();
                    return;
                }
                float eurValue = doc["bpi"]["EUR"]["rate_float"];
                const char* time = doc["time"]["updated"];
                display->clearDisplay();
                display->setCursor(0, 0);
                display->println("Bitcoin Value");
                display->println(time);
                display->println(eurValue);
                display->display();

            }
            else
            {
                Serial.print("Error code: ");
                Serial.println(httpResponseCode);
            }
            httpClient.end();
        }
    }
}