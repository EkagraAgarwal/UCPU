#include "Environment.h"
#include <Arduino.h>

void EnvironmentSensors::begin(int pin) {
    waterPin = pin;
    thermo.begin();
}

void EnvironmentSensors::update() {
    temperature = thermo.getTemperature();
    int rawLevel = analogRead(waterPin);
    depthMM = map(rawLevel, 0, 600, 0, 40); 
}
