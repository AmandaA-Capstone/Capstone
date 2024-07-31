document.addEventListener('DOMContentLoaded', function() {
    const form = document.getElementById('upload-form');
    form.addEventListener('submit', function(event) {
        event.preventDefault();
        
        const fileInput = document.getElementById('file');
        const formData = new FormData();
        formData.append('file', fileInput.files[0]);

        fetch('http://127.0.0.1:5000/upload', {
            method: 'POST',
            body: formData
        })
        .then(response => response.json())
        .then(data => {
            const resultDiv = document.getElementById('upload-result');
            if (data.error) {
                resultDiv.innerHTML = `<p>Error: ${data.error}</p>`;
            } else {
                resultDiv.innerHTML = `<p>File uploaded successfully. Stress Level: ${data['Stress Level']}, Heart Alert: ${data['Heart Alert']}</p>`;
            }
        })
        .catch(error => {
            console.error('Error:', error);
            const resultDiv = document.getElementById('upload-result');
            resultDiv.innerHTML = `<p>Error: ${error.message}</p>`;
        });
    });
});
