#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "pins.h"

#define BUTTON_DEBOUNCE_DELAY   30
#define BUTTONS_COUNT           2

typedef void (*inputdebounce_state_cb)(uint8_t);

class Buttons{
    public:
    
        void init_buttons();
        void update();
        void onBtn1Press(inputdebounce_state_cb pressedCallback);
        void onBtn2Press(inputdebounce_state_cb pressedCallback);
        void onBtn1Release(inputdebounce_state_cb releasedCallback);
        void onBtn2Release(inputdebounce_state_cb releasedCallback);
        bool isBtn1Pressed();
        bool isBtn2Pressed();

    private:
        struct Button {
            uint8_t pin;
            volatile bool pressed;
            inputdebounce_state_cb onPress;
            inputdebounce_state_cb onRelease;
        };
        Button buttons[BUTTONS_COUNT];
        TaskHandle_t task = nullptr;
        static void IRAM_ATTR onEdge(void *arg);
        static void taskLoop(void *arg);
        void settle();
};
