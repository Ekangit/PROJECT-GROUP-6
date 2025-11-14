/**
 * @file project.ino
 * @brief Weather Data Display System for ESP32 T-Display-AMOLED
 * @version 2.0
 * @date 2025-11-14
 * @author GROUP-6
 * 
 * @description
 * This application fetches weather data from SMHI's API and displays it on an 
 * ESP32 T-Display-AMOLED device. Complete feature set includes:
 * - Boot screen with version and group info (US1.1)
 * - 7-day weather forecast display (US1.2)
 * - City selection from dropdown menu (US1.3)
 * - Touch-based swipe navigation (US2.1)
 * - Button-based navigation (US2.2)
 * - Historical weather data visualization (US3.1)
 * - Historical temperature line graph (US3.2)
 * - Settings/configuration screen (US4.x)
 * - Enhanced weather parameters (wind, humidity, pressure)
 * 
 * @dependencies
 * - Arduino.h: Core Arduino framework
 * - WiFi.h: WiFi connectivity
 * - HTTPClient.h: HTTP requests to SMHI API
 * - ArduinoJson.h: JSON parsing
 * - LilyGo_AMOLED.h: Display driver
 * - LV_Helper.h: LVGL helper functions
 * - lvgl.h: GUI library
 * 
 * @note Delete WiFi credentials before committing to GitHub
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <time.h>
#include <LilyGo_AMOLED.h>
#include <LV_Helper.h>
#include <lvgl.h>

// ============================================================================
// CONFIGURATION CONSTANTS
// ============================================================================

/// Wi-Fi SSID (Delete before committing to GitHub)
static const char* WIFI_SSID     = "BTH_Guest";

/// Wi-Fi Password (Delete before committing to GitHub)
static const char* WIFI_PASSWORD = "nektarin87rosa";

/// Application version number
static const char* APP_VERSION = "2.0";

/// Project group identifier
static const char* GROUP_NUMBER = "GROUP-6";

/// Boot screen display duration in milliseconds
static const uint32_t BOOT_SCREEN_DURATION = 3000;

// ============================================================================
// CITY CONFIGURATION (US1.3)
// ============================================================================

/**
 * @struct CityConfig
 * @brief Stores city information and API coordinates
 */
struct CityConfig {
    const char* name;           ///< City name
    float latitude;             ///< Latitude coordinate
    float longitude;            ///< Longitude coordinate
    int weatherStation;         ///< SMHI weather station ID
};

/// Available cities for selection (US1.3)
static const CityConfig CITIES[] = {
    {"Stockholm",  59.3293,  18.0686, 98210},  // Stockholm A
    {"Göteborg",   57.7089,  11.9746, 71420},  // Göteborg A
    {"Malmö",      55.6050,  13.0038, 53430},  // Malmö A
    {"Uppsala",    59.8586,  17.6389, 97530},  // Uppsala Aut
    {"Linköping",  58.4108,  15.6214, 86340},  // Malmslätt
    {"Karlskrona", 56.1621,  15.5866, 65090}   // Karlskrona
};

/// Number of available cities
static const int CITY_COUNT = sizeof(CITIES) / sizeof(CityConfig);

/// Currently selected city index
static int currentCityIndex = 1;  // Default: Göteborg

// ============================================================================
// API ENDPOINTS
// ============================================================================

/// SMHI API endpoint for weather forecast (dynamically updated based on city)
static String SMHI_FORECAST_API = "";

/// SMHI API endpoint for historical data (dynamically updated based on city)
static String SMHI_HISTORY_API = "";

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

/// LilyGo AMOLED display object
LilyGo_Class amoled;

/// Main tileview container for navigation
static lv_obj_t* tileview;

/// Tile 0: Settings/City selection screen
static lv_obj_t* t0;

/// Tile 1: Forecast screen
static lv_obj_t* t1;

/// Tile 2: Historical weather data screen
static lv_obj_t* t2;

/// Tile 3: Historical temperature graph screen
static lv_obj_t* t3;

/// Dropdown for city selection (US1.3)
static lv_obj_t* city_dropdown;

/// Label for settings screen
static lv_obj_t* t0_label;

/// Label for tile 1
static lv_obj_t* t1_label;

/// Label for tile 2
static lv_obj_t* t2_label;

/// Chart object for temperature graph
static lv_obj_t* temperature_chart;

/// Navigation buttons (US2.2)
static lv_obj_t* btn_next;
static lv_obj_t* btn_prev;
static lv_obj_t* btn_refresh;

// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * @struct WeatherData
 * @brief Stores weather forecast data for a single time point
 */
struct WeatherData {
    String dateTime;        ///< Date and time of forecast (ISO 8601 format)
    float temperature;      ///< Temperature in Celsius
    float precipitation;    ///< Precipitation in mm/h
    int weatherSymbol;      ///< SMHI weather symbol code
    float windSpeed;        ///< Wind speed in m/s
    int windDirection;      ///< Wind direction in degrees
    int humidity;           ///< Relative humidity in %
    float pressure;         ///< Air pressure in hPa
};

/**
 * @struct HistoricalData
 * @brief Stores historical temperature measurement
 */
struct HistoricalData {
    String dateTime;        ///< Date and time of measurement
    float temperature;      ///< Measured temperature in Celsius
};

/// Array to store 7 days of forecast data (at 12:00 each day)
static WeatherData forecast[7];

/// Array to store historical temperature data (last 30 days)
static HistoricalData history[30];

/// Number of valid forecast entries
static int forecastCount = 0;

/// Number of valid historical entries
static int historyCount = 0;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * @brief Updates API URLs based on selected city (US1.3)
 * @param cityIndex Index of city in CITIES array
 */
void updateAPIUrls(int cityIndex) {
    if (cityIndex < 0 || cityIndex >= CITY_COUNT) {
        cityIndex = 0;  // Default to first city
    }
    
    const CityConfig& city = CITIES[cityIndex];
    
    // Build forecast API URL
    SMHI_FORECAST_API = "https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/";
    SMHI_FORECAST_API += String(city.longitude, 4);
    SMHI_FORECAST_API += "/lat/";
    SMHI_FORECAST_API += String(city.latitude, 4);
    SMHI_FORECAST_API += "/data.json";
    
    // Build historical API URL
    SMHI_HISTORY_API = "https://opendata-download-metobs.smhi.se/api/version/1.0/parameter/2/station/";
    SMHI_HISTORY_API += String(city.weatherStation);
    SMHI_HISTORY_API += "/period/latest-months.json";
    
    Serial.printf("APIs updated for city: %s\n", city.name);
    Serial.printf("Forecast API: %s\n", SMHI_FORECAST_API.c_str());
    Serial.printf("History API: %s\n", SMHI_HISTORY_API.c_str());
}

/**
 * @brief Gets the name of currently selected city
 * @return City name as string
 */
String getCurrentCityName() {
    if (currentCityIndex >= 0 && currentCityIndex < CITY_COUNT) {
        return String(CITIES[currentCityIndex].name);
    }
    return "Unknown";
}

/**
 * @brief Extracts the date from an ISO 8601 datetime string
 * @param dateTime Full datetime string (e.g., "2025-11-05T12:00:00Z")
 * @return Date portion as string (e.g., "2025-11-05")
 */
String extractDate(const String& dateTime) {
    int tPos = dateTime.indexOf('T');
    if (tPos > 0) {
        return dateTime.substring(0, tPos);
    }
    return dateTime;
}

/**
 * @brief Extracts the time from an ISO 8601 datetime string
 * @param dateTime Full datetime string (e.g., "2025-11-05T12:00:00Z")
 * @return Time portion as string (e.g., "12:00")
 */
String extractTime(const String& dateTime) {
    int tPos = dateTime.indexOf('T');
    if (tPos > 0) {
        String time = dateTime.substring(tPos + 1);
        int colonPos = time.indexOf(':');
        if (colonPos > 0) {
            return time.substring(0, colonPos + 3); // HH:MM
        }
    }
    return "00:00";
}

/**
 * @brief Gets weather description based on SMHI weather symbol code
 * @param symbol SMHI weather symbol code (1-27)
 * @return Human-readable weather description
 * @note SMHI weather symbols: 1-Clear, 2-Nearly clear, 3-Variable, etc.
 */
String getWeatherDescription(int symbol) {
    switch (symbol) {
        case 1: return "Clear sky";
        case 2: return "Nearly clear";
        case 3: return "Variable";
        case 4: return "Halfclear";
        case 5: return "Cloudy";
        case 6: return "Overcast";
        case 7: return "Fog";
        case 8: return "Light rain";
        case 9: return "Moderate rain";
        case 10: return "Heavy rain";
        case 11: return "Thunderstorm";
        case 12: return "Light sleet";
        case 13: return "Moderate sleet";
        case 14: return "Heavy sleet";
        case 15: return "Light snow";
        case 16: return "Moderate snow";
        case 17: return "Heavy snow";
        case 18: return "Light rain shower";
        case 19: return "Moderate rain shower";
        case 20: return "Heavy rain shower";
        case 21: return "Thunder shower";
        case 22: return "Light sleet shower";
        case 23: return "Moderate sleet shower";
        case 24: return "Heavy sleet shower";
        case 25: return "Light snow shower";
        case 26: return "Moderate snow shower";
        case 27: return "Heavy snow shower";
        default: return "Unknown";
    }
}

// ============================================================================
// API FUNCTIONS
// ============================================================================

/**
 * @brief Fetches weather forecast data from SMHI API
 * @details Makes HTTP GET request to SMHI's forecast API and parses JSON response.
 *          Extracts forecasts for 12:00 each day for the next 7 days.
 * @return true if data was fetched successfully, false otherwise
 * @note Populates the global forecast[] array and sets forecastCount
 */
bool fetchWeatherForecast() {
    Serial.println("\n========== FETCH FORECAST DEBUG ==========");
    
    // Check 1: WiFi Status
    Serial.printf("Check 1: WiFi Status = %d (3=Connected)\n", WiFi.status());
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(">>> FAIL POINT 1: WiFi not connected!");
        return false;
    }
    Serial.println("✓ WiFi is connected");

    // Check 2: API URL
    Serial.printf("Check 2: API URL length = %d\n", SMHI_FORECAST_API.length());
    Serial.printf("API URL: %s\n", SMHI_FORECAST_API.c_str());
    if (SMHI_FORECAST_API.length() == 0) {
        Serial.println(">>> FAIL POINT 2: API URL not initialized!");
        return false;
    }
    Serial.println("✓ API URL is set");
    
    // Check 3: Create secure client
    Serial.println("Check 3: Creating WiFiClientSecure...");
    WiFiClientSecure client;
    client.setInsecure();  // Skip certificate validation for SMHI
    Serial.println("✓ Client created and set to insecure mode");
    
    // Check 4: Create HTTP client
    Serial.println("Check 4: Creating HTTPClient...");
    HTTPClient http;
    http.setTimeout(15000);  // 15 second timeout
    Serial.println("✓ HTTPClient created with 15s timeout");
    
    // Check 5: Begin HTTP connection
    Serial.println("Check 5: Starting HTTP connection...");
    bool beginResult = http.begin(client, SMHI_FORECAST_API.c_str());
    Serial.printf("http.begin() returned: %d (1=success)\n", beginResult);
    if (!beginResult) {
        Serial.println(">>> FAIL POINT 3: http.begin() returned false!");
        Serial.println("This usually means invalid URL format");
        return false;
    }
    Serial.println("✓ HTTP connection started");
    
    // Check 6: Send GET request
    Serial.println("Check 6: Sending GET request...");
    Serial.println("(This may take 5-15 seconds...)");
    int httpCode = http.GET();
    
    Serial.printf(">>> HTTP Response Code: %d\n", httpCode);
    Serial.println("Common codes: 200=OK, 404=NotFound, -1=ConnectionFailed, -11=Timeout");
    
    // Check 7: Handle response
    if (httpCode != HTTP_CODE_OK && httpCode > 0) {
        Serial.printf(">>> FAIL POINT 4: Bad HTTP code %d: %s\n", httpCode, http.errorToString(httpCode).c_str());
        http.end();
        return false;
    } else if (httpCode <= 0) {
        Serial.printf(">>> FAIL POINT 5: Connection error %d: %s\n", httpCode, http.errorToString(httpCode).c_str());
        Serial.println("HINT: Negative codes mean network issues");
        Serial.println("  -1 = Connection failed");
        Serial.println("  -11 = Read timeout (server not responding)");
        Serial.println("HINT: Check if school firewall blocks HTTPS to opendata-download-metfcst.smhi.se");
        Serial.println("HINT: Try pinging the SMHI server from another device on same network");
        http.end();
        return false;
    }

    Serial.println("✓ HTTP 200 OK - Request successful!");
    
    // Check 8: Get payload
    Serial.println("Check 8: Reading response payload...");
    String payload = http.getString();
    Serial.printf("✓ Payload received: %d bytes\n", payload.length());
    
    if (payload.length() == 0) {
        Serial.println(">>> FAIL POINT 6: Empty payload!");
        http.end();
        return false;
    }
    
    http.end();

    // Check 9: Parse JSON
    Serial.println("Check 9: Allocating JSON buffer (64KB)...");
    DynamicJsonDocument doc(65536);  // 64KB buffer for large forecast
    
    Serial.println("Check 10: Parsing JSON...");
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        Serial.printf(">>> FAIL POINT 7: JSON parsing failed: %s\n", error.c_str());
        
        // Provide helpful hints
        if (error == DeserializationError::NoMemory) {
            Serial.println("HINT: Not enough memory to parse JSON. Payload might be too large.");
        }
        Serial.println("First 500 chars of payload:");
        Serial.println(payload.substring(0, 500));
        return false;
    }
    
    Serial.println("✓ JSON parsed successfully");

    // Check 11: Extract forecast data
    Serial.println("Check 11: Extracting time series...");
    forecastCount = 0;
    JsonArray timeSeries = doc["timeSeries"].as<JsonArray>();
    
    if (timeSeries.isNull()) {
        Serial.println(">>> FAIL POINT 8: No 'timeSeries' array in JSON!");
        Serial.println("JSON structure might be different than expected");
        return false;
    }
    
    Serial.printf("✓ Found %d time series entries\n", timeSeries.size());
    
    if (timeSeries.size() == 0) {
        Serial.println(">>> FAIL POINT 9: timeSeries array is empty!");
        return false;
    }
    
    // Check 12: Extract 12:00 forecasts
    Serial.println("Check 12: Extracting 12:00 noon forecasts...");
    for (JsonObject timePoint : timeSeries) {
        if (forecastCount >= 7) break;
        
        String validTime = timePoint["validTime"].as<String>();
        
        // Check if time is 12:00
        if (validTime.indexOf("T12:00:00Z") > 0) {
            forecast[forecastCount].dateTime = validTime;
            
            // Extract parameters - including wind, humidity, pressure
            JsonArray parameters = timePoint["parameters"].as<JsonArray>();
            for (JsonObject param : parameters) {
                String name = param["name"].as<String>();
                
                if (name == "t") {  // Temperature
                    forecast[forecastCount].temperature = param["values"][0].as<float>();
                } else if (name == "pcat") {  // Precipitation category
                    forecast[forecastCount].precipitation = param["values"][0].as<float>();
                } else if (name == "Wsymb2") {  // Weather symbol
                    forecast[forecastCount].weatherSymbol = param["values"][0].as<int>();
                } else if (name == "ws") {  // Wind speed
                    forecast[forecastCount].windSpeed = param["values"][0].as<float>();
                } else if (name == "wd") {  // Wind direction
                    forecast[forecastCount].windDirection = param["values"][0].as<int>();
                } else if (name == "r") {  // Relative humidity
                    forecast[forecastCount].humidity = param["values"][0].as<int>();
                } else if (name == "msl") {  // Air pressure (mean sea level)
                    forecast[forecastCount].pressure = param["values"][0].as<float>();
                }
            }
            
            Serial.printf("  Day %d: %s, %.1f°C, Wind: %.1f m/s, Humidity: %d%%\n", 
                         forecastCount + 1, 
                         extractDate(validTime).c_str(), 
                         forecast[forecastCount].temperature,
                         forecast[forecastCount].windSpeed,
                         forecast[forecastCount].humidity);
            forecastCount++;
        }
    }
    
    Serial.printf("✓ Successfully extracted %d forecast entries\n", forecastCount);
    
    if (forecastCount == 0) {
        Serial.println(">>> FAIL POINT 10: No 12:00 forecasts found in data!");
        Serial.println("HINT: SMHI might have changed their data format");
        return false;
    }
    
    Serial.println("========== FETCH FORECAST SUCCESS ==========\n");
    return true;
}

/**
 * @brief Fetches historical temperature data from SMHI API
 * @details Makes HTTP GET request to SMHI's observation API and parses JSON response.
 *          Extracts temperature measurements for the last months.
 * @return true if data was fetched successfully, false otherwise
 * @note Populates the global history[] array and sets historyCount
 */
bool fetchHistoricalData() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("ERROR: WiFi not connected!");
        return false;
    }

    // Check if API URL is set
    if (SMHI_HISTORY_API.length() == 0) {
        Serial.println("ERROR: Historical API URL not initialized!");
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();  // Skip certificate validation for SMHI
    
    HTTPClient http;
    http.setTimeout(15000);  // 15 second timeout
    
    Serial.println("Fetching historical data from SMHI...");
    Serial.printf("API URL: %s\n", SMHI_HISTORY_API.c_str());
    Serial.println("Sending HTTP GET request...");
    
    // Use WiFiClientSecure for HTTPS
    bool beginResult = http.begin(client, SMHI_HISTORY_API.c_str());
    if (!beginResult) {
        Serial.println("ERROR: Failed to begin HTTP connection!");
        return false;
    }
    
    int httpCode = http.GET();
    
    Serial.printf("HTTP Response Code: %d\n", httpCode);
    
    if (httpCode != HTTP_CODE_OK && httpCode > 0) {
        Serial.printf("HTTP request failed, error code %d: %s\n", httpCode, http.errorToString(httpCode).c_str());
        http.end();
        return false;
    } else if (httpCode <= 0) {
        Serial.printf("Connection error: %s\n", http.errorToString(httpCode).c_str());
        Serial.println("HINT: Check firewall settings.");
        http.end();
        return false;
    }

    Serial.println("HTTP request successful, parsing JSON...");
    String payload = http.getString();
    Serial.printf("Payload size: %d bytes\n", payload.length());
    http.end();

    // Parse JSON - allocate enough memory for historical data
    // Historical data is smaller than forecast, typically 10-20KB
    DynamicJsonDocument doc(32768);  // 32KB buffer for historical data
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        Serial.print("ERROR: JSON parsing failed: ");
        Serial.println(error.c_str());
        Serial.printf("Error code: %s\n", error.c_str());
        
        // Provide helpful hints
        if (error == DeserializationError::NoMemory) {
            Serial.println("HINT: Not enough memory to parse JSON. Payload might be too large.");
        }
        return false;
    }
    
    Serial.println("JSON parsed successfully");

    // Extract temperature data (limit to 30 entries)
    historyCount = 0;
    JsonArray values = doc["value"].as<JsonArray>();
    
    Serial.printf("Found %d historical entries\n", values.size());
    
    // Get last 30 entries
    int startIndex = max(0, (int)values.size() - 30);
    int index = 0;
    
    for (JsonObject entry : values) {
        if (index >= startIndex && historyCount < 30) {
            history[historyCount].dateTime = entry["date"].as<String>();
            history[historyCount].temperature = entry["value"].as<float>();
            
            if (historyCount < 3) {  // Show first 3 for debugging
                Serial.printf("  Entry %d: %s, %.1f°C\n", historyCount + 1, 
                             extractDate(history[historyCount].dateTime).c_str(), 
                             history[historyCount].temperature);
            }
            historyCount++;
        }
        index++;
    }
    
    Serial.printf("Successfully fetched %d historical entries\n", historyCount);
    return historyCount > 0;
}

// ============================================================================
// UI FUNCTIONS
// ============================================================================

/**
 * @brief Displays the boot screen with version and group information
 * @details Shows boot screen for BOOT_SCREEN_DURATION milliseconds (US1.1)
 * @note This screen displays the program version and group number
 */
void showBootScreen() {
    lv_obj_t* boot_label = lv_label_create(lv_scr_act());
    
    String bootText = String("Weather System v") + APP_VERSION + 
                     String("\n") + GROUP_NUMBER + 
                     String("\n\nLoading...");
    
    lv_label_set_text(boot_label, bootText.c_str());
    lv_obj_set_style_text_font(boot_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_align(boot_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(boot_label);
    
    lv_timer_handler();
    delay(BOOT_SCREEN_DURATION);
    
    lv_obj_del(boot_label);
}

/**
 * @brief Updates the forecast screen with weather data
 * @details Displays 7-day forecast with enhanced weather parameters (US1.2)
 * @note Shows date, temperature, weather condition, wind, humidity, pressure
 */
void updateForecastScreen() {
    if (t1_label == NULL) return;
    
    String forecastText = "7-Day Forecast - " + getCurrentCityName() + "\n";
    forecastText += "================================\n\n";
    
    if (forecastCount == 0) {
        forecastText += "No forecast data available.\n";
        forecastText += "Check WiFi connection.";
    } else {
        for (int i = 0; i < forecastCount; i++) {
            String date = extractDate(forecast[i].dateTime);
            
            forecastText += date + " 12:00\n";
            forecastText += String(forecast[i].temperature, 1) + "°C - ";
            forecastText += getWeatherDescription(forecast[i].weatherSymbol) + "\n";
            forecastText += "Wind: " + String(forecast[i].windSpeed, 1) + " m/s\n";
            forecastText += "Humidity: " + String(forecast[i].humidity) + "%\n";
            forecastText += "Pressure: " + String(forecast[i].pressure, 0) + " hPa\n";
            
            if (i < forecastCount - 1) {
                forecastText += "--------------------------------\n";
            }
        }
    }
    
    lv_label_set_text(t1_label, forecastText.c_str());
}

/**
 * @brief Updates the historical data screen
 * @details Displays historical weather data (US3.1)
 * @note Shows date, time, and temperature for recent measurements
 */
void updateHistoricalScreen() {
    if (t2_label == NULL) return;
    
    String historyText = "Historical Weather Data\n";
    historyText += getCurrentCityName() + "\n";
    historyText += "================================\n\n";
    
    if (historyCount == 0) {
        historyText += "No historical data available.\n";
        historyText += "Check WiFi connection.";
    } else {
        // Show last 10 entries to fit on screen
        int startIdx = max(0, historyCount - 10);
        
        for (int i = startIdx; i < historyCount; i++) {
            String date = extractDate(history[i].dateTime);
            String time = extractTime(history[i].dateTime);
            
            historyText += date + " " + time + "\n";
            historyText += "Temperature: " + String(history[i].temperature, 1) + " C\n";
            
            if (i < historyCount - 1) {
                historyText += "--------------------------------\n";
            }
        }
    }
    
    lv_label_set_text(t2_label, historyText.c_str());
}

/**
 * @brief Creates and updates the temperature line graph
 * @details Displays historical temperature data as a line graph (US3.2)
 * @note Shows temperature trends over time
 */
void updateTemperatureChart() {
    if (temperature_chart == NULL) return;
    
    lv_chart_series_t* series = lv_chart_get_series_next(temperature_chart, NULL);
    if (series == NULL) return;
    
    // Clear existing data
    lv_chart_set_point_count(temperature_chart, historyCount);
    
    // Add temperature data points
    for (int i = 0; i < historyCount; i++) {
        lv_chart_set_next_value(temperature_chart, series, (int32_t)history[i].temperature);
    }
    
    lv_chart_refresh(temperature_chart);
}

/**
 * @brief Event handler for city dropdown selection (US1.3)
 */
static void city_dropdown_event_cb(lv_event_t* e) {
    lv_obj_t* dropdown = lv_event_get_target(e);
    uint16_t option = lv_dropdown_get_selected(dropdown);
    
    if (option != currentCityIndex) {
        currentCityIndex = option;
        Serial.printf("City changed to: %s\n", CITIES[currentCityIndex].name);
        
        // Update API URLs
        updateAPIUrls(currentCityIndex);
        
        // Refresh data
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Fetching new data for selected city...");
            if (fetchWeatherForecast()) {
                updateForecastScreen();
                lv_timer_handler();
            }
            if (fetchHistoricalData()) {
                updateHistoricalScreen();
                updateTemperatureChart();
                lv_timer_handler();
            }
        }
    }
}

/**
 * @brief Event handler for navigation buttons (US2.2)
 */
static void btn_event_cb(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target(e);
    
    if (btn == btn_next) {
        // Move to next tile - use lv_obj_scroll_to for LVGL 8.x
        lv_obj_scroll_by(tileview, -lv_disp_get_hor_res(NULL), 0, LV_ANIM_ON);
    } else if (btn == btn_prev) {
        // Move to previous tile
        lv_obj_scroll_by(tileview, lv_disp_get_hor_res(NULL), 0, LV_ANIM_ON);
    } else if (btn == btn_refresh) {
        // Refresh data
        Serial.println("Refresh button pressed");
        if (WiFi.status() == WL_CONNECTED) {
            if (fetchWeatherForecast()) {
                updateForecastScreen();
            }
            if (fetchHistoricalData()) {
                updateHistoricalScreen();
                updateTemperatureChart();
            }
            lv_timer_handler();
        }
    }
}

/**
 * @brief Creates the main user interface with tiled navigation
 * @details Creates a tileview with four horizontal tiles (US2.1, US2.2):
 *          - Tile 0: Settings/City selection
 *          - Tile 1: Weather forecast
 *          - Tile 2: Historical data
 *          - Tile 3: Temperature graph
 * @note Tiles can be navigated by sliding finger or using buttons
 */
void create_ui() {
    // Create fullscreen tileview for navigation
    tileview = lv_tileview_create(lv_scr_act());
    lv_obj_set_size(tileview, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_OFF);

    // Add four horizontal tiles for navigation
    t0 = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_HOR);  // Settings
    t1 = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_HOR);  // Forecast
    t2 = lv_tileview_add_tile(tileview, 2, 0, LV_DIR_HOR);  // Historical
    t3 = lv_tileview_add_tile(tileview, 3, 0, LV_DIR_HOR);  // Graph

    // Tile 0: Settings/City Selection Screen (US1.3)
    {
        t0_label = lv_label_create(t0);
        lv_label_set_text(t0_label, "Settings");
        lv_obj_set_style_text_font(t0_label, &lv_font_montserrat_20, 0);
        lv_obj_align(t0_label, LV_ALIGN_TOP_MID, 0, 10);
        
        // City selection dropdown
        lv_obj_t* city_label = lv_label_create(t0);
        lv_label_set_text(city_label, "Select City:");
        lv_obj_set_style_text_font(city_label, &lv_font_montserrat_16, 0);
        lv_obj_align(city_label, LV_ALIGN_TOP_LEFT, 10, 50);
        
        city_dropdown = lv_dropdown_create(t0);
        
        // Build city list for dropdown
        String cityList = "";
        for (int i = 0; i < CITY_COUNT; i++) {
            if (i > 0) cityList += "\n";
            cityList += CITIES[i].name;
        }
        lv_dropdown_set_options(city_dropdown, cityList.c_str());
        lv_dropdown_set_selected(city_dropdown, currentCityIndex);
        lv_obj_align(city_dropdown, LV_ALIGN_TOP_LEFT, 10, 80);
        lv_obj_set_width(city_dropdown, 200);
        lv_obj_add_event_cb(city_dropdown, city_dropdown_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
        
        // Instructions
        lv_obj_t* info_label = lv_label_create(t0);
        lv_label_set_text(info_label, "\nSwipe right or press Next\nto view weather data.\n\nSwipe left/right between\nscreens.");
        lv_obj_set_style_text_font(info_label, &lv_font_montserrat_12, 0);
        lv_obj_align(info_label, LV_ALIGN_CENTER, 0, 40);
    }

    // Tile 1: Weather Forecast Screen (US1.2)
    {
        t1_label = lv_label_create(t1);
        lv_label_set_text(t1_label, "Loading forecast...");
        lv_obj_set_style_text_font(t1_label, &lv_font_montserrat_12, 0);
        lv_obj_align(t1_label, LV_ALIGN_TOP_LEFT, 10, 40);
        lv_label_set_long_mode(t1_label, LV_LABEL_LONG_SCROLL);
        lv_obj_set_width(t1_label, lv_disp_get_hor_res(NULL) - 20);
    }

    // Tile 2: Historical Weather Data Screen (US3.1)
    {
        t2_label = lv_label_create(t2);
        lv_label_set_text(t2_label, "Loading historical data...");
        lv_obj_set_style_text_font(t2_label, &lv_font_montserrat_12, 0);
        lv_obj_align(t2_label, LV_ALIGN_TOP_LEFT, 10, 40);
        lv_label_set_long_mode(t2_label, LV_LABEL_LONG_SCROLL);
        lv_obj_set_width(t2_label, lv_disp_get_hor_res(NULL) - 20);
    }

    // Tile 3: Temperature Graph Screen (US3.2)
    {
        // Create title label
        lv_obj_t* chart_title = lv_label_create(t3);
        lv_label_set_text(chart_title, "Temperature History");
        lv_obj_set_style_text_font(chart_title, &lv_font_montserrat_16, 0);
        lv_obj_align(chart_title, LV_ALIGN_TOP_MID, 0, 40);
        
        // Create chart
        temperature_chart = lv_chart_create(t3);
        lv_obj_set_size(temperature_chart, lv_disp_get_hor_res(NULL) - 20, 
                       lv_disp_get_ver_res(NULL) - 120);
        lv_obj_align(temperature_chart, LV_ALIGN_CENTER, 0, 20);
        
        lv_chart_set_type(temperature_chart, LV_CHART_TYPE_LINE);
        lv_chart_set_range(temperature_chart, LV_CHART_AXIS_PRIMARY_Y, -20, 40);
        lv_chart_set_point_count(temperature_chart, 30);
        
        // Add series
        lv_chart_series_t* series = lv_chart_add_series(temperature_chart, 
                                                        lv_palette_main(LV_PALETTE_RED),
                                                        LV_CHART_AXIS_PRIMARY_Y);
        
        // Configure chart appearance
        lv_chart_set_div_line_count(temperature_chart, 5, 10);
        lv_obj_set_style_size(temperature_chart, 3, LV_PART_INDICATOR);
    }
    
    // Create navigation buttons (US2.2) - visible on all screens
    {
        // Previous button
        btn_prev = lv_btn_create(lv_scr_act());
        lv_obj_set_size(btn_prev, 60, 30);
        lv_obj_align(btn_prev, LV_ALIGN_BOTTOM_LEFT, 10, -10);
        lv_obj_t* btn_prev_label = lv_label_create(btn_prev);
        lv_label_set_text(btn_prev_label, "< Prev");
        lv_obj_center(btn_prev_label);
        lv_obj_add_event_cb(btn_prev, btn_event_cb, LV_EVENT_CLICKED, NULL);
        
        // Next button
        btn_next = lv_btn_create(lv_scr_act());
        lv_obj_set_size(btn_next, 60, 30);
        lv_obj_align(btn_next, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
        lv_obj_t* btn_next_label = lv_label_create(btn_next);
        lv_label_set_text(btn_next_label, "Next >");
        lv_obj_center(btn_next_label);
        lv_obj_add_event_cb(btn_next, btn_event_cb, LV_EVENT_CLICKED, NULL);
        
        // Refresh button
        btn_refresh = lv_btn_create(lv_scr_act());
        lv_obj_set_size(btn_refresh, 70, 30);
        lv_obj_align(btn_refresh, LV_ALIGN_BOTTOM_MID, 0, -10);
        lv_obj_t* btn_refresh_label = lv_label_create(btn_refresh);
        lv_label_set_text(btn_refresh_label, "Refresh");
        lv_obj_center(btn_refresh_label);
        lv_obj_add_event_cb(btn_refresh, btn_event_cb, LV_EVENT_CLICKED, NULL);
    }
}

// ============================================================================
// NETWORK FUNCTIONS
// ============================================================================

/**
 * @brief Connects to WiFi network
 * @details Attempts to connect to the configured WiFi network with 15-second timeout
 * @note Required for fetching data from SMHI API
 * @note WiFi credentials must be configured in WIFI_SSID and WIFI_PASSWORD
 */
void connect_wifi() {
    Serial.printf("Connecting to WiFi SSID: %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    const uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected successfully!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("WiFi connection failed (timeout).");
    }
}

// ============================================================================
// MAIN ARDUINO FUNCTIONS
// ============================================================================

/**
 * @brief Setup function - runs once on startup
 * @details Initializes hardware, display, WiFi, and fetches initial data
 * 
 * Initialization sequence:
 * 1. Initialize serial communication
 * 2. Initialize AMOLED display
 * 3. Initialize LVGL graphics library
 * 4. Show boot screen (US1.1)
 * 5. Set default city and update API URLs (US1.3)
 * 6. Create UI with tiled navigation (US2.1, US2.2)
 * 7. Connect to WiFi
 * 8. Fetch weather forecast data (US1.2)
 * 9. Fetch historical temperature data (US3.1, US3.2)
 * 10. Update all screens with data
 */
void setup() {
    // Initialize serial communication for debugging
    Serial.begin(115200);
    
    // IMPORTANT: Wait for Serial Monitor to connect (especially on ESP32-S3 with USB CDC)
    // This gives you time to open the Serial Monitor before messages start
    delay(2000);  // 2 seconds - open Serial Monitor during this time!
    
    // Send some initial characters to "wake up" the serial connection
    Serial.println();
    Serial.println();
    Serial.println("==========================================");
    Serial.println("=== Weather System Starting ===");
    Serial.println("==========================================");
    Serial.printf("Version: %s\n", APP_VERSION);
    Serial.printf("Group: %s\n", GROUP_NUMBER);
    Serial.println();

    // Initialize AMOLED display
    if (!amoled.begin()) {
        Serial.println("ERROR: Failed to initialize LilyGO AMOLED display!");
        while (true) delay(1000);
    }
    Serial.println("Display initialized successfully");

    // Initialize LVGL graphics library
    beginLvglHelper(amoled);
    Serial.println("LVGL initialized successfully");

    // Show boot screen with version and group info (US1.1)
    showBootScreen();

    // Set default city and update API URLs (US1.3)
    updateAPIUrls(currentCityIndex);
    Serial.printf("Default city: %s\n", getCurrentCityName().c_str());

    // Create main UI with four tiles (US2.1, US2.2)
    create_ui();
    Serial.println("UI created successfully with city selection and navigation buttons");

    // Connect to WiFi
    connect_wifi();

    // Fetch and display weather data
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n=== Fetching Weather Data ===");
        
        // Fetch 7-day forecast (US1.2)
        if (fetchWeatherForecast()) {
            Serial.println("Forecast data fetched successfully");
            updateForecastScreen();
            lv_timer_handler();  // Refresh display to show forecast
            delay(10);
        } else {
            Serial.println("Failed to fetch forecast data");
            lv_label_set_text(t1_label, "ERROR: Failed to fetch forecast.\nCheck WiFi connection.");
            lv_timer_handler();
        }

        // Fetch historical data (US3.1, US3.2)
        if (fetchHistoricalData()) {
            Serial.println("Historical data fetched successfully");
            updateHistoricalScreen();
            updateTemperatureChart();
            lv_timer_handler();  // Refresh display to show historical data
            delay(10);
        } else {
            Serial.println("Failed to fetch historical data");
            lv_label_set_text(t2_label, "ERROR: Failed to fetch history.\nCheck WiFi connection.");
            lv_timer_handler();
        }
    } else {
        Serial.println("WARNING: Cannot fetch data - WiFi not connected");
        lv_label_set_text(t1_label, "ERROR: WiFi not connected!\n\nPlease check:\n- SSID and password\n- Network is 2.4GHz\n- Not using eduroam");
        lv_label_set_text(t2_label, "ERROR: WiFi not connected!");
        lv_timer_handler();  // Show error messages
    }

    Serial.println("\n=== System Ready ===");
    Serial.println("Navigate using:");
    Serial.println("  - Swipe left/right on touchscreen");
    Serial.println("  - Use Prev/Next/Refresh buttons at bottom");
    Serial.println("  - Select city from Settings screen");
}

/**
 * @brief Loop function - runs continuously after setup
 * @details Handles LVGL timer events for UI updates and touch input
 * 
 * This function must be called frequently to:
 * - Process touch input for tile navigation (US2.1)
 * - Update display animations
 * - Handle UI events
 * 
 * @note Small delay prevents excessive CPU usage
 */
void loop() {
    // Handle LVGL tasks (touch input, animations, etc.)
    lv_timer_handler();
    
    // Small delay to prevent excessive CPU usage
    delay(5);
}