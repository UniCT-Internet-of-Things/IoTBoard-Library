#include <iot_board.h>

int count = 0;
int count_sent = 0;
bool displayNeedUpdate = false;


// OTAA credentials
const char *devEui = "70B3D57ED006D4DB";
const char *appEui = "0000000000000000";
const char *appKey = "A28C4078E4328DFDC1278D8A3E5195F8";


char myStr[50];
char outStr[255];

byte recvStatus = 0;


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

void onBtn1Released(uint8_t pinBtn){
    digitalWrite(LED_GREEN, !digitalRead(LED_GREEN));
}

void onLoRaReceive (sBuffer *Data_Rx, bool isConfirmed, uint8_t fPort){
    Serial.println("Ricevuto qualcosa");
    Serial.println(isConfirmed);
    Serial.println(fPort);
    for(int i = 0; i < Data_Rx->Counter; i++){
      Serial.print(Data_Rx->Data[i], HEX);
    }
    Serial.println();
}

void setup()
{

    IoTBoard::init_serial();
    IoTBoard::init_buttons();
    IoTBoard::init_leds();
    IoTBoard::init_display();
    display->println("Display enabled");
    display->display();

    IoTBoard::init_spi();
    display->println("SPI (HSPI) enabled");
    display->display();
    
    if(IoTBoard::init_lorawan()){
        
        display->println("LoRaWAN enabled");
        lorawan->setDeviceClass(CLASS_A);
        lorawan->setDataRate(SF9BW125);

        // set channel to random
        lorawan->setChannel(MULTI);

        lorawan->setDevEUI(devEui);
        lorawan->setAppEUI(appEui);
        lorawan->setAppKey(appKey);
        lorawan->onMessage(&onLoRaReceive);


          // Join procedure
        bool isJoined;
        do {
          Serial.println("Joining...");
          isJoined =  lorawan->join();
          
          //wait for 10s to try again
          delay(10000);
        }while(!isJoined);
        Serial.println("Joined to network");

    } else {
        display->println("LoRaWAN error");
    }
    display->display();

    buttons->onBtn1Release(onBtn1Released);
    delay(500);
}


void loop()
{

    buttons->update();

    if(runEvery(10000)){
        sprintf(myStr, "Hello-%d", count_sent); 
        lorawan->sendUplink(myStr, strlen(myStr), 0, 1);
        count_sent++;
    }


    //lorawan->update();

 

    recvStatus = lorawan->readData(outStr);

    
  // Check Lora RX
    lorawan->update();
    //delay(1);
}