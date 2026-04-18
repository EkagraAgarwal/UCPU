#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "Modulino.h"

class EnvironmentSensors {
public:
    void begin(int pin);
    void update();
    float getTemperature() const { return temperature; }
    float getDepth() const { return depthMM; }

private:
    ModulinoThermo thermo;
    int waterPin;
    float temperature = 0;
    float depthMM = 0;
};

#endif
