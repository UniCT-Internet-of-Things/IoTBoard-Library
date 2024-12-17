#include <iot_board.h>
#include <atomic.h>

void onBtn1Released(uint8_t pinBtn){
    digitalWrite(LED_GREEN, !digitalRead(LED_GREEN));
}

int count = 0;
int count_sent = 0;
volatile bool sent = false;
const char* str;
volatile bool packetReceived = false;
volatile bool canReceive = false;

void onZigbeeReceive(){
    
    if(sent){
        canReceive = true;
    }
}

void onZigbeeTransfer(){
    count_sent = count_sent +1;
    sent = true;
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
    zigbee->setAddress(0x3);
    zigbee->setPanId(0x8888);
    zigbee->setChannel(12);

    attachInterrupt(ZIGBEE_INT, onZigbeeReceive, FALLING);
    
    display->printf("Zigbee enabled (%04X)\n", zigbee->getAddress());
    display->display();

    

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
        bool ready;

        buttons->update();
         if(str != nullptr){
            display->clearDisplay();
            display->setCursor(0,0);
            display->printf("[%04X] Received:\n", zigbee->getAddress());
            display->println(str);
            

            display->print("LQI/RSSI=");
            display->print(zigbee->getRXPacket()->lqi, DEC);
            display->print("/");
            display->println(int(zigbee->getRXPacket()->rssi), DEC);
            display->printf("Source: %04X\n", zigbee->getRXPacket()->src_addr);
            display->printf("Dest: %04X\n", zigbee->getRXPacket()->dest_addr);
            display->printf("Count: %d \n", count);
            display->printf("Sent Count: %d \n", count_sent);
            
            display->display();
        }
        delay(1);
        
    }
}

void networkTask(void *pvParameters){
    for(;;){
        if(!sent){
            canReceive = false;
            String s = "Ciao "+String(count_sent);
            zigbee->startPacket();
            zigbee->writeString((char *)s.c_str());
            zigbee->sendPacket(0xffff, false);
            while(!zigbee->transmissionSuccess()){
                delay(10);
            }
            onZigbeeTransfer();

  
        
            delay(500);
        } 
        if(canReceive && zigbee->receivePacket()){
            str = zigbee->readString();
            count++;
            sent = false;
        }
        delay(1);
        
    }
}
