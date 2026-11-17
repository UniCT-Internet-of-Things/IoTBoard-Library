#include "buttons.h"

void Buttons::init_buttons(){
    const uint8_t pins[BUTTONS_COUNT] = {BTN_1, BTN_2};
    for(int i = 0; i < BUTTONS_COUNT; i++){
        buttons[i].pin = pins[i];
        buttons[i].onPress = nullptr;
        buttons[i].onRelease = nullptr;
        pinMode(pins[i], INPUT_PULLUP);
    }
    delay(BUTTON_DEBOUNCE_DELAY);
    for(int i = 0; i < BUTTONS_COUNT; i++){
        buttons[i].pressed = digitalRead(buttons[i].pin) == LOW;
    }
    if(task == nullptr){
#ifdef ARDUINO_RUNNING_CORE
        xTaskCreatePinnedToCore(taskLoop, "buttons", 4096, this, 2, &task, ARDUINO_RUNNING_CORE);
#else
        xTaskCreate(taskLoop, "buttons", 4096, this, 2, &task);
#endif
    }
    for(int i = 0; i < BUTTONS_COUNT; i++){
        attachInterruptArg(buttons[i].pin, onEdge, this, CHANGE);
    }
}

void IRAM_ATTR Buttons::onEdge(void *arg){
    Buttons *self = (Buttons *)arg;
    BaseType_t woken = pdFALSE;
    if(self->task){
        vTaskNotifyGiveFromISR(self->task, &woken);
    }
    if(woken){
        portYIELD_FROM_ISR();
    }
}

void Buttons::taskLoop(void *arg){
    Buttons *self = (Buttons *)arg;
    for(;;){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        while(ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(BUTTON_DEBOUNCE_DELAY)) > 0){
        }
        self->settle();
    }
}

void Buttons::settle(){
    for(int i = 0; i < BUTTONS_COUNT; i++){
        Button &b = buttons[i];
        bool pressed = digitalRead(b.pin) == LOW;
        if(pressed == b.pressed){
            continue;
        }
        b.pressed = pressed;
        inputdebounce_state_cb cb = pressed ? b.onPress : b.onRelease;
        if(cb){
            cb(b.pin);
        }
    }
}

void Buttons::update(){
}

void Buttons::onBtn1Press(inputdebounce_state_cb pressedCallback){
    buttons[0].onPress = pressedCallback;
}

void Buttons::onBtn2Press(inputdebounce_state_cb pressedCallback){
    buttons[1].onPress = pressedCallback;
}

void Buttons::onBtn1Release(inputdebounce_state_cb releasedCallback){
    buttons[0].onRelease = releasedCallback;
}

void Buttons::onBtn2Release(inputdebounce_state_cb releasedCallback){
    buttons[1].onRelease = releasedCallback;
}

bool Buttons::isBtn1Pressed(){
    return buttons[0].pressed;
}

bool Buttons::isBtn2Pressed(){
    return buttons[1].pressed;
}
