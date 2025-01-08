#include "iot_board.h"

Buttons *buttons;
SPIClass *radios_spi;
Display *display;
Zigbee *zigbee;
LoRaClass *lora;
LoRaWANClass *lorawan = &_instance_lorawan;

IoTBoardClass * IoTBoardClass::_instance = nullptr;


const sRFM_pins RFM_pins = {
  	.CS = LORA_CS,
  	.RST = LORA_RST,
  	.DIO0 = LORA_DI0,
  	.DIO1 = LORA_DIO1,
  	.DIO2 = LORA_DIO2,
  	.DIO5 = -1,
};

void IoTBoardClass::init_serial(uint baudrate){
    delay(2000);
    Serial.begin(baudrate);
    Serial.println("[OK]\tSerial Initialised");
    
}

void IoTBoardClass::init_leds(){
    pinMode(LED_YELLOW, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_RGB_B, OUTPUT);
    pinMode(LED_RGB_R, OUTPUT);
    pinMode(LED_RGB_G, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_RGB_B, HIGH);
    digitalWrite(LED_RGB_R, HIGH);
    digitalWrite(LED_RGB_G, HIGH);
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("[OK]\tLEDs Initialised");

}


bool IoTBoardClass::init_display(){

    #ifdef DISPLAY_ENABLED
    display = new Display;
    if(display->init_interface()){
        display->setTextSize(1); 
        display->setTextColor(WHITE);
        display->display();
        Serial.println("[OK]\tDisplay Initialised");
        return true;
    } else {
        Serial.println("[ERROR]\tDisplay Initializing");
        return false;
    }

    #endif
}


void IoTBoardClass::init_spi(uint8_t spiInterface){
    radios_spi = new SPIClass(HSPI);
    radios_spi->setBitOrder(MSBFIRST);
    radios_spi->setDataMode(SPI_MODE0);
    radios_spi->begin(SPI_CLK, SPI_MISO, SPI_MOSI);
    isSPIActivated = true;
    Serial.println("[OK]\tSPI Initialised");
}


bool IoTBoardClass::init_zigbee(){

    if(!isSPIActivated){
        Serial.println("[ERROR][Zigbee] You must enable the SPI firstly. (init_spi)");
        return false;
    }
    zigbee = new Zigbee(radios_spi);
    zigbee->begin();
    isZigbeeActivated = true;
    //attachInterrupt(ZIGBEE_INT, zigbee_interrupt_handler, CHANGE);

    Serial.println("[OK]\tZigbee Initialised");
    return true;

}


bool IoTBoardClass::init_lora(){

    if(!isSPIActivated){
        Serial.println("[ERROR][LoRa] You must enable the SPI firstly. (init_spi)");
        return false;
    }
    
    lora = &LoRa;
    lora->setSPI(*radios_spi);
    lora->setPins(LORA_CS, LORA_RST, LORA_DI0);
    uint count = 0;

    pinMode(ZIGBEE_RST, OUTPUT);
    digitalWrite(ZIGBEE_RST, HIGH);
    
    pinMode(ZIGBEE_CS, OUTPUT);
    digitalWrite(ZIGBEE_CS, HIGH);
    
    pinMode(ZIGBEE_WAKE, OUTPUT);
    digitalWrite(ZIGBEE_WAKE, HIGH);

    while (!lora->begin(866E6)) {
      Serial.print(".");
      delay(500);
      count++;
      if(count >=6){ 
        Serial.println();
        Serial.println("[Error]\tLoRa not Initialised");
        return false;
        }
    }
    Serial.println("[OK]\tLoRa Initialised");
    isLoRaActivated = true;
    return true;
}

void IoTBoardClass::init_buttons(){
    buttons = new Buttons();
    buttons->init_buttons();
    Serial.println("[OK]\tButtons Initialised");
}

bool IoTBoardClass::init_lorawan(){
    if(!isSPIActivated){
        Serial.println("[ERROR][LoRaWAN] You must enable the SPI firstly. (init_spi)");
        return false;
    }

    if(isLoRaActivated){
        Serial.println("[ERROR][LoRaWAN] You can not enable LoRa if you wanna use LoRaWAN");
        return false;
    }

    if(!isZigbeeActivated){
        pinMode(ZIGBEE_RST, OUTPUT);
        digitalWrite(ZIGBEE_RST, HIGH);
        
        pinMode(ZIGBEE_CS, OUTPUT);
        digitalWrite(ZIGBEE_CS, HIGH);
        
        pinMode(ZIGBEE_WAKE, OUTPUT);
        digitalWrite(ZIGBEE_WAKE, HIGH);
    }

    

    if(!lorawan->init(*radios_spi)){
        Serial.println("[ERROR][LoRaWAN] Can not initialise the module");
        return false;
    }
    isLoRaWANActivated = true;
    Serial.println("[OK]\tLoRaWAN Initialised");
    return true;
}
