function updateData() {
    fetch('/data')
        .then(response => response.json())
        .then(data => {
            document.getElementById('flow-rate').textContent = data.flow.toFixed(2);
            document.getElementById('total-volume').textContent = data.volume.toFixed(2);
            document.getElementById('update-time').textContent = data.time;
            updateChartData(data);  // Update the chart
        })
        .catch(error => console.error('Error:', error));
}

function setLoggingInterval() {
    const interval = document.getElementById('logging-interval').value;

    fetch('/set_interval', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({ interval: parseInt(interval) })
    })
    .then(response => {
        if (response.ok) {
            alert("Logging interval updated successfully!");
        } else {
            alert("Failed to update logging interval.");
        }
    })
    .catch(error => {
        console.error('Error:', error);
        alert("Error setting logging interval.");
    });
}


// Update every second
document.addEventListener('DOMContentLoaded', function() {
    // Initial update
    updateData();
    // Set up interval for updates
    setInterval(updateData, 1000);
});