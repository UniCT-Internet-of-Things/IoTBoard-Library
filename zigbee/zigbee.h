#pragma once
#ifdef ZIGBEE_ENABLED

//#include "mrf24j.h"
#include "nanoshield.h"
#include "pins.h"
#endif



class Zigbee: public Nanoshield_MRF{

    public:
        Zigbee(SPIClass * spi) : Nanoshield_MRF(MRF24J40MA, ZIGBEE_CS, spi){};

};

// class Zigbee{

//     public:

//         bool init(SPIClass * spi);
//         void setChannel(byte channel);
//         void setPANID(word panid);
//         void setAddress(word address16);
//         void setPromiscuous(bool enable=false);

//         void send(word dest16, uint8_t* data, byte len);
//         void send(word dest16, char* data);

//         uint8_t * getRxBuffer();
//         rx_info_t * getRxInfo();


//         void onEvents(void (*rx_handler)(void), void (*tx_handler)(void));
//         void interruptHandler();

//         void setTurbo(bool enable);
        
    
//     private:
//         Mrf24j *interface;
// };