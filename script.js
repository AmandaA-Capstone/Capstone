document.addEventListener('DOMContentLoaded', function() {
    const form = document.getElementById('prediction-form');
    if (form) {
        form.addEventListener('submit', function(event) {
            event.preventDefault();
            
            const hrv = document.getElementById('hrv').value;
            const spo2 = document.getElementById('spo2').value;
            const accelerometer = document.getElementById('accelerometer').value;
            const gyroscope = document.getElementById('gyroscope').value;
            
            fetch('http://127.0.0.1:5000/predict', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    HRV: parseFloat(hrv),
                    SpO2: parseFloat(spo2),
                    Accelerometer: parseFloat(accelerometer),
                    Gyroscope: parseFloat(gyroscope)
                })
            })
            .then(response => response.json())
            .then(data => {
                const resultDiv = document.getElementById('result');
                if (data['Stress Level'] !== undefined && data['Heart Alert'] !== undefined) {
                    resultDiv.innerHTML = `<p>Stress Level: ${data['Stress Level']}</p><p>Heart Alert: ${data['Heart Alert']}</p>`;
                } else if (data['error']) {
                    resultDiv.innerHTML = `<p>Error: ${data['error']}</p>`;
                } else {
                    resultDiv.innerHTML = `<p>Prediction failed. Please try again.</p>`;
                }
            })
            .catch(error => {
                console.error('Error:', error);
                const resultDiv = document.getElementById('result');
                resultDiv.innerHTML = `<p>Error: ${error.message}</p>`;
            });
        });
    }
});
