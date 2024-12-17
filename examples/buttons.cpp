#include <iot_board.h>



void onBtn1Released(uint8_t pinBtn){
    digitalWrite(LED_GREEN, !digitalRead(LED_GREEN));
}

void setup() {
    
    IoTBoard::init_serial();
    IoTBoard::init_buttons();
    IoTBoard::init_leds();
    IoTBoard::init_display();
    display->println("Display enabled");
    display->display();
    

    buttons->onBtn1Release(onBtn1Released);

}

void loop(){
    buttons->update();
    delay(1);
}


