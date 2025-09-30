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

