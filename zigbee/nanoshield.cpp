/**
 * This is our new Arduino library to use the 2.4GHz MRF24J40 wireless radio from Microchip.
 * It supports communication using the 802.15.4 wireless protocol at the MAC level, and enables
 * use of the MRF24J40MA/B/C/D/E wireless modules from Microchip.
 *
 * Copyright (c) 2015 Circuitar
 * This software is released under the MIT license. See the attached LICENSE file for details.
 *
 * IoT Board version: interrupt driven, see "Driver model" in nanoshield.h.
*/
#include "nanoshield.h"

#define MRF_RXMCR    0x00
#define MRF_PANIDL   0x01
#define MRF_PANIDH   0x02
#define MRF_SADRL    0x03
#define MRF_SADRH    0x04
#define MRF_RXFLUSH  0x0D
#define MRF_TXNCON   0x1B
#define MRF_PACON2   0x18
#define MRF_WAKECON  0x22
#define MRF_TXSTAT   0x24
#define MRF_SOFTRST  0x2A
#define MRF_TXSTBL   0x2E
#define MRF_INTSTAT  0x31
#define MRF_INTCON   0x32
#define MRF_GPIO     0x33
#define MRF_TRISGPIO 0x34
#define MRF_SLPACK   0x35
#define MRF_RFCTL    0x36
#define MRF_BBREG1   0x39
#define MRF_BBREG2   0x3A
#define MRF_BBREG6   0x3E
#define MRF_CCAEDTH  0x3F
#define MRF_RFCON0   0x200
#define MRF_RFCON1   0x201
#define MRF_RFCON2   0x202
#define MRF_RFCON6   0x206
#define MRF_RFCON7   0x207
#define MRF_RFCON8   0x208
#define MRF_RFSTATE  0x20F
#define MRF_RSSI     0x210
#define MRF_SLPCON0  0x211
#define MRF_SLPCON1  0x220
#define MRF_TESTMODE 0x22F
#define MRF_RXFIFO   0x300

#define MRF_INTSTAT_TXNIF 0b00000001
#define MRF_INTSTAT_RXIF  0b00001000
#define MRF_TXSTAT_TXNSTAT 0b00000001
#define MRF_TXSTAT_CCAFAIL 0b00100000

#define MRF_TASK_STACK    4096
#define MRF_TASK_PRIORITY 3
#define MRF_CB_TASK_STACK 8192    // the user callback runs here: room for printf, display...
#define MRF_CB_TASK_PRIORITY 2    // below the radio task, above loop() (priority 1)
#define MRF_TX_WAIT_MS    100     // longest wait for the previous TX in sendPacket()

SPISettings Nanoshield_MRF::spiSettings = SPISettings(8000000, MSBFIRST, SPI_MODE0);
Nanoshield_MRF* Nanoshield_MRF::instance = nullptr;

Nanoshield_MRF::Nanoshield_MRF(Mrf24j40Type type, int cs, SPIClass * spi, int intPin) {
  this->cs = cs;
  this->intPin = intPin;
  this->type = type;
  this->panId = 0;
  this->srcAddr = 1;
  this->channel = 11;
  this->promiscuous = false;
  this->seqNumber = 0;
  this->txCount = 0;
  this->rxSize = 0;
  this->rxCount = 0;
  this->rssi = 0;
  this->lqi = 0;
  this->_spi = spi;
  this->lock = nullptr;
  this->task = nullptr;
  this->txBusy = false;
  this->txStatus = 0;
  this->rxHead = 0;
  this->rxTail = 0;
  this->rxQueued = 0;
  this->rxDropped = 0;
  this->rxCallback = nullptr;
  this->callbackTask = nullptr;
  memset(&current, 0, sizeof(current));
  this->rxBuf = current.rx_data;
}

bool Nanoshield_MRF::begin() {
  pinMode(cs, OUTPUT);
  digitalWrite(cs, HIGH);
  if (lock == nullptr) {
    lock = xSemaphoreCreateRecursiveMutex();
  }
  // The radio RESET pin is not wired and the module stays powered across ESP32 resets: clear
  // whatever the previous sketch left (sleep, coordinator or no-ACK mode, pending frames).
  reset();
  delay(2);
  init();

  if (intPin < 0 || !intPinDriven()) {
    return false;
  }

  instance = this;
  pinMode(intPin, INPUT_PULLUP);
  if (task == nullptr) {
#ifdef ARDUINO_RUNNING_CORE
    xTaskCreatePinnedToCore(serviceTask, "mrf24j40", MRF_TASK_STACK, this, MRF_TASK_PRIORITY, &task, ARDUINO_RUNNING_CORE);
#else
    xTaskCreate(serviceTask, "mrf24j40", MRF_TASK_STACK, this, MRF_TASK_PRIORITY, &task);
#endif
  }
  attachInterrupt(intPin, onInterrupt, FALLING);
  xTaskNotifyGive(task);                 // first pass: handle anything already pending
  return true;
}

// The radio drives INT (high when idle, low while an interrupt is pending): with the pin
// connected the level does not depend on the pull resistor, with the pin floating it does.
bool Nanoshield_MRF::intPinDriven() {
  pinMode(intPin, INPUT_PULLDOWN);
  delay(2);
  int pd = digitalRead(intPin);
  pinMode(intPin, INPUT_PULLUP);
  delay(2);
  int pu = digitalRead(intPin);
  return pd == pu;
}

void IRAM_ATTR Nanoshield_MRF::onInterrupt() {
  BaseType_t woken = pdFALSE;
  if (instance && instance->task) {
    vTaskNotifyGiveFromISR(instance->task, &woken);
  }
  if (woken) {
    portYIELD_FROM_ISR();
  }
}

void Nanoshield_MRF::serviceTask(void* arg) {
  Nanoshield_MRF* mrf = (Nanoshield_MRF*)arg;
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    // INT stays low until INTSTAT is read: if it is still low, an event arrived meanwhile
    do {
      mrf->service();
    } while (digitalRead(mrf->intPin) == LOW);
  }
}

void Nanoshield_MRF::service() {
  take();
  uint8_t flags = readShort(MRF_INTSTAT);    // the only place where INTSTAT is read
  if (flags & MRF_INTSTAT_TXNIF) {
    txStatus = readShort(MRF_TXSTAT);
    txBusy = false;
  }
  if (flags & MRF_INTSTAT_RXIF) {
    readRxFifo();
  }
  give();
}

// Copies the frame from the RX FIFO into the queue and parses the MAC header.
// Called with the lock held.
void Nanoshield_MRF::readRxFifo() {
  uint8_t frame[MRF_MAX_PSDU_SIZE];

  writeShort(MRF_BBREG1, 0b00000100);        // RXDECINV = 1: no reception while reading
  int len = readLong(MRF_RXFIFO);            // PSDU length, FCS included
  bool valid = len >= 5 && len <= MRF_MAX_PSDU_SIZE;
  uint8_t fifoLqi = 0, fifoRssi = 0;
  if (valid) {
    for (int i = 0; i < len; i++) {
      frame[i] = readLong(MRF_RXFIFO + 1 + i);
    }
    fifoLqi = readLong(MRF_RXFIFO + 1 + len);
    fifoRssi = readLong(MRF_RXFIFO + 1 + len + 1);
  }
  writeShort(MRF_RXFLUSH, 0b00000001);
  writeShort(MRF_BBREG1, 0);                 // RXDECINV = 0: receive again
  if (!valid) {
    return;
  }

  // MAC header: frame control, sequence number, addressing fields
  uint16_t fcf = frame[0] | frame[1] << 8;
  uint8_t frameType = fcf & 0b111;
  int dstMode = (fcf >> 10) & 0b11;
  int srcMode = (fcf >> 14) & 0b11;
  bool panCompression = fcf & 0b01000000;
  if (frameType != MRF_FRAME_DATA && !promiscuous) {
    return;
  }

  rx_info_t info;
  info.frame_type = frameType;
  info.seq = frame[2];
  info.panid = 0xFFFF;
  info.dest_addr = 0xFFFE;
  info.src_addr = 0xFFFE;
  int pos = 3;
  if (dstMode) {
    info.panid = frame[pos] | frame[pos + 1] << 8;
    pos += 2;
    if (dstMode == 2) info.dest_addr = frame[pos] | frame[pos + 1] << 8;
    pos += dstMode == 2 ? 2 : 8;
  }
  if (srcMode) {
    if (!(panCompression && dstMode)) {
      if (!dstMode) info.panid = frame[pos] | frame[pos + 1] << 8;
      pos += 2;
    }
    if (srcMode == 2) info.src_addr = frame[pos] | frame[pos + 1] << 8;
    pos += srcMode == 2 ? 2 : 8;
  }
  int payloadLen = len - MRF_MFR_SIZE - pos;
  if (payloadLen < 0) {
    return;
  }
  info.payload_length = payloadLen;
  memcpy(info.rx_data, &frame[pos], payloadLen);
  info.rx_data[payloadLen] = 0;              // readString() stays inside the buffer
  info.lqi = fifoLqi;
  info.rssi_raw = fifoRssi;
  info.rssi = (int)lroundf(fifoRssi * 0.197948f - 87.646977f);

  if (rxQueued == MRF_RX_QUEUE_SIZE) {       // full: drop the oldest frame
    rxTail = (rxTail + 1) % MRF_RX_QUEUE_SIZE;
    rxQueued--;
    rxDropped++;
  }
  rxQueue[rxHead] = info;
  rxHead = (rxHead + 1) % MRF_RX_QUEUE_SIZE;
  rxQueued++;
  if (rxCallback && callbackTask) {
    xTaskNotifyGive(callbackTask);
  }
}

void Nanoshield_MRF::onReceiveCallback(ZigbeeReceiveCallback callback) {
  take();
  rxCallback = callback;
  if (rxCallback && callbackTask == nullptr) {
#ifdef ARDUINO_RUNNING_CORE
    xTaskCreatePinnedToCore(callbackTaskLoop, "mrf24j40_rx", MRF_CB_TASK_STACK, this, MRF_CB_TASK_PRIORITY, &callbackTask, ARDUINO_RUNNING_CORE);
#else
    xTaskCreate(callbackTaskLoop, "mrf24j40_rx", MRF_CB_TASK_STACK, this, MRF_CB_TASK_PRIORITY, &callbackTask);
#endif
  }
  give();
  if (rxCallback && callbackTask) {
    xTaskNotifyGive(callbackTask);           // frames already in the queue
  }
}

// Takes the frames from the queue and hands them to the user callback. receivePacket() sets the
// current frame, so inside the callback read*() and readString() work as usual.
void Nanoshield_MRF::callbackTaskLoop(void* arg) {
  Nanoshield_MRF* mrf = (Nanoshield_MRF*)arg;
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    for (;;) {
      mrf->take();
      ZigbeeReceiveCallback cb = mrf->rxCallback;
      mrf->give();
      if (!cb || !mrf->receivePacket()) {
        break;
      }
      cb(&mrf->current);
    }
  }
}

void Nanoshield_MRF::take() {
  if (lock) xSemaphoreTakeRecursive(lock, portMAX_DELAY);
}

void Nanoshield_MRF::give() {
  if (lock) xSemaphoreGiveRecursive(lock);
}

void Nanoshield_MRF::setPanId(uint16_t panId) {
  take();
  this->panId = panId;
  writeShort(MRF_PANIDH, panId >> 8);
  writeShort(MRF_PANIDL, panId);
  give();
}

void Nanoshield_MRF::setAddress(uint16_t addr) {
  take();
  this->srcAddr = addr;
  writeShort(MRF_SADRH, addr >> 8);
  writeShort(MRF_SADRL, addr);
  give();
}

uint16_t Nanoshield_MRF::getPanId() {
  return panId;
}

uint16_t Nanoshield_MRF::getAddress() {
  return srcAddr;
}

void Nanoshield_MRF::setCoordinator(bool coord) {
  take();
  uint8_t reg = readShort(MRF_RXMCR);
  writeShort(MRF_RXMCR, coord ? reg | 0b00001100 : reg & ~0b00001100);
  give();
}

void Nanoshield_MRF::setChannel(int channel) {
  if (channel < 11 || channel > 26) {
    return;
  }

  take();
  this->channel = channel;
  // Change channel
  writeLong(MRF_RFCON0, (channel - 11) << 4 | 0x03);

  // Perform RF state machine reset and wait for RF circuitry to calibrate
  writeShort(MRF_RFCTL, 0b00000100);
  writeShort(MRF_RFCTL, 0);
  delayMicroseconds(200);
  give();
}

int Nanoshield_MRF::getChannel() {
  return channel;
}

float Nanoshield_MRF::measureSignalStrength() {
  take();
  // Disable PA
  if (type != MRF24J40MA) {
    writeLong(MRF_TESTMODE, 0b00001000);  // Configure RF state machine for normal operation
    writeShort(MRF_TRISGPIO, 0b00000110); // Configure GPIO1 and GPIO2 for output
    writeShort(MRF_GPIO, 0b00000100);     // Set GPIO1 0 to disable PA and GPIO2 to 1 to enable LNA
  }

  // Initiate RSSI calculation (RSSIMODE1 = 1)
  writeShort(MRF_BBREG6, 0b11000000);

  // Wait until conversion is ready and read RSSI
  while (!(readShort(MRF_BBREG6) & 0b00000001));
  rssi = readLong(MRF_RSSI);

  // Re-enable PA
  enablePaLna();
  give();

  return getSignalStrength();
}

float Nanoshield_MRF::getSignalStrength() {
  return rssi * 0.197948 - 87.646977;
}

int Nanoshield_MRF::getLinkQuality() {
  return lqi;
}

void Nanoshield_MRF::sleep() {
  take();
  // Disable PA and LNA
  switch (type) {
    case MRF24J40MB:
      // GPIO1 and GPIO2 are connected
      writeLong(MRF_TESTMODE, 0b00001000);  // Configure RF state machine for normal operation
      writeShort(MRF_TRISGPIO, 0b00000110); // Configure GPIO1 and GPIO2 for output
      writeShort(MRF_GPIO, 0);              // Set GPIO1 and GPIO2 to 0 to disable PA and LNA
      break;
    case MRF24J40MC:
      // GPIO1, GPIO2 and GPIO3 are connected - GPIO3 enables (high) or disables (low) the PA voltage regulator
      writeLong(MRF_TESTMODE, 0b00001000);  // Configure RF state machine for normal operation
      writeShort(MRF_TRISGPIO, 0b00001110); // Configure GPIO1, GPIO2 and GPIO3 for output
      writeShort(MRF_GPIO, 0);              // Set GPIO1, GPIO2 and GPIO3 to 0 to disable PA, LNA and PA regulator
      break;
    case MRF24J40MD:
    case MRF24J40ME:
      // GPIO0, GPIO1 and GPIO2 are connected
      writeLong(MRF_TESTMODE, 0b00001000);  // Configure RF state machine for normal operation
      writeShort(MRF_TRISGPIO, 0b00000111); // Configure GPIO0, GPIO1 and GPIO2 for output
      writeShort(MRF_GPIO, 0);              // Set GPIO1 and GPIO2 to 0 to disable PA and LNA
      break;
    default:
      break;
  }

  writeShort(MRF_SOFTRST, 0b00000100); // Perform Power Management Reset (RSTPWR = 1)
  writeShort(MRF_WAKECON, 0b10000000); // Enable Immediate Wake-up Mode (IMMWAKE = 1)
  writeShort(MRF_SLPACK, 0b10000000);  // Put the module to sleep (SLPACK = 1)
  give();
}

void Nanoshield_MRF::wakeup() {
  take();
  reset(); // Perform software reset
  init();  // Reinitialize all registers
  // the reset clears address, channel and receive mode: restore them
  setAddress(srcAddr);
  setChannel(channel);
  setPromiscuous(promiscuous);
  txBusy = false;
  give();
}

bool Nanoshield_MRF::write(uint8_t b) {
  return writeToBuffer(&b, 1);
}

uint8_t Nanoshield_MRF::read() {
  uint8_t result;
  return readFromBuffer(&result, 1) ? result : 0;
}

bool Nanoshield_MRF::writeInt(int16_t i) {
  return writeToBuffer(&i, 2);
}

int16_t Nanoshield_MRF::readInt() {
  int16_t result;
  return readFromBuffer(&result, 2) ? result : 0;
}

bool Nanoshield_MRF::writeUnsignedInt(uint16_t u) {
  return writeToBuffer(&u, 2);
}

uint16_t Nanoshield_MRF::readUnsignedInt() {
  uint16_t result;
  return readFromBuffer(&result, 2) ? result : 0;
}

bool Nanoshield_MRF::writeLong(int32_t l) {
  return writeToBuffer(&l, 4);
}

int32_t Nanoshield_MRF::readLong() {
  int32_t result;
  return readFromBuffer(&result, 4) ? result : 0;
}

bool Nanoshield_MRF::writeUnsignedLong(uint32_t u) {
  return writeToBuffer(&u, 4);
}

uint32_t Nanoshield_MRF::readUnsignedLong() {
  uint32_t result;
  return readFromBuffer(&result, 4) ? result : 0;
}

bool Nanoshield_MRF::writeFloat(float f) {
  return writeToBuffer(&f, sizeof(float));
}

float Nanoshield_MRF::readFloat() {
  float result;
  return readFromBuffer(&result, sizeof(float)) ? result : 0;
}

int Nanoshield_MRF::writeBytes(uint8_t* buf, int size) {
  if (size > bytesLeftToWrite()) {
    size = bytesLeftToWrite();
  }

  writeToBuffer(buf, size);
  return size;
}

int Nanoshield_MRF::readBytes(uint8_t* buf, int size) {
  if (size > bytesLeftToRead()) {
    size = bytesLeftToRead();
  }

  readFromBuffer(buf, size);
  return size;
}

int Nanoshield_MRF::writeString(char* str) {
  if (bytesLeftToWrite() == 0) {
    return 0;
  }

  int size = strlen(str);

  if (size > bytesLeftToWrite() - 1) {
    size = bytesLeftToWrite() - 1;
  }

  writeToBuffer(str, size);
  write('\0');
  return size + 1;
}

// Without a destination buffer it returns a pointer into the received frame (valid until the
// next receivePacket()) and moves past the string, so further read*() calls work.
const char* Nanoshield_MRF::readString(char* str, int size) {
  char* result = (char*)&rxBuf[rxCount];

  if (str == NULL) {
    rxCount += strnlen(result, bytesLeftToRead());
    if (rxCount < rxSize) {
      rxCount++;                              // skip the '\0'
    }
    return result;
  }

  if (size > bytesLeftToRead()) {
    size = bytesLeftToRead();
  }

  if (size == 0) {
    return 0;
  }

  str[size - 1] = '\0';
  strncpy(str, (char*)&rxBuf[rxCount], size - 1);
  rxCount += strlen(str) + 1;
  return result;
}

void Nanoshield_MRF::startPacket() {
  txCount = 0;
}

int Nanoshield_MRF::bytesLeftToWrite() {
  return MRF_MAX_PAYLOAD_SIZE - txCount;
}

bool Nanoshield_MRF::sendPacket(uint16_t addr, bool ack) {
  if (txCount == 0) {
    return false;
  }

  // The TX FIFO must not be rewritten while the radio is still sending it
  unsigned long start = millis();
  while (txBusy && millis() - start < MRF_TX_WAIT_MS) {
    delay(1);
  }

  take();
  // Send header size, frame size and header
  int i = 0;
  writeLong(i++, MRF_MHR_SIZE);                  // Header size
  writeLong(i++, MRF_MHR_SIZE + txCount);        // Frame size
  writeLong(i++, ack ? 0b01100001 : 0b01000001); // Frame control bits 0-7 (data frame, no security, no pending data, ack disabled, intra-PAN)
  writeLong(i++, 0b10001000);                    // Frame control bits 8-15 (16-bit addresses with PAN)
  writeLong(i++, seqNumber++);                   // Sequence number
  writeLong(i++, panId);                         // PAN ID
  writeLong(i++, panId >> 8);
  writeLong(i++, addr);                          // Destination address
  writeLong(i++, addr >> 8);
  writeLong(i++, srcAddr);                       // Source address
  writeLong(i++, srcAddr >> 8);

  // Send data payload
  for (int j = 0; j < txCount; j++) {
    writeLong(i++, txBuf[j]);
  }

  // Start transmission (0x37 0x01); the TXNIF interrupt tells when it ends
  txBusy = true;
  writeShort(MRF_TXNCON, ack ? 0b00000101 : 0b00000001);
  give();

  return true;
}

bool Nanoshield_MRF::transmissionDone() {
  return !txBusy;
}

bool Nanoshield_MRF::transmissionSuccess() {
  return !txBusy && !(txStatus & MRF_TXSTAT_TXNSTAT);
}

int Nanoshield_MRF::getRetries() {
  return (txStatus >> 6) & 0b11;
}

tx_info_t Nanoshield_MRF::getTXInfo() {
  tx_info_t info;
  info.tx_ok = transmissionSuccess();
  info.retries = getRetries();
  info.channel_busy = (txStatus & MRF_TXSTAT_CCAFAIL) != 0;
  return info;
}

bool Nanoshield_MRF::receivePacket() {
  take();
  if (rxQueued == 0) {
    give();
    return false;
  }
  current = rxQueue[rxTail];
  rxTail = (rxTail + 1) % MRF_RX_QUEUE_SIZE;
  rxQueued--;
  give();

  rxBuf = current.rx_data;
  rxSize = current.payload_length;
  rxCount = 0;
  lqi = current.lqi;
  rssi = current.rssi_raw;
  return true;
}

int Nanoshield_MRF::bytesLeftToRead() {
  return rxSize - rxCount;
}

int Nanoshield_MRF::packetsAvailable() {
  take();
  int n = rxQueued;
  give();
  return n;
}

uint32_t Nanoshield_MRF::droppedPackets() {
  take();
  uint32_t n = rxDropped;
  give();
  return n;
}

rx_info_t* Nanoshield_MRF::getRXPacket() {
  return &current;
}

uint16_t Nanoshield_MRF::getSourceAddress() {
  return current.src_addr;
}

uint16_t Nanoshield_MRF::getDestinationAddress() {
  return current.dest_addr;
}

void Nanoshield_MRF::reset() {
  writeShort(MRF_SOFTRST, 0b00000111); // Perform full software reset (RSTPWR = 1, RSTBB = 1, RSTMAC = 1)
}

void Nanoshield_MRF::init() {
  // MRF24J40 module configuration
  writeShort(MRF_PACON2, 0b10011000);    // Setup recommended PA/LNA control timing (TXONTS = 0x6)
  writeShort(MRF_TXSTBL, 0b10010101);    // Setup recommended PA/LNA control timing (RFSTBL = 0x9)
  writeLong(MRF_RFCON0, 0b00000011);     // Set recommended value for RF Optimize Control (RFOPT = 0x3)
  writeLong(MRF_RFCON1, 0b00000010);     // Set recommended value for VCO Optimize Control (VCOOPT = 0x2)
  writeLong(MRF_RFCON2, 0b10000000);     // Enable PLL (PLLEN = 1)
  writeLong(MRF_RFCON6, 0b10010000);     // Set recommended value for TX Filter Control and 20MHz Clock Recovery Control
                                         //  (TXFIL = 1, 20MRECVR = 1)
  writeLong(MRF_RFCON7, 0b10000000);     // Use 100kHz internal oscillator for Sleep Clock (SLPCLKSEL = 0x2)
  writeLong(MRF_RFCON8, 0b00010000);     // Set recommended value for VCO Control (RFVCO = 1)
  writeLong(MRF_SLPCON0, 0b00000001);    // Disable the sleep clock to save power (/SLPCLKEN = 1),
                                         //  INT active on the falling edge (INTEDGE = 0)
  writeLong(MRF_SLPCON1, 0b00100001);    // Disable CLKOUT pin and set Sleep Clock Divisor to minimum of 0x01 for the
                                         //  100kHz internal oscillator (/CLOUKTEN = 1, SLPCLKDIV = 0x01)
  writeShort(MRF_BBREG2, 0b10111000);    // Use CCA Mode 1 - Energy above threshold - and set CCA Carrier Sense
                                         //  Threshold to recommended value (CCAMODE = 0x2, CCACSTH = 0xE)
  writeShort(MRF_CCAEDTH, 0b01100000);   // Set energy detection threshold to recommended value (CCAEDTH = 0x60)
  writeShort(MRF_BBREG6, 0b01000000);    // Calculate RSSI for each packet received (RSSIMODE2 = 1)
  writeShort(MRF_INTCON, 0b11110110);    // Enable RX FIFO and TX normal FIFO interrupts (RXIE = 0, TXNIE = 0)
  enablePaLna();                         // Enable PA and LNA control

  setPanId(panId);
  setChannel(11);                        // Set default channel (must keep 11) and reset state machine
  delay(2);                              // Let RF stabilize
}

void Nanoshield_MRF::enablePaLna() {
  if (type != MRF24J40MA) {
    writeShort(MRF_TRISGPIO, 0);          // Configure all GPIO pins as input
    writeShort(MRF_GPIO, 0);
    writeLong(MRF_TESTMODE, 0b00001111);  // Configure RF state machine for automatic PA/LNA control
  }
}

uint8_t Nanoshield_MRF::readShort(uint8_t addr) {
  this->_spi->beginTransaction(spiSettings);
  digitalWrite(cs, LOW);
  this->_spi->transfer(addr << 1 & 0b01111110);
  uint8_t result = this->_spi->transfer(0);
  digitalWrite(cs, HIGH);
  this->_spi->endTransaction();
  return result;
}

void Nanoshield_MRF::setPromiscuous(boolean enabled) {
  take();
  promiscuous = enabled;
  writeShort(MRF_RXMCR, enabled ? 0x01 : 0x00);
  give();
}

void Nanoshield_MRF::writeShort(uint8_t addr, uint8_t data) {
  this->_spi->beginTransaction(spiSettings);
  digitalWrite(cs, LOW);
  this->_spi->transfer((addr << 1 & 0b01111110) | 0b00000001);
  this->_spi->transfer(data);
  digitalWrite(cs, HIGH);
  this->_spi->endTransaction();
}

uint8_t Nanoshield_MRF::readLong(uint16_t addr) {
  this->_spi->beginTransaction(spiSettings);
  digitalWrite(cs, LOW);
  this->_spi->transfer(addr >> 3 | 0b10000000);
  this->_spi->transfer(addr << 5);
  uint8_t result = this->_spi->transfer(0);
  digitalWrite(cs, HIGH);
  this->_spi->endTransaction();
  return result;
}

void Nanoshield_MRF::writeLong(uint16_t addr, uint8_t data) {
  this->_spi->beginTransaction(spiSettings);
  digitalWrite(cs, LOW);
  this->_spi->transfer(addr >> 3 | 0b10000000);
  this->_spi->transfer(addr << 5 | 0b00010000);
  this->_spi->transfer(data);
  digitalWrite(cs, HIGH);
  this->_spi->endTransaction();
}

bool Nanoshield_MRF::writeToBuffer(void* data, int size) {
  if (size > bytesLeftToWrite()) {
    return false;
  }

  memcpy(&txBuf[txCount], data, size);
  txCount += size;
  return true;
}

bool Nanoshield_MRF::readFromBuffer(void* data, int size) {
  if (size > bytesLeftToRead()) {
    return false;
  }

  memcpy(data, &rxBuf[rxCount], size);
  rxCount += size;
  return true;
}
