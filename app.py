from flask import Flask, request, jsonify
import joblib
import numpy as np
import pandas as pd
from flask_cors import CORS
from werkzeug.utils import secure_filename
import os

app = Flask(__name__)
CORS(app)  # Enable CORS for all routes

# Load your pre-trained models
linear_regression_model = joblib.load('linear_regression_model.pkl')
logistic_regression_model = joblib.load('logistic_regression_model.pkl')

# Define the upload folder
UPLOAD_FOLDER = 'uploads'
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

# Ensure the upload folder exists
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

@app.route('/upload', methods=['POST'])
def upload_file():
    if 'file' not in request.files:
        return jsonify({'error': 'No file part'}), 400
    
    file = request.files['file']
    if file.filename == '':
        return jsonify({'error': 'No selected file'}), 400

    filename = secure_filename(file.filename)
    filepath = os.path.join(app.config['UPLOAD_FOLDER'], filename)
    file.save(filepath)

    try:
        # Read the file into a DataFrame
        if filename.endswith('.csv'):
            df = pd.read_csv(filepath)
        else:
            df = pd.read_excel(filepath)
        
        # Assuming the file has the necessary columns: HRV, SpO2, Accelerometer, Gyroscope
        features = df[['HRV', 'SpO2', 'Accelerometer', 'Gyroscope']].values
        
        # Perform predictions
        stress_levels = linear_regression_model.predict(features)
        heart_alerts = logistic_regression_model.predict(features)
        
        # Convert to native Python types and return the results
        results = {
            'Stress Level': [float(stress) for stress in stress_levels],
            'Heart Alert': [int(alert) for alert in heart_alerts]
        }
        
        return jsonify(results)
    except Exception as e:
        app.logger.error(f"Error during file processing: {e}")
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    app.run(debug=True)
