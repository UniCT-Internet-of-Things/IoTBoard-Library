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
platform = espressif32
board = arduino_nano_esp32
framework = arduino
lib_deps = 
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
	-DLORA_ENABLED
	-DZIGBEE_ENABLED
	-DDISPLAY_ENABLED
	-DLEDS_ENABLED
	-DESPMESH_ENABLED
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

### Run Every

The `runEvery()` function returns `true` once every `interval` milliseconds, without blocking the `loop` like `delay()`. The library provides 8 independent timers, selected by the second parameter (default `0`), so different intervals in the same sketch must use different timers:

```cpp
void loop(){
    if(IoTBoard::runEvery(1000)){
        Serial.println("every second");
    }
    if(IoTBoard::runEvery(5000, 1)){
        Serial.println("every 5 seconds");
    }
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


### EspMesh

The IoT Board can be part of an [ESP-WIFI-MESH] network. The nodes organise themselves in a tree: one of them is elected as root and it is the only one connected to the Wi-Fi router, the others reach it through the mesh. To be used, you must add the `-DESPMESH_ENABLED` build flag, call the `init_espmesh()` function with the router credentials and also update the mesh by the main `loop`:

```cpp
void setup(){
    IoTBoard::init_espmesh("router-ssid", "router-password");
}

void loop(){
    espmesh->update();
}
```

All the nodes must run the same firmware, since any of them can be elected as root.

To receive the messages and the status of the mesh you must register a callback function. The callback is called by `espmesh->update()`, so the display, the LEDs and the other devices can be used inside it:

```cpp
void onMesh(const EspMeshEvent &event){
    switch(event.type){
        case EspMeshEventType::Connected:
            digitalWrite(LED_GREEN, HIGH);
            break;
        case EspMeshEventType::Disconnected:
            digitalWrite(LED_GREEN, LOW);
            break;
        case EspMeshEventType::Message:
            Serial.printf("From " MACSTR ": %s\n", MAC2STR(event.from), (char *) event.data);
            break;
        default:
            break;
    }
}

void setup(){
    IoTBoard::init_serial();
    IoTBoard::init_leds();
    IoTBoard::init_espmesh("router-ssid", "router-password", onMesh);
}

void loop(){
    espmesh->update();
}
```

The available events are:

- `Connected` and `Disconnected`: the node joined or left the mesh
- `BecameRoot`: the node has been elected as root
- `GotIP`: the root obtained the IP from the router
- `ChildJoined` and `ChildLeft`: a node connected to this one or left it (`event.from` is its MAC)
- `Message`: a message has been received, `event.from` is the MAC of the sender, `event.data` the payload (null terminated) and `event.len` its length

From this point you can access to the `espmesh` pointer to send messages. Each node is identified by its MAC address:

```cpp
void loop(){
    espmesh->update();

    if(IoTBoard::runEvery(5000) && espmesh->isConnected()){
        String s = "Hello from layer " + String(espmesh->getLayer());
        espmesh->sendToRoot(s.c_str(), s.length());
    }
}
```

The available methods to send messages are:

```cpp
espmesh->sendToRoot(data, len);                 // to the root
espmesh->sendTo(mac, data, len);                // to the node with that MAC
espmesh->broadcast(data, len);                  // to all the nodes
espmesh->sendToGroup(group, data, len);         // to the nodes that called joinGroup(group)
espmesh->sendToServer(ip, port, data, len);     // outside the mesh, via the root (UDP)
```

The group ID is a multicast MAC address, e.g. `01:00:5E:00:00:01`, and a node joins it with `espmesh->joinGroup(group)`. The maximum length of a message is 1472 bytes.

The other methods are `isRoot()`, `isConnected()`, `hasIP()`, `getLayer()`, `getMac(mac)` and `getNodes(nodes, maxNodes)` (on the root it returns the MAC of all the nodes of the mesh).

To change the default values you must use the `EspMeshConfig` structure:

```cpp
void setup(){
    EspMeshConfig config;
    config.router_ssid = "router-ssid";
    config.router_pass = "router-password";
    config.mesh_id[5] = 0x01;                   // default 77:77:77:77:77:77
    config.mesh_pass = "my-mesh-password";      // default "iot-lab-board-pass"
    config.max_layer = 4;                       // default 6
    config.max_children = 4;                    // default 6
    config.role = EspMeshRole::Auto;            // Auto (election), Root or Node
    config.on_event = onMesh;

    IoTBoard::init_espmesh(config);
}
```

The root can also forward the messages outside the mesh. By setting `forward_ip` and `forward_port`, the root sends to that server via UDP every message it receives, also the ones sent by itself with `sendToRoot()`, in this way the messages reach the server even after a new root is elected:

```cpp
void setup(){
    EspMeshConfig config;
    config.router_ssid = "router-ssid";
    config.router_pass = "router-password";
    config.forward_ip = IPAddress(192, 168, 1, 10);
    config.forward_port = 5000;

    IoTBoard::init_espmesh(config);
}
```

To receive the messages on the PC you can use netcat (`-k` is required to accept messages from a new root):

```bash
nc -kul 5000
```

The mesh uses the Wi-Fi interface of the board, so it can not be used together with the `WiFi` class (`WiFi.begin()`). Once the root has the IP, the libraries based on sockets (`WiFiClient`, `HTTPClient`, MQTT) can be used on the root.

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

