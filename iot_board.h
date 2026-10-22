#pragma once

#include <Arduino.h>
#include <SPI.h>

#include "pins.h"
#include "./buttons/buttons.h"
#include "./display/display.h"
#include "./zigbee/zigbee.h"
#include "./lora/lora_dev.h"
#include "./lorawan/lorawan.h"
#include "./espmesh/espmesh.h"

#define IOTBOARD_TIMERS 8

extern Buttons *buttons;
extern SPIClass *radios_spi;
extern Display *display;
extern Zigbee *zigbee;
extern LoRaClass *lora;
extern LoRaWANClass *lorawan;
#ifdef ESPMESH_ENABLED
extern EspMesh *espmesh;
#endif

class IoTBoardClass{

    public:
        IoTBoardClass(){};
        void init_serial(uint baudrate=115200);
        void init_leds();
        void init_buttons();
        bool init_display();

        void init_spi(uint8_t spiInterface=HSPI);
        bool init_zigbee();
        bool init_lora();
        bool init_lorawan();
#ifdef ESPMESH_ENABLED
        bool init_espmesh(const EspMeshConfig &config);
        bool init_espmesh(const char *router_ssid, const char *router_pass, espmesh_event_cb callback=nullptr);
#endif

        bool run_every(unsigned long interval, uint8_t timer=0);
        
        static IoTBoardClass* get_instance(){
            if(_instance == nullptr){
                _instance = new IoTBoardClass();
            }
            return _instance;
        };
        
    protected:
    private:
        static IoTBoardClass* _instance;
        bool isSPIActivated = false;
        bool isZigbeeActivated = false;
        bool isLoRaActivated = false;
        bool isLoRaWANActivated = false;
        bool isEspMeshActivated = false;
        unsigned long timers[IOTBOARD_TIMERS] = {0};
};

namespace IoTBoard{

    inline void init_serial(uint baudrate=115200){ IoTBoardClass::get_instance()->init_serial(baudrate);};
    inline void init_leds(){ IoTBoardClass::get_instance()->init_leds();};
    inline void init_buttons(){ IoTBoardClass::get_instance()->init_buttons();};
    inline bool init_display(){ return IoTBoardClass::get_instance()->init_display();};

    inline void init_spi(uint8_t spiInterface=HSPI){ IoTBoardClass::get_instance()->init_spi(spiInterface);};
    inline bool init_zigbee(){ return IoTBoardClass::get_instance()->init_zigbee();};
    inline bool init_lora(){ return IoTBoardClass::get_instance()->init_lora();};
    inline bool init_lorawan(){ return IoTBoardClass::get_instance()->init_lorawan();};
#ifdef ESPMESH_ENABLED
    inline bool init_espmesh(const EspMeshConfig &config){ return IoTBoardClass::get_instance()->init_espmesh(config);};
    inline bool init_espmesh(const char *router_ssid, const char *router_pass, espmesh_event_cb callback=nullptr){ return IoTBoardClass::get_instance()->init_espmesh(router_ssid, router_pass, callback);};
#endif

    inline bool runEvery(unsigned long interval, uint8_t timer=0){ return IoTBoardClass::get_instance()->run_every(interval, timer);};
}


