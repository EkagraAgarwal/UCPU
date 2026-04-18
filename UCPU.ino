// SPDX-License-Identifier: MPL-2.0
/*
 * BluePulse Node - Integrated Buoy Systems
 * Orchestrates Movement Tracking, Environmental Sensing, and Visual Feedback.
 */

#include "Modulino.h"
#include <Arduino_RouterBridge.h>
#include "Movement.h"
#include "Environment.h"
#include "Display.h"

MovementTracker tracker;
EnvironmentSensors env;
Visualizer gui;

const int WATER_LEVEL_PIN = A0;

void setup() {
  Bridge.begin();
  Monitor.begin(115200);
  while (!Monitor);

  Modulino.begin();
  
  Monitor.println("Initializing Modules...");
  gui.begin();
  env.begin(WATER_LEVEL_PIN);
  tracker.begin();

  Monitor.println("STATIONARY CALIBRATION - DO NOT MOVE BUOY");
  tracker.calibrate();

  Monitor.println("System Active: BluePulse Node Online");
}

void loop() {
  // 1. Update all modules
  tracker.update();
  env.update();
  
  // 2. Update visualizer
  gui.drawDepth(env.getDepth());

  // 3. Log data (Throttled for stability)
  static unsigned long lastLog = 0;
  if (millis() - lastLog >= 100) {
    lastLog = millis();
    Monitor.print("Temp:");  Monitor.print(env.getTemperature(), 1);
    Monitor.print(",Depth:"); Monitor.print(env.getDepth(), 0);
    Monitor.print(",Speed:"); Monitor.print(tracker.getSpeed(), 2);
    Monitor.print(",Roll:");  Monitor.print(tracker.getRoll(), 1);
    Monitor.print(",Pitch:"); Monitor.println(tracker.getPitch(), 1);
  }
}
