#include <iot_board.h>

int count = 0;
int count_sent = 0;
const char *str;

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

    IoTBoard::init_zigbee();
    zigbee->setAddress(0x1);
    zigbee->setPanId(0x8888);
    zigbee->setChannel(12);

    display->printf("Zigbee enabled (%04X)\n", zigbee->getAddress());
    display->display();
    delay(500);
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
    count_sent = count_sent + 1;
    if (zigbee->receivePacket()){
        str = zigbee->readString();
        count++;
    }

    if (str != nullptr){
        display->clearDisplay();
        display->setCursor(0, 0);
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
    delay(500);
}
