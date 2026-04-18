#ifndef MOVEMENT_H
#define MOVEMENT_H

#include "Modulino.h"

class MovementTracker {
public:
    void begin();
    void calibrate();
    void update();
    float getSpeed() const { return speed; }
    float getRoll() const { return roll; }
    float getPitch() const { return pitch; }

private:
    ModulinoMovement movement;
    float vx = 0, vy = 0, vz = 0;
    float speed = 0;
    float roll = 0, pitch = 0;
    float ax_off = 0, ay_off = 0, az_off = 0;
    float ax_prev = 0, ay_prev = 0, az_prev = 0;
    unsigned long prevMicros = 0;

    const float G_TO_MS2 = 9.80665;
    const float VEL_LEAK = 0.97;
    const float LPF_BETA = 0.25;
    const float DEADZONE = 0.05;
    float ax_filt = 0, ay_filt = 0, az_filt = 0;
};

#endif
