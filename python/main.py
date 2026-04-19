import time
import joblib
import pandas as pd
import json
import os
import webbrowser
import numpy as np
from datetime import datetime
from collections import deque
from arduino.app_bricks.web_ui import WebUI
from arduino.app_utils import App, Bridge, Logger

logger = Logger("BluePulse")
ui = WebUI(assets_dir_path="/app/assets")

def get_path(filename):
    return os.path.join(os.path.dirname(__file__), filename)

# Load ML Model and Feature List
logger.info("Loading Marine Mode ML Model...")
model = None
feature_cols = []
try:
    model_path = get_path("marine_mode_pipeline.joblib")
    feature_path = get_path("feature_cols.json")
    
    model = joblib.load(model_path)
    with open(feature_path, "r") as f:
        feature_cols = json.load(f)
    
    # --- SCLEARN VERSION PATCH ---
    try:
        if hasattr(model, "named_steps") and "imputer" in model.named_steps:
            imputer = model.named_steps["imputer"]
            if not hasattr(imputer, "_fill_dtype"):
                logger.info("Patching SimpleImputer...")
                imputer._fill_dtype = imputer.statistics_.dtype
    except Exception as patch_e:
        logger.warning(f"Failed to apply SimpleImputer patch: {patch_e}")

    logger.info(f"ML Model loaded. Features: {feature_cols}")
except Exception as e:
    logger.error(f"Failed to load ML model: {e}")

# Buffers to calculate lags, diffs, and means
# We need at least 4 previous points for lag3 and diff calculation
history = {
    "temp": deque(maxlen=10),
    "depth": deque(maxlen=10),
    "speed": deque(maxlen=10)
}

latest_data = {
    "temperature": 0.0, "depth": 0.0, "speed": 0.0,
    "roll": 0.0, "pitch": 0.0, "prediction": "Unknown", "probability": 0.0
}

url_opened = False
diagnostics_printed = False

def loop():
    global latest_data, url_opened, diagnostics_printed
    
    if not url_opened:
        try:
            target_url = ui.local_url
            logger.info(f"Access Web Dashboard at: {target_url}")
            webbrowser.open(target_url)
            url_opened = True
        except Exception as e:
            logger.error(f"Could not auto-open browser: {e}")

    try:
        data = Bridge.call("get_telemetry")
        
        if isinstance(data, (list, tuple)) and len(data) >= 5:
            t_curr = float(data[0])
            d_curr = float(data[1])
            s_curr = float(data[2])
            roll = float(data[3])
            pitch = float(data[4])

            # Update history
            history["temp"].append(t_curr)
            history["depth"].append(d_curr)
            history["speed"].append(s_curr)

            # Ensure we have enough history to avoid missing feature errors
            # (If we don't have enough, we'll fill with current value or 0)
            def get_hist(key, lag=0, default=0.0):
                try:
                    # Indexing from the end: -1 is current, -2 is lag1, etc.
                    return history[key][-(lag+1)]
                except IndexError:
                    return history[key][-1] if history[key] else default

            # --- Feature Engineering ---
            now = datetime.now()
            hour = now.hour
            day_of_year = now.timetuple().tm_yday

            # Prepare Features
            # Mapping based on model's feature_names_in_
            features = {
                "TEMP_clean": [t_curr],
                "PSAL_clean": [d_curr], 
                "current_speed": [s_curr],
                "current_speed_lag1": [get_hist("speed", lag=1)],
                "current_speed_lag2": [get_hist("speed", lag=2)],
                "current_speed_lag3": [get_hist("speed", lag=3)],
                "current_speed_mean_3h": [np.mean(history["speed"])],
                "current_speed_std_3h": [np.std(history["speed"]) if len(history["speed"]) > 1 else 0.0],
                "current_speed_diff_1h": [s_curr - get_hist("speed", lag=1)],
                "psal_diff_1h": [d_curr - get_hist("depth", lag=1)],
                "temp_diff_1h": [t_curr - get_hist("temp", lag=1)],
                "hour_sin": [np.sin(2 * np.pi * hour / 24)],
                "hour_cos": [np.cos(2 * np.pi * hour / 24)],
                "dayofyear_sin": [np.sin(2 * np.pi * day_of_year / 365)],
                "dayofyear_cos": [np.cos(2 * np.pi * day_of_year / 365)]
            }
            
            input_df = pd.DataFrame(features)
            
            if not diagnostics_printed:
                logger.info(f"Generated features: {input_df.columns.tolist()}")
                diagnostics_printed = True

            # Reorder columns to match training
            input_processed = input_df[feature_cols]

            if model:
                # 3. Run Inference
                prediction = model.predict(input_processed)[0]
                probabilities = model.predict_proba(input_processed)[0]
                max_prob = max(probabilities)

                # 4. Update global state for UI
                latest_data = {
                    "temperature": t_curr,
                    "depth": d_curr,
                    "speed": s_curr,
                    "roll": roll,
                    "pitch": pitch,
                    "prediction": str(prediction),
                    "probability": float(max_prob)
                }

    except Exception as e:
        logger.error(f"Processing Error: {str(e)}")
        
    time.sleep(0.1)

def get_live_data():
    return latest_data

ui.expose_api("GET", "/data", get_live_data)

if __name__ == "__main__":
    App.run(user_loop=loop)
