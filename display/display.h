
#pragma once
#ifdef DISPLAY_ENABLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "pins.h"

#endif


class Display : public Adafruit_SSD1306{

    public:
        Display() : Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET){};
        bool init_interface();
        
};

