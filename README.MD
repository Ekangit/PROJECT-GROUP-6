# PA1484 – Software Development:  Project
## Introduction
This project is a weather app for a LilyGO ESP32-s3. 

It solves the problem of not knowing what kind of weather the upcoming week or day will have as well as what kind of weather has already happened. 

The weather app uses the touchscreen on the microcontroller to navigate through the various screens. The app uses SMHI's own API to fetch the accurate information to display. It also uses PlatformIO to simplify the complexity of microcontrollers as well as making the hardware ready for compiling and uploading.

The main functionality is an weather app that displays forecasts for the upcoming seven days and historical weather data in a graph with data taken from SMHI's API. It also has a settings screen to change parameters and cities for the forecast and historical graph.


## Getting started
### Prerequisites
You need a LilyGO ESP32-s3 (might work for a ESP32 but not tested) and a USB-C to whatever-the-computer-needs cable. You also need C++ and platformIO extentions for VSC.


### Installation
Go to extentions and search for C++ and download, do the same for PlatformIO. When downloaded, open the repositorys folder and start to code in the project.io file (you can use other 
files aswell in the same folder).


## Building and running
You build the project by clicking on the PlatformIO icon to the most left part of the screen which looks like an alien, then depending on options you have, click on Build if you only have one option, if multiple click Build all and the project will try to build and the console will show its progress and the results.

You upload the project to the microcontroller in a similiar way, you go to the platformIO icon and click upload/upload all but before the the upload has been built, you must press RESET and BOOT on the microcontroller then let go of BOOT and then RESET every time you upload.

After the program has been succesfully uploaded indicated by the console, either remove the USB-C cable and then plug it back in or press the reset button and release. This should make the microcontroller reset with the new program and show it if it has anything to show. If using the correct library and functions, you can use the touchscreen to interact with the screen depending on what the code does. Our code uses touch screen in the form of sliding to next or previous screen as well as pressing on the screen on buttons or dropdown menus. It also has a slider for the historical graph which when pressed and then slided will slide with the finger.


## Features
Lastly, write which of the user stories you did and didn't develop in this project,  in the form of a checklist. Like this:

- [x] US1.1C: As a user, I want to see a starting screen to display the current program version and group number on the first screen.
- [x] US1.3: As a user, I want to have a screen to view weather forecast data.
- [x] US1.2C: As a user, I want to see the weather forecast for the next 7 days for the selected city on the second screen in terms of temperature and weather conditions with symbols (eg., clear sky, rain, snow, thunder) per day at 12:00.

- [x] US2.1: As a user, I want to be able to navigate between different screens (like forecast screen) by sliding a finger over the touch screen.

- [x] US3.1: As a user, I want to have a screen to view historical weather data.
- [x] US3.2D: As a user, on the third screen I want to view the latest months (SMHI API period: latest-months) of historical hourly data for selected weather parameter in the selected city, using a slider to interact with the historical graph by scrolling where a depleted slider corresponds to the oldest datapoint and a full slider corresponds to the latest datapoint.

- [x] US4.1: As a user, on the fourth screen, I want to access a single settings screen to configure both the city and weather parameter options.
- [x] US4.2B: As a user, I want to select from four weather parameters, namely temperature (1), humidity (6), wind speed (4), and Air pressure (9), using a dropdown list, to customize the historical graph.
- [x] US4.3B: As a user, I want to select from five different cities, namely Karlskrona(65090), Stockholm(97400), Göteborg(72420), Malmö(53300), and Kiruna(180940), using a dropdown, to view their weather data for the historical data and starting screen forecast.
- [x] US4.4: As a user, I want to reset the selected city and weather parameter to default using a button.
- [x] US4.5: As a user, I want to set my default city and weather parameter to the current selection using a button, so they are automatically selected when I start the device.
- [x] US4.6: As a user, I want the device to store my default city and weather parameter so that they are retained even after a restart.

- [ ] US5.1: As a user, I want to access a screen to view the weather forecast for all of
Sweden.
- [ ] US5.2: As a user, I want to see the temperature forecast for each administrative area of Sweden (“Landskap”) on a map, with: A Color-coded temperature zone per each administrative area, using the center of each area to access the forecast. A looped animation displaying the hourly forecast for the next 24 hours.

- [x] NF1: The code must follow a suitable coding standard.
- [x] NF2: The code must be well-documented, including: Inline comments within the code. Markdown documentation in the project repository.
- [x] NF3: The project must follow the terms and conditions for data (license compliance) with the data source.
