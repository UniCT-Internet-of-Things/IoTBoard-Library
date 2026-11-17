#include <iot_board.h>

#define MY_ID   0x0001     
#define PAN_ID  0x8888      
#define CHANNEL 15         

volatile uint32_t received = 0;
volatile uint16_t lastFrom = 0;
volatile int lastRssi = 0;


void onZigbeeReceive(rx_info_t *packet) {
    const char *text = zigbee->readString();
    
    Serial.printf("from %04X to %04X PAN %04X seq %u | %d dBm LQI %d | %u B: %s\n",
                  packet->src_addr, packet->dest_addr, packet->panid, packet->seq,
                  packet->rssi, packet->lqi, packet->payload_length, text);
    lastFrom = packet->src_addr;
    lastRssi = packet->rssi;
    received++;
}

void setup() {
    IoTBoard::init_serial();
    IoTBoard::init_display();
    IoTBoard::init_spi();
    if (!IoTBoard::init_zigbee()) {
        display->println("Zigbee: INT not wired");
        display->display();
        while (true) delay(1000);
    }
    zigbee->setPanId(PAN_ID);
    zigbee->setAddress(MY_ID);
    zigbee->setChannel(CHANNEL);
    zigbee->onReceiveCallback(onZigbeeReceive);
}

void loop() {
    static uint16_t sent = 0;

    if (IoTBoard::runEvery(2000)) {
        char msg[16];
        snprintf(msg, sizeof(msg), "Hello %u", sent++);
        zigbee->startPacket();
        zigbee->writeString(msg);
        zigbee->sendPacket(0xFFFF);              // broadcast, no ACK
    }

    if (IoTBoard::runEvery(500, 1)) {
        display->clearDisplay();
        display->setCursor(0, 0);
        display->printf("Node %04X ch %d\n", MY_ID, CHANNEL);
        display->printf("Sent     %u\n", sent);
        display->printf("Received %lu\n", received);
        display->printf("Last %04X %d dBm\n", lastFrom, lastRssi);
        display->printf("Dropped  %lu\n", zigbee->droppedPackets());
        display->display();
    }
}
