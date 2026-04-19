import time
import joblib
import pandas as pd
import json
from arduino.app_utils import App, Bridge

# Load ML Model and Feature List
print("Loading Marine Mode ML Model...")
model = joblib.load("marine_mode_pipeline.joblib")
with open("feature_cols.json", "r") as f:
    feature_cols = json.load(f)

# Global state to store the latest results for the frontend
latest_data = {
    "temperature": 0.0,
    "depth": 0.0,
    "speed": 0.0,
    "roll": 0.0,
    "pitch": 0.0,
    "prediction": "Unknown",
    "probability": 0.0
}

def loop():
    global latest_data
    try:
        # 1. Fetch data from STM32 Bridge
        temp = Bridge.get("temperature")
        depth = Bridge.get("depth")
        speed = Bridge.get("speed")
        roll = Bridge.get("roll")
        pitch = Bridge.get("pitch")

        if temp is not None:
            # 2. Prepare data for ML Inference
            # The model expects a DataFrame with specific column names
            input_data = pd.DataFrame([{
                "temperature": temp,
                "depth": depth,
                "speed": speed,
                "roll": roll,
                "pitch": pitch
            }])
            
            # Ensure we only use the columns the model was trained on
            # (In case there are extra columns or specific ordering needed)
            input_df = input_data[feature_cols]

            # 3. Run Inference
            prediction = model.predict(input_df)[0]
            probabilities = model.predict_proba(input_df)[0]
            max_prob = max(probabilities)

            # 4. Update global state
            latest_data = {
                "temperature": float(temp),
                "depth": float(depth),
                "speed": float(speed),
                "roll": float(roll),
                "pitch": float(pitch),
                "prediction": str(prediction),
                "probability": float(max_prob)
            }
            
            print(f"Update: {prediction} ({max_prob:.2f}) | Speed: {speed:.2f}")

    except Exception as e:
        print(f"Error in loop: {e}")
        
    time.sleep(0.1)

# API Endpoint for the index.html frontend
@App.api("/data")
def get_live_data():
    return latest_data

# Start the App Lab framework
if __name__ == "__main__":
    App.run(user_loop=loop)
