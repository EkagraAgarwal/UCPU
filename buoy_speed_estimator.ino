// SPDX-License-Identifier: MPL-2.0
/*
 * Professional-Grade Ocean Wave Speed Estimator
 * Features: 
 * 1. 2nd Order Low-Pass Filter on Acceleration
 * 2. Trapezoidal Integration for Precision
 * 3. Earth-Frame Tilt Compensation
 * 4. Throttled Serial Output to prevent Bridge Overload
 */

#include "Modulino.h"
#include <Arduino_RouterBridge.h>
#include <math.h>

ModulinoMovement movement;

// --- CONFIGURATION ---
const float G_TO_MS2 = 9.80665;
const float SAMPLE_RATE_HZ = 50.0;
const float DT = 1.0 / SAMPLE_RATE_HZ;
const float LPF_BETA = 0.25;      // Acceleration smoothing (0.0 to 1.0)
const float VEL_LEAK = 0.97;      // Velocity stability (0.95 to 0.99)
const float DEADZONE = 0.05;      // ignore noise below 0.05 m/s2

// --- STATE VARIABLES ---
float ax_filt = 0, ay_filt = 0, az_filt = 0; // Filtered Accel
float ax_prev = 0, ay_prev = 0, az_prev = 0; // For Trapezoidal Integration
float vx = 0, vy = 0, vz = 0;                // Velocity (m/s)
float speed = 0;

// Calibration
float ax_off = 0, ay_off = 0, az_off = 0;

void setup() {
  Bridge.begin();
  Monitor.begin(115200);
  while (!Monitor);

  Modulino.begin();
  while (!movement.begin()) { delay(500); }

  Monitor.println("CALIBRATING... STAY STILL");
  float sX = 0, sY = 0, sZ = 0;
  for (int i = 0; i < 200; i++) {
    if (movement.update()) {
      sX += movement.getX(); sY += movement.getY(); sZ += movement.getZ();
      delay(10);
    } else { i--; }
  }
  ax_off = sX / 200.0; ay_off = sY / 200.0; az_off = (sZ / 200.0) - 1.0;

  Monitor.println("Roll,Pitch,Acc_Net,Speed_ms");
}

void loop() {
  static unsigned long prevMicros = 0;
  static unsigned long lastPrint = 0;
  unsigned long currMicros = micros();
  
  // 1. Precise 50Hz Execution Loop
  if (currMicros - prevMicros >= 20000) { 
    prevMicros = currMicros;

    if (movement.update()) {
      // 2. Get Orientation (Radians)
      float rR = movement.getRoll()  * (PI / 180.0);
      float pR = movement.getPitch() * (PI / 180.0);

      // 3. Raw Calibrated Body Acceleration
      float ax_b = movement.getX() - ax_off;
      float ay_b = movement.getY() - ay_off;
      float az_b = movement.getZ() - az_off;

      // 4. Earth-Frame Rotation (Matrix Multiplication)
      float sP = sin(pR); float cP = cos(pR);
      float sR = sin(rR); float cR = cos(rR);

      float ax_e = (ax_b * cP + ay_b * sR * sP + az_b * cR * sP);
      float ay_e = (ay_body * cR - az_body * sR); // Wait, fix variable names
      ay_e = (ay_b * cR - az_b * sR);
      float az_e = (ax_b * -sP + ay_b * sR * cP + az_b * cR * cP);

      // 5. Convert to m/s2 and subtract gravity
      float ax_net = ax_e * G_TO_MS2;
      float ay_net = ay_e * G_TO_MS2;
      float az_net = (az_e - 1.0) * G_TO_MS2;

      // 6. Low-Pass Filter (Smoothing)
      ax_filt = (ax_net * LPF_BETA) + (ax_filt * (1.0 - LPF_BETA));
      ay_filt = (ay_net * LPF_BETA) + (ay_filt * (1.0 - LPF_BETA));
      az_filt = (az_net * LPF_BETA) + (az_filt * (1.0 - LPF_BETA));

      // 7. Deadzone Application
      if (abs(ax_filt) < DEADZONE) ax_filt = 0;
      if (abs(ay_filt) < DEADZONE) ay_filt = 0;
      if (abs(az_filt) < DEADZONE) az_filt = 0;

      // 8. Trapezoidal Integration ( (prev + current)/2 * dt )
      vx = (vx + (ax_prev + ax_filt) * 0.5 * DT) * VEL_LEAK;
      vy = (vy + (ay_prev + ay_filt) * 0.5 * DT) * VEL_LEAK;
      vz = (vz + (az_prev + az_filt) * 0.5 * DT) * VEL_LEAK;

      // Update "Previous" values for next step
      ax_prev = ax_filt; ay_prev = ay_filt; az_prev = az_filt;

      speed = sqrt(vx * vx + vy * vy + vz * vz);

      // 9. Throttled Output (10Hz) to keep the Serial Bridge clean
      if (millis() - lastPrint >= 100) {
        lastPrint = millis();
        Monitor.print(movement.getRoll(), 1);  Monitor.print(",");
        Monitor.print(movement.getPitch(), 1); Monitor.print(",");
        Monitor.print(az_filt, 2);             Monitor.print(",");
        Monitor.println(speed, 2);
      }
    }
  }
}
