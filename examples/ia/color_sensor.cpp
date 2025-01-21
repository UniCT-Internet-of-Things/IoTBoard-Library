#include <iot_board.h>
#include <Adafruit_TCS34725.h>
#include <EloquentTinyML.h>
#include "model.h"


#define TENSOR_ARENA_SIZE 2 * 1024
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

Eloquent::TinyML::TfLite<
    TF_NUM_INPUTS,
    TF_NUM_OUTPUTS,
    TENSOR_ARENA_SIZE>
    ml;

void setup(){
    IoTBoard::init_serial();
    delay(3000);

    if (!ml.begin(model_data)){
        Serial.println("Errore");
        while (true)
            delay(10);
    }
    if (tcs.begin()){
        // Serial.println("Found sensor");
    }
    else{
        Serial.println("No TCS34725 found ... check your connections");
        while (1)
            ; // halt!
    }

    Serial.println("capybara");
}

void loop(){
    
    float red, green, blue;

    tcs.setInterrupt(false); // turn on LED

    delay(60); // takes 50ms to read

    tcs.getRGB(&red, &green, &blue);

    tcs.setInterrupt(true); // turn off LED
    float input[3] = {red, green, blue};
    int color = ml.predictClass(input);
    switch (color){
        case 0:
            Serial.println("Blue");
            break;

        case 1:
            Serial.println("Green");
            break;
        case 2:
            Serial.println("Red");
            break;
        case 3:
            Serial.println("Yellow");
            break;
    }

    delay(1000);
}