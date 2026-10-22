/*
- -DESPMESH_ENABLED build flag is required
*/

#include <iot_board.h>

const char *ssid = "ARSLab-IoT_2G";
const char *password = "arslabiot";

int count_sent = 0;

void onMesh(const EspMeshEvent &event){
    switch(event.type){
        case EspMeshEventType::Connected:
            digitalWrite(LED_GREEN, HIGH);
            break;
        case EspMeshEventType::Disconnected:
            digitalWrite(LED_GREEN, LOW);
            digitalWrite(LED_YELLOW, LOW);
            break;
        case EspMeshEventType::BecameRoot:
            digitalWrite(LED_YELLOW, HIGH);
            break;
        case EspMeshEventType::Message:
            display->clearDisplay();
            display->setCursor(0, 0);
            display->printf("From " MACSTR "\n", MAC2STR(event.from));
            display->printf("%s\n", (char *) event.data);
            display->display();
            break;
        default:
            break;
    }
}

void setup(){

    IoTBoard::init_serial();
    IoTBoard::init_leds();
    IoTBoard::init_display();

    if(IoTBoard::init_espmesh(ssid, password, onMesh)){
        display->println("EspMesh enabled");
    }
    else{
        display->println("EspMesh Error");
    }
    display->display();
}

void loop(){

    espmesh->update();

    if(IoTBoard::runEvery(5000) && espmesh->isConnected() && !espmesh->isRoot()){
        String s = "Hello " + String(count_sent) + " layer " + String(espmesh->getLayer());
        espmesh->sendToRoot(s.c_str(), s.length());
        count_sent++;
    }
}
