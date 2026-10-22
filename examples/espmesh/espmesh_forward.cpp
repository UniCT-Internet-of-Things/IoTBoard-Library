
#include <iot_board.h>

const char *ssid = "ARSLab-IoT_2G";
const char *password = "arslabiot";

IPAddress server(192, 168, 1, 5);
uint16_t port = 5000;

int count_sent = 0;

void onBtn1Released(uint8_t pinBtn){
    String s = "Broadcast " + String(count_sent++);
    espmesh->broadcast(s.c_str(), s.length());
}

void onBtn2Released(uint8_t pinBtn){
    String s = "To server " + String(count_sent++);
    espmesh->sendToServer(server, port, s.c_str(), s.length());
}

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
        case EspMeshEventType::GotIP:
            Serial.println("Root connected to the router");
            break;
        case EspMeshEventType::ChildJoined:
            Serial.printf("Child joined " MACSTR "\n", MAC2STR(event.from));
            break;
        case EspMeshEventType::ChildLeft:
            Serial.printf("Child left " MACSTR "\n", MAC2STR(event.from));
            break;
        case EspMeshEventType::Message:
            display->clearDisplay();
            display->setCursor(0, 0);
            display->printf("From " MACSTR "\n", MAC2STR(event.from));
            display->printf("%s\n", (char *) event.data);
            display->display();
            break;
    }
}

void setup(){

    IoTBoard::init_serial();
    IoTBoard::init_leds();
    IoTBoard::init_buttons();
    IoTBoard::init_display();
    buttons->onBtn1Release(onBtn1Released);
    buttons->onBtn2Release(onBtn2Released);

    EspMeshConfig config;
    config.router_ssid = ssid;
    config.router_pass = password;
    config.max_layer = 4;
    config.forward_ip = server;
    config.forward_port = port;
    config.on_event = onMesh;

    if(IoTBoard::init_espmesh(config)){
        display->println("EspMesh enabled");
    }
    else{
        display->println("EspMesh Error");
    }
    display->display();
}

void loop(){

    buttons->update();
    espmesh->update();

    if(IoTBoard::runEvery(5000) && espmesh->isConnected()){
        String s = "Hello " + String(count_sent++) + " layer " + String(espmesh->getLayer());
        espmesh->sendToRoot(s.c_str(), s.length());
        if(espmesh->isRoot()){
            uint8_t nodes[32][6];
            int count = espmesh->getNodes(nodes, 32);
            Serial.printf("Root, %d nodes in the mesh, IP %s\n", count, espmesh->hasIP() ? "ok" : "no");
        }
    }
}
