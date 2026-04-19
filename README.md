# TidalGate: Predictive Hydrokinetic Telemetry Node

**Optimizing Tidal Energy Yield through Feed-Forward Edge Intelligence.**

TidalGate is a telemetry buoy system designed to address the "Impedance Matching" problem in tidal energy. By profiling the energy of incoming swells upstream, TidalGate enables downstream turbines to  tune their resistance, aiming to increase energy harvest efficiency and reduce structural fatigue.


## The Problem & The Solution
Traditional tidal turbines are **reactive**; they adjust to wave impact only after it occurs, which can lead to energy loss and mechanical wear. 
**TidalGate** shifts the paradigm to **Predictive Control**:
* **Early Warning:** Deployed upstream to profile energy before it reaches the turbine array.
* **Density Correction:** Uses thermal data to adjust energy calculations for water density variations.
* **Edge Processing:** Performs real-time orbital velocity fusion on-device to minimize data latency.

---

## Tech Stack

### **Hardware: The Heterogeneous Architecture**
* **Core Controller:** Arduino Uno Q.
* **Motion Engine:** Modulino Movement (LSM6DSOX 6-axis IMU).
* **Environmental Engine:** Modulino Thermo (STTS22H) + Analog Submersion Probe for depth.


## Project Structure

* `UCPU.ino` - Main orchestration logic and sensor polling.
* `Movement.h/cpp` - 3D orbital velocity calculation and tilt compensation.
* `Environment.h/cpp` - Temperature and water depth (submersion) sensing modules.
* `Display.h/cpp` - LED Matrix driver for localized status and level visualization.
* `main.py` - Linux-side bridge for data logging and hosting the telemetry portal.

---

## ⚙️ Setup & Deployment

### 1. Hardware Interconnect
1. Connect the Modulino modules via the **Qwiic** bus.
2. Connect the Analog Water Level Sensor to **Pin A0**.
3. Power the Uno Q via USB-C.

### 2. Firmware Installation
1. Open the project in the Arduino AppLap.
2. Ensure necessary libraries are installed: `Arduino_Modulino`, `Arduino_LED_Matrix`, and `Arduino_RouterBridge`.
3. Upload code to the board.
4. **Calibration:** Keep the buoy flat and still for the first 3 seconds after boot to allow for IMU bias calibration.

### 3. Linux/Web Dashboard Setup
1. Access the Linux shell via ADB: `adb shell`.
2. Navigate to the directory containing your web files: `cd /home/arduino/`.
3. Start the local web server:
   ```bash
   sudo python3 -m http.server 80
   '''

---
*Developed for the UCSD DataHacks 2026. SPDX-License-Identifier: MPL-2.0*
