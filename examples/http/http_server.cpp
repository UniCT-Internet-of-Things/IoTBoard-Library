/*
ESP Async WebServer library is required and AsyncTCP
*/

#include <iot_board.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char *ssid = "";
const char *password = "";
AsyncWebServer server(80);

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

    //GET on root
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Hello, world");
    });

    //GET with parameters
    server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam("parametro")) {
            message = request->getParam("parametro")->value();
        } else {
            message = "No message sent";
        }
        request->send(200, "text/plain", "Hello, GET: " + message);
    });

    // POST with Body
    server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request){
        String message;
        if (request->hasParam("parametro", true)) {
            message = request->getParam("parametro", true)->value();
        } else {
            message = "No message sent";
        }
        request->send(200, "text/plain", "Hello, POST: " + message);
    });
    
    display->clearDisplay();
    display->setCursor(0, 0);
    display->println("Connected");
    display->println(WiFi.localIP());
    display->println("Server actived on port 80");
    display->display();

    server.begin();
}

void loop(){
}