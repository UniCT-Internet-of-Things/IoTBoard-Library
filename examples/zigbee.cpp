#include <iot_board.h>



void onBtn1Released(uint8_t pinBtn){
    digitalWrite(LED_GREEN, !digitalRead(LED_GREEN));
}

int count = 0;
int count_sent = 0;
rx_info_t* rxInfo = nullptr;


void onZigbeeReceive(){
    if(rxInfo == nullptr)
        rxInfo = zigbee->getRxInfo();
    count = count +1;
}

void onZigbeeTransfer(){
    count_sent = count_sent +1;
}

void interrupt_routine(){
    zigbee->interruptHandler();
}

TaskHandle_t Task1;
TaskHandle_t Task2;
void networkTask(void *pvParameters);
void devicesTask(void *pvParameters);

void setup() {
    
    IoTBoard::init_serial();
    IoTBoard::init_buttons();
    IoTBoard::init_leds();
    IoTBoard::init_display();
    display->println("Display enabled");
    display->display();

    IoTBoard::init_spi();
    display->println("SPI (HSPI) enabled");
    display->display();

    IoTBoard::init_zigbee();
    zigbee->setAddress(0x2);
    zigbee->setPanId(2);
    zigbee->setChannel(12);
    attachInterrupt(ZIGBEE_INT, interrupt_routine, CHANGE);
    display->println("Zigbee enabled");
    display->display();

    if (IoTBoard::init_lora())
    {
        lora->sleep();
        display->println("LoRa enabled");
        display->display();
    }
    else
    {
        display->println("LoRa error");
        display->display();
    }
    

    buttons->onBtn1Release(onBtn1Released);

    xTaskCreatePinnedToCore(
        networkTask, "Network Task"  // A name just for humans
        ,
        2048  // The stack size can be checked by calling `uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);`
        ,
        NULL  // Task parameter which can modify the task behavior. This must be passed as pointer to void.
        ,
        1  // Priority
        ,
        &Task1  // Task handle is not used here - simply pass NULL
        ,
        0  // Core on which the task will run
    );

    xTaskCreatePinnedToCore(
        devicesTask, "Devices Task"  // A name just for humans
        ,
        2048  // The stack size can be checked by calling `uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);`
        ,
        NULL  // Task parameter which can modify the task behavior. This must be passed as pointer to void.
        ,
        2  // Priority
        ,
        &Task2  // Task handle is not used here - simply pass NULL
        ,
        1  // Core on which the task will run
    );
    delay(500);


}

void loop(){
    
}


void devicesTask(void *pvParameters){
    for(;;){
        buttons->update();

        if(rxInfo != nullptr){
            display->clearDisplay();
            display->setCursor(0,0);
            display->println("Received data:");
            for (int i = 0; i < rxInfo->frame_length; i++) {
                display->write(rxInfo->rx_data[i]);
            }
            
            display->print("\r\nLQI/RSSI=");
            display->print(rxInfo->lqi, DEC);
            display->print("/");
            display->println(rxInfo->rssi, DEC);
            display->printf("Count: %d \n", count);
            display->printf("Sent Count: %d \n", count_sent);
            
            display->display();
        }
    }
}

void networkTask(void *pvParameters){
    for(;;){
        zigbee->onEvents(&onZigbeeReceive, &onZigbeeTransfer);
        delay(100);
        zigbee->send(0xffff, "Ciao\0");
        delay(100);
    }
}
