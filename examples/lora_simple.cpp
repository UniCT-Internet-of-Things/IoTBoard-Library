#include <iot_board.h>

int count = 0;
int count_sent = 0;
bool displayNeedUpdate = false;

uint16_t localAddress = 0x01;     // address of this device
uint16_t destination = 0xFF;      // destination to send to

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
        Serial.println("error: message length does not match length");
        return; // skip rest of function
    }

    count++;
    display->clearDisplay();
    display->setCursor(0,0);
    display->printf("From: %04X\n", sender);
    display->printf("Sent to: %04X\n", recipient);
    display->printf("Message ID: %d\n", incomingMsgId);
    display->printf("Count: %d\n", count);
    display->printf("Message length: %d\n", incomingLength);
    display->printf("Message: %s\n", incoming);
    display->printf("RSSI: %d\n", lora->packetRssi());
    display->printf("Snr: %02f\n", lora->packetSnr());
    displayNeedUpdate = true;
}

void setup(){

    IoTBoard::init_serial();
    IoTBoard::init_buttons();
    IoTBoard::init_leds();
    IoTBoard::init_display();
    display->println("Display enabled");
    display->display();

    IoTBoard::init_spi();
    display->println("SPI (HSPI) enabled");
    display->display();

    if (IoTBoard::init_lora()){
        lora->onReceive(onLoRaReceive);
        lora->receive();
        display->println("LoRa enabled");
    }
    else{
        display->printf("LoRa Error");
    }

    display->display();
    delay(500);
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
    count_sent++;
    lora->receive();
    if(displayNeedUpdate){
        displayNeedUpdate = false;
        display->display();
    }
    delay(5000);
}