# ESP32-T4-S3-Template

Welcome to the lab using the ESP32-T4-S3 LilyGO!

This repository serves as a foundation for software engineering projects aimed at the ESP32 hardware.
This document will guide you through the setup process and help you prepare to work with your ESP32 hardware.

---

# How to get started

This project will work regardless of the operating system you use, and there are appended guides for each below.
Each of these tutorials is done on a fresh install of Visual Studio Code, which is the IDE you must use to complete this project.
Setting up and configuring the project will take multiple minutes, up to 15 min so be patient with the software.

## General

1. Install Visual Studio Code
   * Visit [Visual Studio Code's website](https://code.visualstudio.com/download) and download the latest version or use the package manager of your system.
   * Run `Visual Studio Code` and follow the steps.
2. Install the correct extension.
   * Head over to the extensions tab on your left.
   * Search for ["PlatformIO IDE"](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) and install it.

## Windows

3. Install the Clang compiler and C++ extension for Visual Studio Code.
   * (optional) If you want/need to use Windows Subsystem for Linux (WSL) have at the [documentation](https://code.visualstudio.com/docs/remote/wsl).

## Linux

3. Install the Clang compiler and C++ extension for Visual Studio Code.

4. Run the following command to add access to the `ttyACM0` port in order to avoid complications [1]
   * `sudo adduser $USER $(stat --format="%G" /dev/ttyACM0 )`

## MacOS

3. Run `git -v` in the Terminal app to check whether git and the macOS command line tools are installed. If the command line tools are not installed, a pop up will appear to install it.

## How to run the program

1. Open the locally cloned repository with Visual Studio Code
    * If the "Do you trust the authors of the files in this folder?" dialog appears, click on "Yes, I trust the authors"
2. Open up the file `project/project.ino`
3. Connect your ESP32 to your computer via a USB cable.
4. Build and upload the project to the device. See screenshot.

![[screenshot](./assets/screenshot.png)](./assets/screenshot.png)

---

## What to add where

Your code belongs in one file and one file only. The only place you should add, change, or remove things from is the [**project.ino**](project/project.ino) file in the `project` folder. Changing anything else might break the code and cause a lot of headaches for all involved parties.

## Areas of addition

You are meant to write a small application for the computer that retrieves the weather for a given location, and most of that code belongs in your own functions and the `void loop()` function at the bottom of the `project.ino` file.

## How to Connect to WiFi

The ESP32-T4-S3 LilyGO has a built-in WiFi module, which allows you to connect to the internet.
However, it does not work with eduroam, so you will need to connect to a different WiFi network.
To connect to WiFi, you need to provide your SSID and password in the [project/project.ino](./project/project.ino) file.

---

# Project Implementation - Weather Data Display System

## Overview

This project implements a complete weather data display system for the ESP32 T-Display-AMOLED that fetches and displays weather information from SMHI's (Swedish Meteorological and Hydrological Institute) API.

**Version:** 1.0  
**Group:** GROUP-6

## Features Implemented

### User Stories

#### ✅ US1.1 - Boot Screen
- Displays program version (v1.0) and group number (GROUP-6)
- Shows for 3 seconds on startup
- Provides visual feedback during system initialization

#### ✅ US1.2 - Weather Forecast Display
- Shows 7-day weather forecast for Karlskrona
- Displays data at 12:00 for each day
- Includes: date, temperature (°C), weather conditions, precipitation
- Updates automatically on startup

#### ✅ US2.1 - Touch Navigation
- Implemented tileview with horizontal swipe navigation
- Three screens accessible by sliding finger over touchscreen:
  - **Screen 1:** 7-day weather forecast
  - **Screen 2:** Historical weather data
  - **Screen 3:** Temperature line graph
- Smooth scrolling between screens

#### ✅ US3.1 - Historical Weather Data
- Displays recent historical weather measurements
- Shows date, time, and temperature
- Data sourced from SMHI's observation station in Karlskrona

#### ✅ US3.2 - Temperature Graph
- Line graph visualization of historical temperature data
- Shows temperature trends over time
- Interactive chart with proper scaling and labels

### Non-Functional Requirements

#### ✅ NF1 - Coding Standard
- Well-structured code with clear function separation
- Consistent naming conventions (snake_case for functions, camelCase for variables)
- Proper use of C++ features and Arduino framework
- Type safety and const correctness

#### ✅ NF2 - Documentation
- **Inline comments:** Every function, struct, and important code block documented
- **Doxygen-style comments:** Function descriptions with @brief, @details, @param, @return, @note tags
- **File header:** Complete file documentation with dependencies and description
- **Section headers:** Code organized into logical sections with clear headers

#### ✅ NF3 - License Compliance
- Uses SMHI Open Data API (open license, attribution provided)
- All libraries used are properly licensed (see platformio.ini)
- No proprietary code included

## Technical Architecture

### Data Flow
```
1. System Boot → Initialize Hardware → Show Boot Screen
2. Connect WiFi → Fetch Forecast → Fetch Historical Data
3. Parse JSON → Store in Arrays → Update UI
4. User Interaction → Swipe Navigation → Display Different Views
```

### API Integration

#### SMHI Forecast API
- **Endpoint:** `https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/15.5866/lat/56.1621/data.json`
- **Location:** Karlskrona (56.1621°N, 15.5866°E)
- **Data:** 7-day forecast with temperature, precipitation, weather symbols

#### SMHI Historical API
- **Endpoint:** `https://opendata-download-metobs.smhi.se/api/version/1.0/parameter/1/station/66110/period/latest-months/data.json`
- **Station:** 66110 (Karlskrona)
- **Parameter:** 1 (Temperature)
- **Period:** Latest months

### Code Structure

```cpp
// 1. Includes & Dependencies
// 2. Configuration Constants
// 3. Global Objects
// 4. Data Structures (WeatherData, HistoricalData)
// 5. Helper Functions (date/time parsing, weather descriptions)
// 6. API Functions (fetch data from SMHI)
// 7. UI Functions (create and update screens)
// 8. Network Functions (WiFi connection)
// 9. Main Functions (setup, loop)
```

### Memory Management
- Static arrays for forecast (7 entries) and history (30 entries)
- JSON parsing with ArduinoJson library
- Efficient string handling to minimize heap fragmentation

## Configuration

### WiFi Setup
Before running, update the WiFi credentials in `project/project.ino`:

```cpp
static const char* WIFI_SSID     = "YOUR_SSID";
static const char* WIFI_PASSWORD = "YOUR_PASSWORD";
```

⚠️ **Important:** Delete credentials before committing to GitHub!

### Customization Options

You can modify these constants in `project.ino`:

```cpp
// Change boot screen duration (milliseconds)
static const uint32_t BOOT_SCREEN_DURATION = 3000;

// Update location coordinates for different city
static const char* SMHI_FORECAST_API = "https://...lon/XX.XXXX/lat/YY.YYYY/...";

// Adjust number of forecast days (max 7)
// Adjust history entries (max 30)
```

## Dependencies

All dependencies are automatically managed by PlatformIO (see `platformio.ini`):

- **Arduino Framework** - Core functionality
- **WiFi** - Network connectivity
- **HTTPClient** - API requests
- **ArduinoJson 7.2.1** - JSON parsing
- **LVGL 8.4.0** - Graphics library
- **LilyGo_AMOLED** - Display driver
- **TFT_eSPI 2.5.0** - Display library

## Usage Instructions

### First Time Setup
1. Clone the repository
2. Open in VS Code with PlatformIO
3. Update WiFi credentials in `project/project.ino`
4. Connect ESP32 device via USB
5. Build and upload (see screenshot in main README)

### Navigation
- **Power On:** Boot screen appears for 3 seconds
- **Screen 1 (Forecast):** Swipe left to go to Screen 2
- **Screen 2 (History):** Swipe left for Screen 3, right for Screen 1
- **Screen 3 (Graph):** Swipe right to go back to Screen 2

### Troubleshooting

#### WiFi Won't Connect
- Check SSID and password are correct
- Ensure network is 2.4GHz (ESP32 doesn't support 5GHz)
- Avoid eduroam (not supported)

#### No Data Displayed
- Check WiFi connection (see Serial Monitor)
- Verify internet access
- SMHI API may be temporarily unavailable

#### Upload Failed
- Enter manual upload mode:
  1. Press and hold BOOT button
  2. Press RST button
  3. Release RST button
  4. Release BOOT button
  5. Try upload again

#### Serial Monitor Not Working
- Ensure USB CDC is enabled in platformio.ini
- Try different USB cable/port
- Check baud rate is set to 115200

## Future Enhancements

Potential features for future versions:
- City selection menu
- Multiple location support
- Weather alerts and notifications
- Configurable update intervals
- Data caching for offline viewing
- More detailed weather parameters (wind, humidity, pressure)
- Custom themes and color schemes
- Battery percentage display
- Auto-refresh at specified intervals

## API Attribution

Weather data provided by:
- **SMHI (Swedish Meteorological and Hydrological Institute)**
- Open Data License
- https://opendata.smhi.se/

## Authors

**GROUP-6**  
Software Engineering Project  
2025


---

---

# LilyGo tutorial

From LilyGO [link](https://github.com/Xinyuan-LilyGO/LilyGo-AMOLED-Series)

## 1️⃣ PlatformIO Quick Start (Recommended)

1. Install [Visual Studio Code](https://code.visualstudio.com/) and [Python](https://www.python.org/)
2. Search for the `PlatformIO` plugin in the `VisualStudioCode` extension and install it.
3. After the installation is complete, you need to restart `VisualStudioCode`
4. After restarting `VisualStudioCode`, select `File` in the upper left corner of `VisualStudioCode` -> `Open Folder` -> select the `LilyGO AMOLED Series` directory
5. Wait for the installation of third-party dependent libraries to complete
6. Click on the `platformio.ini` file, and in the `platformio` column
7. Uncomment one of the lines `src_dir = xxxx` to make sure only one line works
8. Click the (✔) symbol in the lower left corner to compile
9. Connect the board to the computer USB
10. Click (→) to upload firmware
11. Click (plug symbol) to monitor serial output
12. If it cannot be written, or the USB device keeps flashing, please check the **FAQ** below


# 2️⃣ FAQ

1. The board uses USB as the JTAG upload port. When printing serial port information on USB_CDC_ON_BOOT configuration needs to be turned on.
   If the port cannot be found when uploading the program or the USB has been used for other functions, the port does not appear.
   Please enter the upload mode manually.

   1. Connect the board via the USB cable
   2. Press and hold the **BOOT** button , While still pressing the **BOOT** button
   3. Release the **RST** button
   4. Release the **BOOT** button
   5. The USB port should be fixed and will not flash again. You can click Upload.
   6. Press the **RST** button to exit download mode

2. If the above is invalid, burn the [binary file](./firmware/README.MD) to check whether the hardware is normal
3. **1.47 inches AMOLED(T-Display-AMOLED-Lite)** does not support hardware screen rotation function
4. If you use external power supply instead of USBC, please turn off the CDC option. This is because the board will wait for USB access when it starts.

   - For Arduino IDE users, it can be turned off in the options , Please note that turning off USB CDC will turn off Serial redirection to USBC. At this time, you will not see any Serial message output when opening the port from USBC, but output from GPIO43 and GPIO44.

   ```c
   Tools -> USB CDC On Boot -> Disable
   ```

   - For Platformio users, you can add the following compilation flags in the ini file

   ```c
   build_flags =
       ; Enable -DARDUINO_USB_CDC_ON_BOOT will start printing and wait for terminal access during startup
       ; -DARDUINO_USB_CDC_ON_BOOT=1

       ; Enable -UARDUINO_USB_CDC_ON_BOOT will turn off printing and will not block when using the battery
       -UARDUINO_USB_CDC_ON_BOOT
   ```
5. How to find the location of your own libraries, [please see here](https://support.arduino.cc/hc/en-us/articles/4415103213714-Find-sketches-libraries-board-cores-and-other-files-on-your-computer)
     - Windows: `C:\Users\{username}\Documents\Arduino`
     - macOS: `/Users/{username}/Documents/Arduino`
     - Linux: `/home/{username}/Arduino`