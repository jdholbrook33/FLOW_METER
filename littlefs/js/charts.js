// Data storage for different time periods
const timePeriods = {
    hour: {
        labels: [],
        data: []
    },
    day: {
        labels: [],
        data: []
    },
    month: {
        labels: [],
        data: []
    }
};

// Set up the chart data for Chart.js
const chartData = {
    labels: timePeriods.hour.labels,
    datasets: [{
        label: 'Flow Rate (L/min)',
        data: timePeriods.hour.data,
        borderColor: '#FF6B2B',
        backgroundColor: 'rgba(255, 107, 43, 0.1)',
        borderWidth: 2,
        fill: true,
        tension: 0.4
    }]
};

// Chart configuration
const config = {
    type: 'line',
    data: chartData,
    options: {
        responsive: true,
        maintainAspectRatio: false,
        plugins: {
            legend: {
                labels: {
                    color: '#9e9e9e'
                }
            }
        },
        scales: {
            x: {
                grid: {
                    color: '#333333'
                },
                ticks: {
                    color: '#9e9e9e'
                }
            },
            y: {
                grid: {
                    color: '#333333'
                },
                ticks: {
                    color: '#9e9e9e'
                },
                min: 0,
                max: 70
            }
        },
        animation: {
            duration: 0
        }
    }
};

// Create the chart
const flowChart = new Chart(
    document.getElementById('flowChart'),
    config
);

// Function to add new data point to a specific time period
function addDataPoint(timePeriod, flowRate, time) {
    const maxPoints = timePeriod === 'hour' ? 60 : timePeriod === 'day' ? 1440 : 43200;

    timePeriods[timePeriod].labels.push(time);
    timePeriods[timePeriod].data.push(flowRate);

    if (timePeriods[timePeriod].labels.length > maxPoints) {
        timePeriods[timePeriod].labels.shift();
        timePeriods[timePeriod].data.shift();
    }
}

// Function to set the chart to display a specific time period
function setTimePeriod(period) {
    chartData.labels = timePeriods[period].labels;
    chartData.datasets[0].data = timePeriods[period].data;
    flowChart.update('none');  // Update without animation
}

// Function to update chart data (called by main.js)
function updateChartData(data) {
    const now = new Date();
    const timeString = now.toLocaleTimeString();

    // Update all time periods with new data
    addDataPoint('hour', data.flow, timeString);
    addDataPoint('day', data.flow, timeString);
    addDataPoint('month', data.flow, timeString);

    // Refresh the current view
    flowChart.update('none');
}