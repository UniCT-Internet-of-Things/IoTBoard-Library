# IoT Board Library

Library for the IoT Board used in the Internet of Things University course of University of Catania, Italy

## What is it?

The Library is a interface to interact with the board of the course and with its periphericals. It is based on Arduino Framework and designed to be loaded into PlatformIO projects.

## Installation

### Using Ready to Go Empty Project

You can clone [this](https://github.com/UniCT-Internet-of-Things/IoTBoard-EmptyProject) repository to start new project using PlatformIO and the library will be installed automatically.

### Install on a new PlatformIO Project

Add the `lib_deps` in the `platformio.ini` file:

```ini
[env:arduino_nano_esp32]
platform = https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip
board = arduino_nano_esp32
framework = arduino
lib_deps = 
	adafruit/Adafruit GFX Library@^1.11.9
	adafruit/Adafruit SSD1306@^2.5.7
    https://github.com/UniCT-Internet-of-Things/IoTBoard-Library
monitor_speed = 115200
board_build.f_cpu = 240000000L
board_build.memory_type = opi_qspi
upload_speed = 921600
board_build.flash_size = 16MB
build_flags = 
	-DARDUINO_USB_CDC_ON_BOOT=1
 	-DCONFIG_SPIRAM_CACHE_WORKAROUND
        -DCORE_DEBUG_LEVEL=1
        -DBOARD_HAS_PSRAM
	-DCONFIG_SPIRAM_USE=1
	-mfix-esp32-psram-cache-issue
        -mfix-esp32-psram-cache-strategy=memw
	-DLORA_ENABLED
	-DZIGBEE_ENABLED
	-DDISPLAY_ENABLED
	-DLEDS_ENABLED
```

## How use it

To use the library with the board you must include it in the main.cpp file of your project.

```cpp
#include <iot_board.h>

void setup(){
}

void loop(){
}
```

The Library servers as static interface the IoTBoard interface with with you can initialise the devices on the board:

### Serial (UART)

To enable the Serial you must call the `init_serial()` function:

```cpp
IoTBoard::init_serial();
```

From this point you can use the UART by using the classic Serial class of Arduino Framework:

```cpp
void setup(){
    IoTBoard::init_serial();
    Serial.println("Loaded");  
}
```

### Buttons

On the board are placed two programmable buttons, defined by the pins `BTN_1` and `BTN_2`. To be enabled you must call the `init_buttons()` function and also update the status ofthe button by the main `loop`:

```cpp
void setup(){
    IoTBoard::init_buttons();
}

void loop(){
    buttons->update();
}
```

After the initialisation, the buttons pointer will be avaiable to manage the release of each button, by using the methods `onBtn1Release` and `onBtn2Release` to register the callback function for each button:

```cpp
void onBtn1Released(uint8_t pinBtn){
    Serial.println("BTN 1 pressed");
}

void onBtn2Released(uint8_t pinBtn){
    Serial.println("BTN 2 pressed");
}

void setup(){
    IoTBoard::init_serial();
    IoTBoard::init_buttons();
    buttons->onBtn1Release(onBtn1Released);
    buttons->onBtn2Release(onBtn2Released);
}

void loop(){
    buttons->update();
}
```

### LEDs

On board there are tree single LED, red, yellow and green LEDs (identified by the pins `LED_RED`, `LED_YELLOW` and `LED_GREEN`) and one RGB Led. The RGB LED is identified by tree pins, each one for each channel (`LED_RGB_R`, `LED_RGB_G`, `LED_RGB_B`). To initialise the pins you must call the function `init_leds()`:

```cpp
void setup(){
    IoTBoard::init_leds();
}

void loop(){
}
```

Now you can use the classic `digitalWrite` or `digitalRead` on the LED pins.

```cpp
void setup(){
    IoTBoard::init_leds();
    digitalWrite(LED_RED, HIGH);
}

void loop(){
}
```

### Display

The IoT Board is equiped (if present) with a I2C OLED Display (128x64 pixels). To be used, you must init the device with the `init_display()` function:

```cpp

void setup(){
    IoTBoard::init_display();
}

void loop(){
}
```

From this point you can access to the `display` pointer and use the classic *Adafruit SSD1306* API:

```cpp

void setup(){
    IoTBoard::init_display();
    display->clearDisplay();
    display->println("Hello!");
    display->display();
}

void loop(){
}
```

### SPI

The IoT Board has two SPI interface, (vSPI and hSPI). The usage of other devices on board requires the initialisation of hSPI then you must call the function `init_spi()` to init it:

```cpp

void setup(){
    IoTBoard::init_spi();
}

void loop(){
}
```

From this point, the other external devices can be used.

### Zigbee

```cpp
#include <iot_board.h>

const char *str;

void setup()
{

    IoTBoard::init_spi();
    IoTBoard::init_zigbee();

    zigbee->setAddress(0x1);
    zigbee->setPanId(0x8888);
    zigbee->setChannel(12);
}

void loop()
{
    String s = "Hello " + String(count_sent);
    zigbee->startPacket();
    zigbee->writeString((char *)s.c_str());
    zigbee->sendPacket(0xffff, false);
    while (!zigbee->transmissionSuccess()){
        delay(1);
    }
    if (zigbee->receivePacket()){
        str = zigbee->readString();
    }


}
```

### LoRa

```cpp
#include <iot_board.h>

void onLoRaReceive(int packetSize)

void setup(){

    IoTBoard::init_spi();

    if (IoTBoard::init_lora()){
        lora->onReceive(onLoRaReceive);
        lora->receive();
    }
}

void loop()
{
    String s = "Hello " + String(count_sent);
    lora->beginPacket();
    lora->write(destination);              // add destination address
    lora->write(localAddress);             // add sender address
    lora->write(count_sent);                 // add message ID
    lora->write(s.length());        // add payload length
    lora->write((const uint8_t *)s.c_str(), s.length());                // add payload

    lora->endPacket(); // true = async / non-blocking mode
    lora->receive();
    delay(5000);
}

void onLoRaReceive(int packetSize)
{
    if (packetSize == 0)
        return; // if there's no packet, return

    // read packet header bytes:
    uint16_t recipient = lora->read();       // recipient address
    uint16_t sender = lora->read();         // sender address
    byte incomingMsgId = lora->read();  // incoming msg ID
    byte incomingLength = lora->read(); // incoming msg length

    String incoming = ""; // payload of packet

    while (lora->available())
    {                                  // can't use readString() in callback, so
        incoming += (char)lora->read(); // add bytes one by one
    }

    if (incomingLength != incoming.length())
    { // check length for error
        return; // skip rest of function
    }
}
```

### LoRaWAN

```cpp
#include <iot_board.h>

const char *devAddr = "00000000000000000000000000000000";
const char *nwkSKey = "00000000000000000000000000000000";
const char *appSKey = "00000000000000000000000000000000";

char myStr[50];
char outStr[255];
int count_sent = 0;
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


void onLoRaReceive (sBuffer *Data_Rx, bool isConfirmed, uint8_t fPort){
    String s = String((char *) Data_Rx->Data, HEX);
}

void setup()
{


    IoTBoard::init_spi();
    
    if(IoTBoard::init_lorawan()){
        
        lorawan->setDeviceClass(CLASS_A);
        lorawan->setDataRate(SF9BW125);
        // set channel to random
        lorawan->setChannel(MULTI);
        // Put ABP Key and DevAddress here
        lorawan->setNwkSKey(nwkSKey);
        lorawan->setAppSKey(appSKey);
        lorawan->setDevAddr(devAddr);
        
        lorawan->onMessage(&onLoRaReceive);

    } 

}


void loop()
{

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
```
