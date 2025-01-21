#include <iot_board.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
//libreria per il server device
#include <BLEServer.h>
//libreria per il central device
#include <BLEAdvertisedDevice.h>
#include <BLEScan.h>
#include "ble_defines.h"

//#define BLE_PERIFERICA 1

//periferica
BLEServer *serverBLE;
BLECharacteristic *caratteristicaLedVerde;
bool centralConnected = false;

//classe callback della caratteristica
class CallbackLedVerde : public BLECharacteristicCallbacks{
    void onWrite(BLECharacteristic *caratteristicaScritta){
        //qualcosa
        Serial.println(caratteristicaScritta->getValue());
        display->clearDisplay();
        display->setCursor(0,0);
        display->println("Valore Scritto");
        display->println(caratteristicaScritta->getValue());
        display->display();
        if(caratteristicaScritta->getValue() == "on"){
            digitalWrite(LED_GREEN, HIGH);
        } else {
            digitalWrite(LED_GREEN, LOW);
        }
        
        caratteristicaScritta->notify();
    }
};

class CallbacksServer : public BLEServerCallbacks{

    void onConnect(BLEServer *pServer){
        centralConnected=true;
        display->clearDisplay();
        display->setCursor(0,0);
        display->println("Dispositivo Connesso");
        display->display();
    }

    void onDisconnect(BLEServer *pServer){
        centralConnected = false;
        display->clearDisplay();
        display->setCursor(0,0);
        display->println("In attesa di connessione BLE");
        display->display();
    }

};

//fine periferica

//dispositivo centrale

bool connected = false;
bool deviceFound = false;
static BLERemoteCharacteristic* characteristic;
static BLEAddress *pServerAddress;
BLEScan *bleScan;

bool statusLedVerde = false;

void onBtn1Pressed();
void onBtn1Release();
void connectToLampada();
void toggleLed();

class CallbacksAdvertising : public BLEAdvertisedDeviceCallbacks{

    void onResult(BLEAdvertisedDevice advertisedDevice){
        Serial.println(advertisedDevice.toString().c_str());
        if(advertisedDevice.haveName() && advertisedDevice.getName() == "lampada_philips"){
            display->println("Dispositivo Trovato");
            display->display();
            deviceFound = true;
            bleScan->stop();
            pServerAddress = new BLEAddress(advertisedDevice.getAddress());
        }
    }
};


//fine dispositivo centrale

bool toToggle = false;

void setup() {
    IoTBoard::init_serial();
    IoTBoard::init_leds();
    IoTBoard::init_display();
    display->println("Display enabled");
    display->display();
    
    //pulsante BTN_1
    pinMode(BTN_1, INPUT_PULLUP);
    attachInterrupt(BTN_1, onBtn1Pressed, FALLING);

    //inizializzo il dispositivo periferica
    #ifdef BLE_PERIFERICA
    BLEDevice::init("lampada_philips");
    serverBLE = BLEDevice::createServer();
    serverBLE->setCallbacks(new CallbacksServer());
    BLEService *servizioLuce = serverBLE->createService(UUID_SERVICE_LIGHT);
    caratteristicaLedVerde = servizioLuce->createCharacteristic(UUID_CHARATERISTIC_LED_GREEN, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    caratteristicaLedVerde->setValue("off");
    caratteristicaLedVerde->setCallbacks(new CallbackLedVerde());

    servizioLuce->start();

    BLEAdvertising *servizioAdvertising = BLEDevice::getAdvertising();
    servizioAdvertising->addServiceUUID(UUID_SERVICE_LIGHT);
    servizioAdvertising->setScanResponse(true);
    display->clearDisplay();
    display->setCursor(0,0);
    display->println("In attesa di connessione BLE");
    display->display();
    BLEDevice::startAdvertising();
    //fine periferica
    #endif

    #ifndef BLE_PERIFERICA
    //configuro il dispositivo centrale
    BLEDevice::init("");
    display->println("Ricerca del dispositivo");
    display->display();
    bleScan = BLEDevice::getScan();
    bleScan->setAdvertisedDeviceCallbacks(new CallbacksAdvertising());
    bleScan->setActiveScan(true);
    bleScan->setInterval(100);
    bleScan->setWindow(90);
    bleScan->start(5000, true);

    //fine dispositivo centrale
    #endif

}

void loop(){

    #ifndef BLE_PERIFERICA
    if(deviceFound && !connected){
        //devo collegarmi
        connectToLampada();
    } else {
        if(toToggle){
            toToggle = false;
            toggleLed();
        }
        //procedo con la logica del pulsante
    }
    #endif

    #ifdef BLE_PERIFERICA
    if(centralConnected && toToggle){
        toToggle = false;
        toggleLed();
    }
    #endif
}

void callbackNotifica(BLERemoteCharacteristic *carNotificata, uint8_t* pData, size_t length, bool isNotify){
    String s = String(pData, length);

    if(s == "on"){
        digitalWrite(LED_GREEN, HIGH);
    } else {
        digitalWrite(LED_GREEN, LOW);
    }

}

void connectToLampada(){

    display->clearDisplay();
    display->setCursor(0,0);
    display->println("Collegamento in corso");
    display->display();
    
    BLEClient *bleClient = BLEDevice::createClient();
    bleClient->connect(*pServerAddress);
    BLERemoteService *servizioRemoto = bleClient->getService(UUID_SERVICE_LIGHT);
    characteristic = servizioRemoto->getCharacteristic(UUID_CHARATERISTIC_LED_GREEN);
    characteristic->registerForNotify(callbackNotifica, true, false);
    connected = true;
    display->println("Dispositivo collegato");
    display->display();
}

void onBtn1Pressed(){
    detachInterrupt(BTN_1);
    attachInterrupt(BTN_1, onBtn1Release, RISING);
}

void onBtn1Release(){
    detachInterrupt(BTN_1);
    attachInterrupt(BTN_1, onBtn1Pressed, FALLING);
    toToggle = true;
    //digitalWrite(LED_RED, !digitalRead(LED_RED));
}

void toggleLed(){

    #ifndef BLE_PERIFERICA
    statusLedVerde = !statusLedVerde;
    if(statusLedVerde){
        characteristic->writeValue("on");
    } else {
        characteristic->writeValue("off");
    }
    #endif

    #ifdef BLE_PERIFERICA
    if(caratteristicaLedVerde->getValue() == "on"){
        caratteristicaLedVerde->setValue("off");
        digitalWrite(LED_GREEN, LOW);
    } else {
        caratteristicaLedVerde->setValue("on");
        digitalWrite(LED_GREEN, HIGH);
    }
    caratteristicaLedVerde->notify();
    #endif
}
