# Pool Monitoring System

## Project Structure
```
FLOWMETER/
├── CMakeLists.txt
├── partitions.csv          # Custom partition table
├── littlefs/              # Web content directory
│   ├── index.html
│   ├── css/
│   ├── js/
│   │   └── chart.umd.js
│   └── data/
├── main/
│   ├── CMakeLists.txt
│   └── main.c
└── components/
    ├── wifi_manager/
    │   ├── CMakeLists.txt
    │   ├── Kconfig.projbuild
    │   ├── include/
    │   │   └── wifi_manager.h
    │   └── wifi_manager.c
    ├── flow_sensor/
    │   ├── CMakeLists.txt
    │   ├── Kconfig.projbuild
    │   ├── include/
    │   │   └── flow_sensor.h
    │   └── flow_sensor.c
    ├── rtc_manager/
    │   ├── CMakeLists.txt
    │   ├── include/
    │   │   └── rtc_manager.h
    │   └── rtc_manager.c
    ├── storage_manager/
    │   ├── CMakeLists.txt
    │   ├── include/
    │   │   └── storage_manager.h
    │   └── storage_manager.c
    └── web_server/
        ├── CMakeLists.txt
        ├── include/
        │   └── web_server.h
        └── web_server.c
```

## Components Implemented

### 1. WiFi Manager Component
- Implements WiFi Access Point (AP) functionality
- Configurable through menuconfig:
  - SSID
  - Password
  - Channel
  - Maximum connections
- Handles station connections/disconnections
- Provides clear logging of WiFi events

### 2. Flow Sensor Component
- Handles pulse counting for flow measurement
- Features:
  - Real-time flow rate calculation (L/min)
  - Cumulative volume tracking (L)
  - High-speed interrupt-based pulse counting
  - Configurable through menuconfig:
    - GPIO pin (default: GPIO 3)
    - Pulses per liter (default: 300)
    - Update interval (default: 1000ms)
- Performance:
  - Accurately tracks flow rates from 20-50 L/min
  - Smooth transition handling
  - Consistent volume accumulation

### 3. RTC Manager Component
- Integrates with DS3231 RTC module
- Features:
  - I2C communication (SCL: GPIO 7, SDA: GPIO 6)
  - Maintains accurate time using RTC's temperature-compensated crystal
  - RTC keeps UTC time for accuracy
  - Automatically converts to CST (UTC-6) for local display
  - High-precision temperature sensor (0.25°C resolution)
  - ISO 8601 date format (YYYY-MM-DD)
  - 24-hour time format

### 4. Storage Manager Component (New)
- Manages LittleFS file system
- Features:
  - Automatic mounting of LittleFS partition
  - File operations (read/write)
  - Storage statistics
  - Partition size: 1.5MB

### 5. Web Server Component (New)
- Serves web content from LittleFS
- Features:
  - Handles multiple file types (HTML, CSS, JS)
  - Automatic MIME type detection
  - Root path handling
  - Static file serving
  - Clean URL handling

## Partition Table
Custom partition table implemented:
```
# Name,   Type, SubType,  Offset,   Size,    Flags
nvs,      data, nvs,      0x9000,   0x4000,
phy_init, data, phy,      0xf000,   0x1000,
factory,  app,  factory,  0x10000,  0x280000,
storage,  data, littlefs, 0x290000, 0x170000,
```

## Web Content
- Files stored in LittleFS partition
- Created using littlefs_create_partition_image
- Command: `littlefs_create_partition_image(storage ../littlefs FLASH_IN_PROJECT)`

## Data Format
All measurements use metric units:
- Flow Rate: Liters per minute (L/min)
- Volume: Liters (L)
- Temperature: Celsius (°C)
- Time: 24-hour format
- Date: ISO 8601 (YYYY-MM-DD)

## Sample Output
```
[2024-11-14 15:32:47 CST] Flow rate: 40.00 L/min, Total: 214.92 L, Temp: 27.50°C
```

## Build and Flash
```bash
idf.py menuconfig    # Configure WiFi and other settings
idf.py build
idf.py -p [PORT] flash monitor
```

## Access
After flashing:
1. Connect to WiFi AP "FLOWMETER" (password defined in menuconfig)
2. Access web interface at http://192.168.4.1

## Next Steps
1. Implement real-time data updates via WebSocket
2. Add data logging capabilities
3. Enhance web interface with charts and controls