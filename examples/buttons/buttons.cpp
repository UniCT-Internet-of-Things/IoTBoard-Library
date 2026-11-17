#include <iot_board.h>



void onBtn1Released(uint8_t pinBtn){
    digitalWrite(LED_GREEN, !digitalRead(LED_GREEN));
}

void onBtn2Pressed(uint8_t pinBtn){
    Serial.println("BTN 2 pressed");
}

void onBtn2Released(uint8_t pinBtn){
    Serial.println("BTN 2 released");
}

void setup() {
    IoTBoard::init_serial();
    IoTBoard::init_buttons();
    IoTBoard::init_leds();
    IoTBoard::init_display();
    display->println("Display enabled");
    display->display();

    buttons->onBtn1Release(onBtn1Released);
    buttons->onBtn2Press(onBtn2Pressed);
    buttons->onBtn2Release(onBtn2Released);
}

void loop(){
    digitalWrite(LED_RED, buttons->isBtn1Pressed());
    delay(10);
}


