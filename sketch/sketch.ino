// SPDX-License-Identifier: MPL-2.0
/*
 * BluePulse Node - Integrated Buoy Systems
 * Orchestrates Movement Tracking, Environmental Sensing, and Visual Feedback.
 */

#include "Modulino.h"
#include <Arduino_RouterBridge.h>
#include <vector>
#include <zephyr/kernel.h>
#include "Movement.h"
#include "Environment.h"
#include "Display.h"

MovementTracker tracker;
EnvironmentSensors env;
Visualizer gui;

K_MUTEX_DEFINE(data_mtx);
const int WATER_LEVEL_PIN = A0;

std::vector<float> get_telemetry() {
  std::vector<float> data;
  k_mutex_lock(&data_mtx, K_FOREVER);
  data.push_back(env.getTemperature());
  data.push_back(env.getDepth());
  data.push_back(tracker.getSpeed());
  data.push_back(tracker.getRoll());
  data.push_back(tracker.getPitch());
  k_mutex_unlock(&data_mtx);
  return data;
}

void setup() {
  Bridge.begin();
  Bridge.provide("get_telemetry", get_telemetry);
  Monitor.begin(115200);
  // while (!Monitor); // Removed to prevent blocking on headless start

  Modulino.begin();
  
  Monitor.println("Initializing Modules...");
  gui.begin();
  env.begin(WATER_LEVEL_PIN);
  tracker.begin();

  Monitor.println("STATIONARY CALIBRATION - DO NOT MOVE BUOY");
  tracker.calibrate();

  Monitor.println("BLUEPULSE HEARTBEAT: Calibration Complete, entering main loop.");
}

void loop() {
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat >= 5000) {
    lastHeartbeat = millis();
    Monitor.println("BLUEPULSE HEARTBEAT: Still running...");
  }

  k_mutex_lock(&data_mtx, K_FOREVER);
  // 1. Update all modules
  tracker.update();
  env.update();
  k_mutex_unlock(&data_mtx);
  
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
