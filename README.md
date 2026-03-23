# HUIPPU Smart Hiking Watch

## Overview
**HUIPPU** is a smartwatch system that tracks hiking sessions using sensor data and syncs them wirelessly to a Raspberry Pi hub for review via a web UI.

- Real-time step, distance, and calorie tracking via BMA423 accelerometer
- GPS route logging using TinyGPS++
- Bluetooth session sync between watch and Raspberry Pi
- Web UI for session history and user profile management

## Repository structure
```
Watch_Code/
├── RPi/                             # Raspberry Pi
│   ├── static/                      # Web UI static
│   ├── templates/                   # Flask HTML templates
│   ├── bt.py                        # Bluetooth connection and sync
│   ├── db.py                        # SQLite database
│   ├── hike.py                      # Hike session
│   ├── outbound_package.py          # Outbound data package
│   ├── receiver.py                  # Bluetooth receiver
│   ├── wserver.py                   # Flask server
│   └── requirements.txt             # Python dependencies
│
└── TTGo_FW/                         # Watch
    ├── src/
    │   ├── main.cpp                 # Main file
    │   ├── config.h                 # Config
    │   ├── globals.h                # Global variable
    │   ├── screens.cpp / screens.h  # LVGL screens
    │   ├── bluetooth.cpp / .h       # BT sync
    │   ├── gps.cpp / gps.h          # GPS and file save
    │   ├── power.cpp / power.h      # Battery, sleep, wake, shutdown, sensor init
    │   ├── saveFile.cpp / .h        # Session file read/write
    │   ├── utils.cpp / utils.h      # LittleFS file system utilities
    │   ├── image.c                  # Background image asset
    │   ├── fire.c                   # Calorie icon
    │   ├── steps.c                  # Steps icon
    │   └── person.c                 # Distance icon
    └── platformio.ini               # PlatformIO build configuration
```

## Getting Started

### Prerequisites

#### Watch (LILYGO TTGO T-Watch)
- Visual Studio Code with the PlatformIO extension 
- Library and platform versions are managed automatically via `platformio.ini`:
  - espressif32@6.8.1
  - TTGO TWatch Library @ ^1.4.2

#### Hub (Raspberry Pi)
- Python 3.8 or higher
- Bluetooth enabled and working on the RPi
- Install dependencies using 
    `pip install -r requirements.txt`

  This includes:
  - PyBluez
  - Flask 2.2.5
  - Pillow

## Setup

### 1. Flash the Watch
Open the `TTGo_FW/` folder in VS Code with PlatformIO. The folder should contain a platformio.ini file. Connect the T-Watch via USB and click the arrow icon in the PlatformIO toolbar on the bottom of the window, or run:
`pio run`

### 2. Pair Bluetooth
On the Raspberry Pi, pair with the watch:
```bash
bluetoothctl
> scan on
> pair 44:17:93:88:D1:CA
> trust 44:17:93:88:D1:CA
> exit
```

### 3. Start the Hub and Web UI
Navigate to the `RPi/` folder and run:
```bash
python3 main.py
```
Open a browser and navigate to `http://localhost:5000`.

### 4. Set User Profile
 In the landing page, click **Settings**, enter the hiker's weight (kg) and height (cm), and click Save. If the watch is already connected via Bluetooth, the information is pushed to the watch automatically.


## Usage

### Watch Operation
- **Idle screen**: shows current time, battery, Bluetooth and GPS status
- **Start a session**: press the physical side button
- **Hike screen**: displays live steps, distance, duration and calories
- **End a session**: press the button again and data is saved and synced if Bluetooth is connected

### Web Interface
- **Home**: Overview of all synced sessions and BT connection status, watch on the side shows all past sessions combined distance
- **Sessions**: Full session list with steps, distance, duration and kcal as well as a map of the tracked route
- **History**: Chronological session history
- **Settings**: Update user weight and height


## Notes
- Only one session can be stored locally on the watch at a time. Starting a new session without syncing will overwrite the previous one.
- GPS fix can take up to 2 minutes in open sky.
- Calorie estimation uses MET = 6.
- The distance is calculated with an estimated stride length.
