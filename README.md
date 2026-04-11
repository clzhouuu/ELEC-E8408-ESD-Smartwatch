# HUIPPU Smart Hiking Watch

## Overview
**HUIPPU** is a smartwatch system that tracks hiking sessions using sensor data and syncs them wirelessly to a Raspberry Pi hub for review via a web UI.

- Real-time step, distance, and calorie tracking based on BMA423 accelerometer
- GPS route logging using TinyGPS++
- Bluetooth session sync between watch and Raspberry Pi
- Memory handling on SQL dabase
- Web UI for session history visual and user profile management

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
│   ├── create_dummy_session.py      # Creates a sample session for testing
│   └── requirements.txt             # Python dependencies
│
└── TTGo_FW/                         # Watch
    ├── src/
    │   ├── icons/
    │   │   ├── steps.c              # Steps icon
    │   │   ├── image.c                  # Background image asset
    │   │   ├── fire.c                   # Calorie icon
    │   │   └── person.c                 # Distance icon
    │   ├── main.cpp                 # Main file
    │   ├── config.h                 # Config
    │   ├── globals.h                # Global variable
    │   ├── screens.cpp / screens.h  # LVGL screens
    │   ├── bluetooth.cpp / .h       # BT sync
    │   ├── gps.cpp / gps.h          # GPS and file save
    │   ├── power.cpp / power.h      # Battery, sleep, wake, shutdown, sensor init
    │   ├── saveFile.cpp / .h        # Session file read/write
    │   └──  utils.cpp / utils.h      # LittleFS file system utilities
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


### 2. Start the Hub and Web UI
Within the project folder run:
```bash
source venv/bin/activate
```
Navigate to the `RPi/` folder and run:
```bash
python3 main.py
```
Open a browser and navigate to `http://localhost:5000`.

### 3. Pair Bluetooth
After you run `main.py`, the script attempts to connect to the Bluetooth device. The device MAC address is hard-coded in `bt.py`.
If this is the first time pairing the device, you may need to run:
```bash
bluetoothctl
> scan on
> trust 44:17:93:88:D1:CA
> exit
```

### 4. Set User Profile
In the landing page, click **Settings**, enter the hiker's weight (kg) and height (cm), and click Save. If the watch is already connected via Bluetooth, the information is pushed to the watch automatically.


## Usage

### Watch Operation
- **Idle screen**: Shows current time, date, battery, Bluetooth and GPS status
- **Start a session**: Press the start a hike button on the screen
- **Hike screen**: Displays live steps, distance, duration and burned calories
- **End a session**: Press the end hike button to terminate a session and sync
### Web Interface
- **Home**: User height and weight settings, BT connection status, past sessions combined distance
- **Sessions**: Full session list with date, starting time, steps, distance, duration and kcal as well as a map of the tracked route
- **History**: Chronological session history



## Notes
- If the watch is not Bluetooth connected, and a hike is ended, it will save the session on the local memory.
- Only one session can be stored locally on the watch at a time. Starting a new session without syncing will overwrite the previous one.
- GPS fix can take up to 2 minutes in open sky.
- Calorie estimation uses MET = 6.
- The distance is calculated with an estimated stride length.
