/**
 * This is our new Arduino library to use the 2.4GHz MRF24J40 wireless radio from Microchip.
 * It supports communication using the 802.15.4 wireless protocol at the MAC level, and enables
 * use of the MRF24J40MA/B/C/D/E wireless modules from Microchip.
 *
 * Copyright (c) 2015 Circuitar
 * This software is released under the MIT license. See the attached LICENSE file for details.
 *
 * IoT Board version: interrupt driven (ESP32 / FreeRTOS), RX queue, TX status, MAC header parsing.
*/
#ifndef NANOSHIELD_MRF_h
#define NANOSHIELD_MRF_h

#include <Arduino.h>
#include <SPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <functional>

// Driver model
//
// The MRF24J40 has one interrupt status register (INTSTAT) that is cleared when it is read, and
// an RX FIFO that holds one frame only: until the host reads it, the radio neither receives nor
// acknowledges other frames. If several functions read INTSTAT (one waiting for the end of a TX,
// another one checking for RX) they steal each other's flags, and an unread RX FIFO makes the
// radio deaf.
//
// Here the radio INT pin wakes a service task, the only code that reads INTSTAT:
//   - TXNIF: it reads TXSTAT and stores the result of the last transmission;
//   - RXIF:  it moves the frame from the RX FIFO to a queue, so the radio can receive again.
// The task sleeps between interrupts. The interrupt routine does not touch SPI: the bus is shared
// with the LoRa radio and the SPI lock cannot be taken in an interrupt.
// receivePacket(), transmissionDone() and transmissionSuccess() only read this state.
//
// With onReceiveCallback() the frames are not left in the queue for receivePacket(): a second
// task takes them and calls the user function, one frame at a time. A slow callback does not
// stop the radio (frames wait in the queue), but it runs in parallel with loop(): keep it short
// and do not use from both places what is not thread safe (e.g. the display).

// Packet Transmission Details
//
// 802.15.4 MAC Data Frame (max size = max PSDU size = 127)
// -------------------------------------------------------------
// | MAC Header (MHR) | Data Payload (MSDU) | MAC Footer (MFR) |
// -------------------------------------------------------------
// |        9         |       1 - 116       |         2        |
// -------------------------------------------------------------
//
// 802.15.4 MAC Header (MHR)
// -------------------------------------------------------
// | Frame Control | Sequence Number | Addressing Fields |
// -------------------------------------------------------
// |       2       |        1        |         6         |
// -------------------------------------------------------
//
// 802.15.4 Addressing Fields (16-bit addressing with PAN ID)
// -------------------------------------------------
// | PAN ID | Destination Address | Source Address |
// -------------------------------------------------
// |    2   |          2          |        2       |
// -------------------------------------------------
//
// MRF24J40 TX FIFO
// ---------------------------------------------------------------------
// | Header Length | Frame Length | Header (MHR) | Data Payload (MSDU) |
// ---------------------------------------------------------------------
// |       1       |       1      |       9      |       1 - 116       |
// ---------------------------------------------------------------------

#define MRF_PAN_ID_SIZE 2
#define MRF_DEST_ADDR_SIZE 2
#define MRF_SRC_ADDR_SIZE 2
#define MRF_ADDR_FIELDS_SIZE (MRF_PAN_ID_SIZE + MRF_DEST_ADDR_SIZE + MRF_SRC_ADDR_SIZE)
#define MRF_MHR_SIZE (2 + 1 + MRF_ADDR_FIELDS_SIZE)
#define MRF_MFR_SIZE 2
#define MRF_MAX_PSDU_SIZE 127
#define MRF_MAX_PAYLOAD_SIZE (MRF_MAX_PSDU_SIZE - MRF_MHR_SIZE - MRF_MFR_SIZE)

// Packet Reception Details
//
// MRF24J40 RX FIFO
// ------------------------------------------------------------------------
// | Frame Length | Header (MHR) | Data Payload (MSDU) | FCS | LQI | RSSI |
// ------------------------------------------------------------------------
// |       1      |       9      |       1 - 116       |  2  |  1  |   1  |
// ------------------------------------------------------------------------

#define MRF_MAX_RX_FIFO_SIZE (1 + MRF_MHR_SIZE + MRF_MAX_PAYLOAD_SIZE + 2 + 1 + 1)

// Frames kept by the driver until the sketch takes them with receivePacket().
// When the queue is full the oldest frame is dropped (see droppedPackets()).
#ifndef MRF_RX_QUEUE_SIZE
#define MRF_RX_QUEUE_SIZE 8
#endif

// MAC frame types (frame control bits 0-2)
#define MRF_FRAME_BEACON  0
#define MRF_FRAME_DATA    1
#define MRF_FRAME_ACK     2
#define MRF_FRAME_COMMAND 3

typedef struct _rx_info_t{
    uint8_t payload_length;
    int lqi;                       // link quality indicator, 0-255
    int rssi;                      // received power, dBm
    uint16_t src_addr;             // 16-bit source address (0xFFFE: 64-bit address or none)
    uint16_t dest_addr;            // 16-bit destination address (0xFFFF: broadcast)
    uint16_t panid;
    uint8_t seq;                   // MAC sequence number
    uint8_t frame_type;            // MRF_FRAME_*
    uint8_t rssi_raw;              // RSSI register value, 0-255
    uint8_t rx_data[MRF_MAX_PSDU_SIZE];
} __attribute__((packed)) rx_info_t;

/**
 * Based on the TXSTAT register, but "better"
 */
typedef struct _tx_info_t{
    uint8_t tx_ok:1;
    uint8_t retries:2;
    uint8_t channel_busy:1;
} tx_info_t;

// Receive callback: packet->rx_data / payload_length is the payload, the other fields the MAC
// details. Inside the callback read*() and readString() read the same payload.
typedef std::function<void(rx_info_t* packet)> ZigbeeReceiveCallback;

enum Mrf24j40Type { MRF24J40MA, MRF24J40MB, MRF24J40MC, MRF24J40MD, MRF24J40ME };

class Nanoshield_MRF
{
  public:

    // intPin: host pin wired to the radio INT pin
    Nanoshield_MRF(Mrf24j40Type type, int cs, SPIClass * spi, int intPin);
    // false if the INT pin does not look connected: the driver can not work without it
    bool begin(void);
    void setPanId(uint16_t panId);
    void setAddress(uint16_t addr);
    uint16_t getPanId();
    uint16_t getAddress();
    void setCoordinator(bool coord);
    void setChannel(int channel);
    int getChannel();
    float measureSignalStrength();
    float getSignalStrength();
    int getLinkQuality();
    void sleep();
    void wakeup();
    void setPromiscuous(boolean enabled);

    bool write(uint8_t b);
    uint8_t read();
    bool writeInt(int16_t i);
    int16_t readInt();
    bool writeUnsignedInt(uint16_t u);
    uint16_t readUnsignedInt();
    bool writeLong(int32_t l);
    int32_t readLong();
    bool writeUnsignedLong(uint32_t u);
    uint32_t readUnsignedLong();
    bool writeFloat(float f);
    float readFloat();
    int writeBytes(uint8_t* buf, int size);
    int readBytes(uint8_t* buf, int size);
    int writeString(char* str);
    const char* readString(char* str = NULL, int size = 0);

    void startPacket();
    int bytesLeftToWrite();
    // Starts the transmission; if the previous one is still running, waits for it first.
    bool sendPacket(uint16_t addr, bool ack = false);
    // true when the last transmission has ended (also true if nothing was sent)
    bool transmissionDone();
    // true if the last transmission has ended and succeeded (ACK received, if requested)
    bool transmissionSuccess();
    // retransmissions used by the last transmission (0-3)
    int getRetries();
    tx_info_t getTXInfo();

    // Takes the next frame from the queue: read*() then read its payload.
    bool receivePacket();
    int bytesLeftToRead();
    // frames waiting in the queue
    int packetsAvailable();
    // frames dropped because the queue was full
    uint32_t droppedPackets();
    // details of the frame taken by the last receivePacket()
    rx_info_t* getRXPacket();
    uint16_t getSourceAddress();
    uint16_t getDestinationAddress();
    // Calls callback for every received frame (nullptr: back to receivePacket()).
    void onReceiveCallback(ZigbeeReceiveCallback callback);

  private:
    static SPISettings spiSettings;
    static Nanoshield_MRF* instance;

    int cs;
    int intPin;
    Mrf24j40Type type;
    uint16_t panId;
    uint16_t srcAddr;
    int channel;
    bool promiscuous;
    uint8_t seqNumber;
    SPIClass * _spi;
    int txCount;
    uint8_t txBuf[MRF_MAX_PAYLOAD_SIZE];
    int rxCount;
    int rxSize;
    uint8_t* rxBuf;
    uint8_t rssi;
    uint8_t lqi;

    // state shared with the service task (protected by lock)
    SemaphoreHandle_t lock;
    TaskHandle_t task;
    volatile bool txBusy;
    volatile uint8_t txStatus;
    rx_info_t rxQueue[MRF_RX_QUEUE_SIZE];
    int rxHead;
    int rxTail;
    int rxQueued;
    uint32_t rxDropped;
    rx_info_t current;
    ZigbeeReceiveCallback rxCallback;
    TaskHandle_t callbackTask;

    void reset();
    void init();
    void enablePaLna();
    void take();
    void give();
    void service();
    void readRxFifo();
    static void serviceTask(void* arg);
    static void callbackTaskLoop(void* arg);
    static void IRAM_ATTR onInterrupt();
    bool intPinDriven();
    uint8_t readShort(uint8_t address);
    void writeShort(uint8_t address, uint8_t data);
    uint8_t readLong(uint16_t address);
    void writeLong(uint16_t address, uint8_t data);
    bool writeToBuffer(void* data, int size);
    bool readFromBuffer(void* data, int size);
};

#endif
