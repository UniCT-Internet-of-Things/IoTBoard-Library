// #include "zigbee.h"


// bool Zigbee::init(SPIClass * spi){


//     interface = new Mrf24j(ZIGBEE_RST, ZIGBEE_CS, ZIGBEE_INT, ZIGBEE_WAKE, spi);
//     interface->reset();
//     interface->init();
//     interface->set_promiscuous(true);
//     interface->set_bufferPHY(true);

//     return true;
// }

// void Zigbee::setPromiscuous(bool enable){
//     interface->set_promiscuous(enable);
// }

// void Zigbee::setChannel(byte channel){
//     interface->set_channel(channel);
// }

// void Zigbee::setPANID(word panid){
//     interface->set_pan(panid);
// }

// void Zigbee::setAddress(word address16){
//     interface->address16_write(address16);
// }

// void Zigbee::send(word dest16, uint8_t* data, byte len){
//      interface->send16(dest16, data, len);
// }

// uint8_t * Zigbee::getRxBuffer(){
//     return interface->get_rxbuf();
// }

// rx_info_t * Zigbee::getRxInfo(){
//     return interface->get_rxinfo();
// }

// void Zigbee::interruptHandler(){
//     interface->interrupt_handler();
// }

// void Zigbee::send(word dest16, char* data){
//     interface->send16(dest16, data);
// }


// void Zigbee::onEvents(void (*rx_handler)(void), void (*tx_handler)(void)){
//     interface->check_flags(rx_handler, tx_handler);
// }

// void Zigbee::setTurbo(bool enable){
//     if(enable){
//         interface->enable_turbo();
//     } else {
//         interface->disable_turbo();
//     }
// }