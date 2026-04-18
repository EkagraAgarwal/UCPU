# Wave Energy Prediction Buoy

## Project Overview

An Arduino Uno Q-based remote marine device that predicts turbine power generation in real-time before waves arrive. The system uses sensor fusion and machine learning to provide 30-60 second advance predictions of wave energy potential.

---

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    ARDUINO UNO Q (Qualcomm)                 │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   ┌─────────────┐    ┌─────────────┐    ┌─────────────┐     │
│   │   SENSORS   │───▶│  DECISION   │───▶│  OUTPUT     │     │
│   │   MODULE    │    │   ENGINE    │    │  MODULE     │     │
│   └─────────────┘    └─────────────┘    └─────────────┘     │
│         │                   │                   │           │
│         ▼                   ▼                   ▼           │
│   ┌─────────────┐    ┌─────────────┐    ┌─────────────┐     │
│   │    DATA     │    │  PREDICTION │    │ TRANSMITTER │     │
│   │   FUSION    │───▶│   ENGINE    │───▶│  (WiFi)     │     │
│   └─────────────┘    └─────────────┘    └─────────────┘     │
│                                                             │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
                   ┌─────────────────┐
                   │  WEB DASHBOARD  │
                   │  (Real-time)    │
                   └─────────────────┘
```

---

## System Components

### 1. Sensor Layer

| Sensor | Purpose |
|--------|---------|
| Temperature/Humidity | Atmospheric pressure changes correlate with incoming wave patterns |
| Tilt Switch | Detects buoy rocking motion to measure wave height and frequency |
| Water Level | Measures immediate wave height at buoy location |
| Sound Sensor | Detects wave crash intensity for calibration |
| Camera | Visual detection of approaching wave crests |

### 2. Data Fusion Layer

- Normalizes raw sensor values to standardized ranges
- Calculates derivatives (rate of change over time)
- Creates time-window features (rolling averages, peaks, patterns)
- Merges multi-sensor data into unified feature vector

### 3. Prediction Engine

**Primary Path: ML Model (TFLite)**
- Pre-trained on Scripps historical wave and power data
- Input: Feature vector from sensor fusion
- Output: Predicted power, arrival time, confidence score

**Fallback Path: Heuristics**
- Rule-based predictions when ML confidence is low
- Uses simple thresholds and pattern matching
- Ensures system always provides output

### 4. Communication Layer

- Transmits predictions via WiFi to web dashboard
- Broadcast format: JSON payload with power, ETA, confidence, timestamp
- Handles connection drops with retry logic

---

## Data Flow

```
RAW SENSOR DATA
       │
       ▼
┌──────────────────┐
│  SENSOR MANAGER  │  ← Poll all sensors at intervals
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│  DATA NORMALIZER │  ← Convert to 0-1 range
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ FEATURE BUILDER  │  ← Create ML input features
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│    DECISION      │  ← Choose: ML or Heuristic?
│    ENGINE        │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│   PREDICTION     │  │ • Predicted Power (kW)
│    OUTPUT        │  │ • Arrival Time (seconds)
└────────┬─────────┘  │ • Confidence Score (%)
         │
         ▼
┌──────────────────┐
│   TRANSMITTER    │  ← Send to dashboard
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ WEB DASHBOARD    │  ← Real-time visualization
└──────────────────┘
```

---

## ML Pipeline

```
┌─────────────────────┐
│   SCRIPPS DATASET   │
│ (Historical waves   │
│  + power output)    │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│   PREPROCESSING     │
│ • Clean data        │
│ • Normalize values  │
│ • Handle gaps       │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│ FEATURE ENGINEERING │
│ • Wave height       │
│ • Wave period       │
│ • Temperature delta │
│ • Tilt patterns     │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│   MODEL TRAINING    │
│ • Time-series model │
│ • Input: 30s window │
│ • Output: t+30s     │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│    EVALUATION       │
│ • Test accuracy     │
│ • Measure latency   │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  CONVERT TO TFLITE  │
│ • Quantize for MCU  │
│ • Optimize size     │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  DEPLOY TO ARDUINO  │
│   UNO Q (4GB RAM)   │
└─────────────────────┘
```

---

## Dashboard Components

| Section | Content |
|---------|---------|
| Current Prediction | Power (kW), ETA (seconds), Confidence (%) |
| Power Forecast Graph | Predicted power output over next 60 seconds |
| Sensor Readings | Live temp, humidity, tilt, water level, sound |
| Accuracy History | Comparison of predicted vs actual power |
| System Status | Connection health, battery level, sensor status |

---

## Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| Pre-trained model + Heuristics | Ensures reliability; heuristics provide fallback |
| Web dashboard output | Easy demo visualization; accessible from any browser |
| Qwiic sensors | Simplified I2C wiring; plug-and-play for hackathon |
| Qualcomm Q processor | Sufficient RAM (4GB) for TFLite inference |
| Scripps dataset | Real wave data improves model accuracy |

---

## Real-World Application

- **Grid operators** can anticipate renewable energy influx
- **Turbine controllers** can adjust for optimal efficiency
- **Energy storage systems** can prepare for incoming power surges
- **Research institutions** can validate wave energy conversion models

---

## Hackathon Demo Flow

1. Show live sensor readings on dashboard
2. Wave simulation (rock the buoy) triggers sensors
3. ML model predicts incoming power
4. Dashboard displays: "Wave arriving in 30s, ~2.5kW expected"
5. Compare prediction to actual simulated turbine output
#   U C P U  
 