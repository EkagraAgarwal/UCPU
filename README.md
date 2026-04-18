# BluePulse Node - Integrated Ocean Buoy System

Professional-grade firmware for the Arduino Modulino platform, designed for real-time ocean current analysis and environmental monitoring.

## 🚀 Features

- **Tilt-Compensated Speed Estimation**: 3D orbital velocity calculation using accelerometer/gyroscope fusion with Earth-frame rotation.
- **Environmental Monitoring**: Integrated temperature and water depth (submersion) sensing.
- **Visual Feedback**: Real-time water level visualization on the Uno R4 LED Matrix.
- **Advanced Signal Processing**: 2nd order low-pass filtering and trapezoidal integration for noise reduction.

## 📂 Project Structure

- `UCPU.ino`: Main orchestration logic.
- `Movement.h/cpp`: High-precision motion tracking module.
- `Environment.h/cpp`: Temperature and depth sensing module.
- `Display.h/cpp`: LED Matrix visualization module.

## 🛠 Hardware Required

- Arduino Uno R4 WiFi (or compatible with LED Matrix)
- Arduino Modulino (Movement & Thermo modules)
- Analog Water Level Sensor (connected to A0)
- Arduino Router Bridge

## ⚙️ Setup

1. Connect the Modulino modules via Qwiic/I2C.
2. Connect the water level sensor to `A0`.
3. Power the system and **keep the buoy flat and still** for 3 seconds during the calibration phase.
4. Open the Serial Monitor at `115200` baud or use the Serial Plotter to visualize current speed and wave patterns.

---
*SPDX-License-Identifier: MPL-2.0*
