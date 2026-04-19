#include "Movement.h"
#include <Arduino.h>

void MovementTracker::begin() {
    while (!movement.begin()) { delay(500); }
    calibrate(); // Establish zero-point and initial orientation
    prevMicros = micros(); 
}

void MovementTracker::calibrate() {
    float sX = 0, sY = 0, sZ = 0;
    int samples = 200;
    for (int i = 0; i < samples; i++) {
        if (movement.update()) {
            sX += movement.getX(); sY += movement.getY(); sZ += movement.getZ();
            delay(10);
        } else { i--; }
    }
    ax_off = sX / (float)samples; 
    ay_off = sY / (float)samples; 
    az_off = (sZ / (float)samples) - 1.0;
    
    // Reset state
    vx = vy = vz = 0;
    speed = 0;
    prevMicros = micros();
}

void MovementTracker::update() {
    unsigned long currMicros = micros();
    if (currMicros - prevMicros >= 20000) {
        float dt = (currMicros - prevMicros) / 1000000.0;
        prevMicros = currMicros;

        if (movement.update()) {
            roll = movement.getRoll();
            pitch = movement.getPitch();
            
            // Standard Tait-Bryan rotation for Modulino orientation
            float rR = roll * (PI / 180.0);
            float pR = pitch * (PI / 180.0);

            // Raw Body Acceleration (minus offsets)
            float ax_b = movement.getX() - ax_off;
            float ay_b = movement.getY() - ay_off;
            float az_b = movement.getZ() - az_off;

            float sP = sin(pR); float cP = cos(pR);
            float sR = sin(rR); float cR = cos(rR);

            // Earth-Frame Rotation (Simplified for Roll/Pitch only)
            // Z-up convention
            float ax_e = ax_b * cP + az_b * sP;
            float ay_e = ax_b * sR * sP + ay_b * cR - az_b * sR * cP;
            float az_e = -ax_b * cR * sP + ay_b * sR + az_b * cR * cP;

            // Net Linear Acceleration (m/s^2)
            float ax_net = ax_e * G_TO_MS2;
            float ay_net = ay_e * G_TO_MS2;
            float az_net = (az_e - 1.0) * G_TO_MS2;

            // Low Pass Filter
            ax_filt = (ax_net * LPF_BETA) + (ax_filt * (1.0 - LPF_BETA));
            ay_filt = (ay_net * LPF_BETA) + (ay_filt * (1.0 - LPF_BETA));
            az_filt = (az_net * LPF_BETA) + (az_filt * (1.0 - LPF_BETA));

            // Noise Deadzone
            if (abs(ax_filt) < DEADZONE) ax_filt = 0;
            if (abs(ay_filt) < DEADZONE) ay_filt = 0;
            if (abs(az_filt) < DEADZONE) az_filt = 0;

            // Velocity Integration (Trapezoidal)
            vx = (vx + (ax_prev + ax_filt) * 0.5 * dt) * VEL_LEAK;
            vy = (vy + (ay_prev + ay_filt) * 0.5 * dt) * VEL_LEAK;
            vz = (vz + (az_prev + az_filt) * 0.5 * dt) * VEL_LEAK;

            ax_prev = ax_filt;
            ay_prev = ay_filt;
            az_prev = az_filt;

            // STATIONARY CLAMP: If everything is quiet, pull speed to zero faster
            if (ax_filt == 0 && ay_filt == 0 && az_filt == 0) {
                vx *= 0.9; vy *= 0.9; vz *= 0.9;
            }

            speed = sqrt(vx * vx + vy * vy + vz * vz);
            
            // Hard clamp to prevent runaway
            if (speed < 0.01) speed = 0;
        }
    }
}
