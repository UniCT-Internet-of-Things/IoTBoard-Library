#include <iot_board.h>

int count_zigbee = 0;
int count_lora = 0;

void onLoraReceive(int packetSize);
void handle_rx();
void handle_tx();
void interrupt_routine();
void loraSend(String outgoing);


long lastSendTime = 0;    // last send time
int interval = 2000;      // interval between sends

void setup()
{
    IoTBoard::init_serial();

    IoTBoard::init_leds();

    IoTBoard::init_display();
    display->println("Display enabled");
    display->display();

    IoTBoard::init_spi();
    display->println("SPI (HSPI) enabled");
    display->display();

    IoTBoard::init_zigbee();
    zigbee->setAddress(0x1234);
    zigbee->setChannel(12);
    attachInterrupt(ZIGBEE_INT, interrupt_routine, CHANGE);
    display->println("Zigbee enabled");
    display->display();

    if (IoTBoard::init_lora())
    {
        lora->onReceive(onLoraReceive);
        lora->receive();
        display->println("LoRa enabled");
        display->display();
    }
    else
    {
        display->println("LoRa error");
        display->display();
    }
    
}

void loop()
{
    zigbee->checkFlags(&handle_rx, &handle_tx);
    Serial.println("Sending zigbee");
    zigbee->send(0xffff, "hello");
    delay(1000);

    if (millis() - lastSendTime > interval)
    {
        String message = "HeLoRa World!"; // send a message
        loraSend(message);
        Serial.println("Sending " + message);
        lastSendTime = millis();        // timestamp the message
        interval = random(2000) + 1000; // 2-3 seconds
        lora->receive();                 // go back into receive mode
    }

    display->clearDisplay();
    display->display();
    display->setCursor(0, 0);
    display->printf("LoRa Received: %d\n", count_lora);
    display->printf("Zigbee Received: %d\n", count_zigbee);
    display->display();
}

void interrupt_routine()
{
    zigbee->interruptHandler(); // mrf24 object interrupt routine
}

void onLoraReceive(int packetSize)
{
    count_lora = count_lora + 1;
    Serial.println("Lora received");
}

void loraSend(String outgoing)
{
    lora->beginPacket();            // start packet
    lora->print("ciao");          // add payload
    lora->endPacket();              // finish packet and send it
}

void handle_rx()
{
    count_zigbee = count_zigbee + 1;
    Serial.println("Zigbee received");
}

void handle_tx()
{
    // code to transmit, nothing to do
    Serial.println("Zigbee sent");
}
