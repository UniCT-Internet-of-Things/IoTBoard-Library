#include <iot_board.h>

int count = 0;
int count_sent = 0;
bool displayNeedUpdate = false;

const char *devAddr = "260B3E70";
const char *nwkSKey = "AF0D4340E12A7333A23BB42D239C2C5A";
const char *appSKey = "909745BCD7A80913AAAD9138B8D7FAA7";

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
    String s = String((char *) Data_Rx->Data, HEX);
    Serial.println(s);
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
        
        // Put ABP Key and DevAddress here
        lorawan->setNwkSKey(nwkSKey);
        lorawan->setAppSKey(appSKey);
        lorawan->setDevAddr(devAddr);
        
        lorawan->onMessage(&onLoRaReceive);

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
        lorawan->sendUplink(myStr, strlen(myStr), 1, 2);
        count_sent++;
    }


    recvStatus = lorawan->readData(outStr);

    
  // Check Lora RX
    lorawan->update();
    delay(1);
}