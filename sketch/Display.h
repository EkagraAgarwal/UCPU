#ifndef DISPLAY_H
#define DISPLAY_H

#include "Arduino_LED_Matrix.h"

class Visualizer {
public:
    void begin();
    void drawDepth(float depthMM);
    void showBootMessage();

private:
    ArduinoLEDMatrix matrix;
};

#endif
