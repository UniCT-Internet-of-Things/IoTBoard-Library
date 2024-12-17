#include "display.h"


bool Display::init_interface(){
    Wire.begin(OLED_SDA, OLED_SCL);
    if(!begin(SSD1306_SWITCHCAPVCC, 0x3C)){
        return false;
    }

    clearDisplay();
    setTextSize(1); 
    setTextColor(WHITE);
    display();
    return true;
};