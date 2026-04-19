#include "Movement.h"
#include <Arduino.h>

void MovementTracker::begin() {
    while (!movement.begin()) { delay(500); }
}

void MovementTracker::calibrate() {
    float sX = 0, sY = 0, sZ = 0;
    for (int i = 0; i < 200; i++) {
        if (movement.update()) {
            sX += movement.getX(); sY += movement.getY(); sZ += movement.getZ();
            delay(10);
        } else { i--; }
    }
    ax_off = sX / 200.0; 
    ay_off = sY / 200.0; 
    az_off = (sZ / 200.0) - 1.0;
}

void MovementTracker::update() {
    unsigned long currMicros = micros();
    if (currMicros - prevMicros >= 20000) {
        float dt = (currMicros - prevMicros) / 1000000.0;
        prevMicros = currMicros;

        if (movement.update()) {
            roll = movement.getRoll();
            pitch = movement.getPitch();
            float rR = roll * (PI / 180.0);
            float pR = pitch * (PI / 180.0);

            float ax_b = movement.getX() - ax_off;
            float ay_b = movement.getY() - ay_off;
            float az_b = movement.getZ() - az_off;

            float sP = sin(pR); float cP = cos(pR);
            float sR = sin(rR); float cR = cos(rR);

            float ax_e = (ax_b * cP + ay_b * sR * sP + az_b * cR * sP);
            float ay_e = (ay_b * cR - az_b * sR);
            float az_e = (ax_b * -sP + ay_b * sR * cP + az_b * cR * cP);

            float ax_net = ax_e * G_TO_MS2;
            float ay_net = ay_e * G_TO_MS2;
            float az_net = (az_e - 1.0) * G_TO_MS2;

            ax_filt = (ax_net * LPF_BETA) + (ax_filt * (1.0 - LPF_BETA));
            ay_filt = (ay_net * LPF_BETA) + (ay_filt * (1.0 - LPF_BETA));
            az_filt = (az_net * LPF_BETA) + (az_filt * (1.0 - LPF_BETA));

            if (abs(ax_filt) < DEADZONE) ax_filt = 0;
            if (abs(ay_filt) < DEADZONE) ay_filt = 0;
            if (abs(az_filt) < DEADZONE) az_filt = 0;

            vx = (vx + (ax_prev + ax_filt) * 0.5 * dt) * VEL_LEAK;
            vy = (vy + (ay_prev + ay_filt) * 0.5 * dt) * VEL_LEAK;
            vz = (vz + (az_prev + az_filt) * 0.5 * dt) * VEL_LEAK;

            ax_prev = ax_filt; ay_prev = ay_filt; az_prev = az_filt;
            speed = sqrt(vx * vx + vy * vy + vz * vz);
        }
    }
}
