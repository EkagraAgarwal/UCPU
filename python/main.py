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


# ----------------------------
# Model configuration
# ----------------------------
MODEL_FILES = {
    "1h": "marine_power_score_model_1h.joblib",
    "2h": "marine_power_score_model_2h.joblib",
    "3h": "marine_power_score_model_3h.joblib",
}
FEATURE_FILE = "feature_cols.json"

models = {}
feature_cols = []


def patch_simple_imputer_if_needed(loaded_model):
    """Patch sklearn SimpleImputer compatibility issue across versions."""
    try:
        if hasattr(loaded_model, "named_steps") and "imputer" in loaded_model.named_steps:
            imputer = loaded_model.named_steps["imputer"]
            if not hasattr(imputer, "_fill_dtype") and hasattr(imputer, "statistics_"):
                logger.info("Patching SimpleImputer...")
                imputer._fill_dtype = imputer.statistics_.dtype
    except Exception as patch_e:
        logger.warning(f"Failed to apply SimpleImputer patch: {patch_e}")


logger.info("Loading Marine Power Score models...")
try:
    feature_path = get_path(FEATURE_FILE)
    with open(feature_path, "r") as f:
        feature_cols = json.load(f)

    for horizon, filename in MODEL_FILES.items():
        model_path = get_path(filename)
        loaded_model = joblib.load(model_path)
        patch_simple_imputer_if_needed(loaded_model)
        models[horizon] = loaded_model
        logger.info(f"Loaded {horizon} model from {filename}")

    logger.info(f"Feature columns loaded: {feature_cols}")
except Exception as e:
    logger.error(f"Failed to load ML assets: {e}")


# ----------------------------
# History buffers
# ----------------------------
history = {
    "temp": deque(maxlen=10),
    "depth": deque(maxlen=10),
    "speed": deque(maxlen=10)
}


latest_data = {
    "temperature": 0.0,
    "depth": 0.0,
    "speed": 0.0,
    "roll": 0.0,
    "pitch": 0.0,
    "power_score_1h": 0,
    "power_score_2h": 0,
    "power_score_3h": 0,
    "strategy": "Unknown",
    # compatibility fields for older frontends
    "prediction": "Unknown",
    "probability": 0.0,
}

url_opened = False
diagnostics_printed = False


# ----------------------------
# Helpers
# ----------------------------
def curve_power_score(raw_value, raw_min=0.0, raw_max=45.0, gamma=1.2):
    """
    Map raw model output from roughly 0-50 into a more spread-out 0-100 score.

    gamma > 1:
    - pushes mid/low values downward
    - creates more contrast across the range
    - keeps high values more clearly separated
    """
    try:
        x = float(raw_value)
    except Exception:
        return 0

    # normalize into 0-1
    x = (x - raw_min) / (raw_max - raw_min)
    x = max(0.0, min(1.0, x))

    # curve it
    curved = x ** gamma

    # scale to 0-100
    return int(round(curved * 100))



def get_hist(key, lag=0, default=0.0):
    try:
        return history[key][-(lag + 1)]
    except IndexError:
        return history[key][-1] if history[key] else default



def build_feature_frame(t_curr, d_curr, s_curr):
    now = datetime.now()
    hour = now.hour
    day_of_year = now.timetuple().tm_yday

    features = {
        "TEMP_clean": [t_curr],
        "PSAL_clean": [d_curr],
        "current_speed": [s_curr],
        "current_speed_lag1": [get_hist("speed", lag=1)],
        "current_speed_lag2": [get_hist("speed", lag=2)],
        "current_speed_lag3": [get_hist("speed", lag=3)],
        "current_speed_mean_3h": [float(np.mean(history["speed"])) if history["speed"] else s_curr],
        "current_speed_std_3h": [float(np.std(history["speed"])) if len(history["speed"]) > 1 else 0.0],
        "current_speed_diff_1h": [s_curr - get_hist("speed", lag=1)],
        "psal_diff_1h": [d_curr - get_hist("depth", lag=1)],
        "temp_diff_1h": [t_curr - get_hist("temp", lag=1)],
        "hour_sin": [np.sin(2 * np.pi * hour / 24)],
        "hour_cos": [np.cos(2 * np.pi * hour / 24)],
        "dayofyear_sin": [np.sin(2 * np.pi * day_of_year / 365)],
        "dayofyear_cos": [np.cos(2 * np.pi * day_of_year / 365)],
    }

    input_df = pd.DataFrame(features)
    return input_df[feature_cols]



def predict_power_scores(input_processed):
    scores = {}
    for horizon, model in models.items():
        raw_pred = model.predict(input_processed)[0]
        scores[horizon] = curve_power_score(raw_pred)
    return scores



def choose_strategy(score_1h, score_2h, score_3h):
    """
    Convert 3 forecast horizons into a single operating strategy.

    Heuristic goals:
    - prioritize near-term conditions because the device/turbine can act on them first
    - still look ahead to avoid overreacting to short spikes or dips
    - produce stable, judge-friendly labels for the demo
    """
    weighted_score = 0.75 * score_1h + 0.2 * score_2h + 0.05 * score_3h
    improving = (score_3h - score_1h) >= 20
    declining = (score_1h - score_3h) >= 20

    if weighted_score >= 76:
        return "Harvest Priority"

    if improving and weighted_score >= 60:
        return "Sense Only"

    if declining and score_1h < 40:
        return "Sleep"

    if weighted_score >= 35:
        return "Sense Only"

    return "Sleep"



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

            history["temp"].append(t_curr)
            history["depth"].append(d_curr)
            history["speed"].append(s_curr)

            input_processed = build_feature_frame(t_curr, d_curr, s_curr)
            if not diagnostics_printed:
                logger.info(f"Generated features: {input_processed.columns.tolist()}")
                diagnostics_printed = True

            if models:
                scores = predict_power_scores(input_processed)
                score_1h = scores.get("1h", 0)
                score_2h = scores.get("2h", 0)
                score_3h = scores.get("3h", 0)
                strategy = choose_strategy(score_1h, score_2h, score_3h)

                latest_data = {
                    "temperature": t_curr,
                    "depth": d_curr,
                    "speed": s_curr,
                    "roll": roll,
                    "pitch": pitch,
                    "power_score_1h": score_1h,
                    "power_score_2h": score_2h,
                    "power_score_3h": score_3h,
                    "strategy": strategy,
                    # compatibility fields for existing UI code
                    "prediction": strategy,
                    "probability": float(max(score_1h, score_2h, score_3h) / 100.0),
                }

    except Exception as e:
        logger.error(f"Processing Error: {str(e)}")

    time.sleep(0.1)



def get_live_data():
    return latest_data


ui.expose_api("GET", "/data", get_live_data)

if __name__ == "__main__":
    App.run(user_loop=loop)
