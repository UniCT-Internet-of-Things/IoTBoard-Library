#include "buttons.h"



void Buttons::init_buttons(){
    button_1.setup(BTN_1, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);
    button_2.setup(BTN_1, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);
}


void Buttons::update(){
    unsigned long now = millis();
    button_1.process(now);
    button_2.process(now);    
}


void Buttons::onBtn1Release(inputdebounce_state_cb releasedCallback){
    button_1.registerCallbacks(NULL, releasedCallback, NULL, NULL);
}

void Buttons::onBtn2Release(inputdebounce_state_cb releasedCallback){
    button_2.registerCallbacks(NULL, releasedCallback, NULL, NULL);
}