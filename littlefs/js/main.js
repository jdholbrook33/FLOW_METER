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

// Update every second
document.addEventListener('DOMContentLoaded', function() {
    // Initial update
    updateData();
    // Set up interval for updates
    setInterval(updateData, 1000);
});