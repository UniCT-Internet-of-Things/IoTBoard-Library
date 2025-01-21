/*
- knolleary/PubSubClient@^2.8 library is required
*/

#include <iot_board.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

const char *ssid = "UniCT-Team-Public";
const char *password = "LabROBO21/22";

const char* mqtt_server = "broker.hivemq.com";
uint16_t mqtt_port = 8883;
WiFiClientSecure client;
PubSubClient mqttClient(client);

boolean runEvery(unsigned long interval){
    static unsigned long previousMillis = 0;
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;
        return true;
    }
    return false;
}


void mqttCallback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message:");
    for (int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();
    Serial.println("-----------------------");
}

void setup(){

    IoTBoard::init_serial();
    IoTBoard::init_display();
    IoTBoard::init_leds();
    display->println("Display enabled");
    display->display();

    WiFi.begin(ssid, password);
    display->println("Connecting to WiFi");
    display->display();
    while (WiFi.status() != WL_CONNECTED){
        delay(500);
    }

    client.setInsecure();
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(mqttCallback);
   
    display->clearDisplay();
    display->setCursor(0, 0);
    display->println("Internet Connected");
    display->display();

    while (!mqttClient.connected()) {
        String client_id = "iotBoard-client-";
        client_id += String(WiFi.macAddress());
        
        if (mqttClient.connect(client_id.c_str())) {
            display->println("MQTT Connected");
        } else {
            display->println("MQTT error");
            delay(2000);
        }
    }
    display->display();
    mqttClient.subscribe("test/recv1");
}

void loop(){
    if(runEvery(4000)){
        mqttClient.publish("test/status", "Hi, I'm IoTBoard ^^");
    }
    mqttClient.loop();
}