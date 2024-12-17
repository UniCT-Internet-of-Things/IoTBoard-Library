#pragma once

#include "debounce.h"
#include "pins.h"

#define BUTTON_DEBOUNCE_DELAY   20   // [ms]


class Buttons{
    public:
        //Buttons();
        void init_buttons();
        void update();
        void onBtn1Release(inputdebounce_state_cb releasedCallback);
        void onBtn2Release(inputdebounce_state_cb releasedCallback);
    private:
        InputDebounce button_1; 
        InputDebounce button_2; 
};