/**
 * @file project.ino
 * @brief Weather Data Display System for ESP32 T-Display-AMOLED
 * @version 1.0
 * @date 2025-11-05
 * @author GROUP-6
 * 
 * @description
 * This application fetches weather data from SMHI's API and displays it on an 
 * ESP32 T-Display-AMOLED device. Features include:
 * - Boot screen with version and group info
 * - 7-day weather forecast for Karlskrona
 * - Historical weather data visualization
 * - Historical temperature line graph
 * - Touch-based navigation between screens
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
static const char* WIFI_SSID     = "SSID";

/// Wi-Fi Password (Delete before committing to GitHub)
static const char* WIFI_PASSWORD = "PWD";

/// Application version number
static const char* APP_VERSION = "1.0";

/// Project group identifier
static const char* GROUP_NUMBER = "GROUP-6";

/// Boot screen display duration in milliseconds
static const uint32_t BOOT_SCREEN_DURATION = 3000;

/// SMHI API endpoint for Karlskrona weather forecast
/// Coordinates: Latitude 56.1621, Longitude 15.5866
static const char* SMHI_FORECAST_API = 
    "https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/15.5866/lat/56.1621/data.json";

/// SMHI API endpoint for historical weather data (temperature)
/// Station 65090, Parameter 2 (quality controlled air temperature)
static const char* SMHI_HISTORY_API = 
    "https://opendata-download-metobs.smhi.se/api/version/1.0/parameter/2/station/65090/period/latest-months.json";

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

/// LilyGo AMOLED display object
LilyGo_Class amoled;

/// Main tileview container for navigation
static lv_obj_t* tileview;

/// Tile 1: Boot/Forecast screen
static lv_obj_t* t1;

/// Tile 2: Historical weather data screen
static lv_obj_t* t2;

/// Tile 3: Historical temperature graph screen
static lv_obj_t* t3;

/// Label for tile 1
static lv_obj_t* t1_label;

/// Label for tile 2
static lv_obj_t* t2_label;

/// Chart object for temperature graph
static lv_obj_t* temperature_chart;

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
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("ERROR: WiFi not connected!");
        return false;
    }

    HTTPClient http;
    http.begin(SMHI_FORECAST_API);
    
    Serial.println("Fetching weather forecast from SMHI...");
    Serial.println("Sending HTTP GET request...");
    int httpCode = http.GET();
    
    Serial.printf("HTTP Response Code: %d\n", httpCode);
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("HTTP request failed, error code %d: %s\n", httpCode, http.errorToString(httpCode).c_str());
        http.end();
        return false;
    }

    Serial.println("HTTP request successful, parsing JSON...");
    String payload = http.getString();
    Serial.printf("Payload size: %d bytes\n", payload.length());
    http.end();

    // Parse JSON - allocate enough memory for large SMHI response
    // SMHI forecast is typically 40-50KB, so we use dynamic allocation
    DynamicJsonDocument doc(65536);  // 64KB buffer for large forecast
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

    // Extract 7 days of 12:00 forecasts
    forecastCount = 0;
    JsonArray timeSeries = doc["timeSeries"].as<JsonArray>();
    
    Serial.printf("Found %d time series entries\n", timeSeries.size());
    
    for (JsonObject timePoint : timeSeries) {
        if (forecastCount >= 7) break;
        
        String validTime = timePoint["validTime"].as<String>();
        
        // Check if time is 12:00
        if (validTime.indexOf("T12:00:00Z") > 0) {
            forecast[forecastCount].dateTime = validTime;
            
            // Extract parameters
            JsonArray parameters = timePoint["parameters"].as<JsonArray>();
            for (JsonObject param : parameters) {
                String name = param["name"].as<String>();
                
                if (name == "t") {  // Temperature
                    forecast[forecastCount].temperature = param["values"][0].as<float>();
                } else if (name == "pcat") {  // Precipitation category
                    forecast[forecastCount].precipitation = param["values"][0].as<float>();
                } else if (name == "Wsymb2") {  // Weather symbol
                    forecast[forecastCount].weatherSymbol = param["values"][0].as<int>();
                }
            }
            
            Serial.printf("  Day %d: %s, %.1f°C\n", forecastCount + 1, 
                         extractDate(validTime).c_str(), 
                         forecast[forecastCount].temperature);
            forecastCount++;
        }
    }
    
    Serial.printf("Successfully fetched %d forecast entries\n", forecastCount);
    return forecastCount > 0;
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

    HTTPClient http;
    http.begin(SMHI_HISTORY_API);
    
    Serial.println("Fetching historical data from SMHI...");
    Serial.println("Sending HTTP GET request...");
    int httpCode = http.GET();
    
    Serial.printf("HTTP Response Code: %d\n", httpCode);
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("HTTP request failed, error code %d: %s\n", httpCode, http.errorToString(httpCode).c_str());
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
 * @details Displays 7-day forecast for Karlskrona at 12:00 (US1.2)
 * @note Shows date, temperature, weather condition, and precipitation
 */
void updateForecastScreen() {
    if (t1_label == NULL) return;
    
    String forecastText = "7-Day Forecast - Karlskrona\n";
    forecastText += "================================\n\n";
    
    if (forecastCount == 0) {
        forecastText += "No forecast data available.\n";
        forecastText += "Check WiFi connection.";
    } else {
        for (int i = 0; i < forecastCount; i++) {
            String date = extractDate(forecast[i].dateTime);
            
            forecastText += date + " 12:00\n";
            forecastText += String(forecast[i].temperature, 1) + " C - ";
            forecastText += getWeatherDescription(forecast[i].weatherSymbol) + "\n";
            
            if (i < forecastCount - 1) {
                forecastText += "--------------------------------\n";
            }
        }
    }
    
    lv_label_set_text(t1_label, forecastText.c_str());
}

/**
 * @brief Updates the historical data screen
 * @details Displays historical weather data for Karlskrona (US3.1)
 * @note Shows date, time, and temperature for recent measurements
 */
void updateHistoricalScreen() {
    if (t2_label == NULL) return;
    
    String historyText = "Historical Weather Data\n";
    historyText += "Karlskrona\n";
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
 * @note Shows temperature trends for Karlskrona over time
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
 * @brief Creates the main user interface with tiled navigation
 * @details Creates a tileview with three horizontal tiles (US2.1):
 *          - Tile 1: Weather forecast
 *          - Tile 2: Historical data
 *          - Tile 3: Temperature graph
 * @note Tiles can be navigated by sliding finger over touchscreen
 */
void create_ui() {
    // Create fullscreen tileview for navigation
    tileview = lv_tileview_create(lv_scr_act());
    lv_obj_set_size(tileview, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_OFF);

    // Add three horizontal tiles for navigation
    t1 = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_HOR);
    t2 = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_HOR);
    t3 = lv_tileview_add_tile(tileview, 2, 0, LV_DIR_HOR);

    // Tile 1: Weather Forecast Screen
    {
        t1_label = lv_label_create(t1);
        lv_label_set_text(t1_label, "Loading forecast...");
        lv_obj_set_style_text_font(t1_label, &lv_font_montserrat_14, 0);
        lv_obj_align(t1_label, LV_ALIGN_TOP_LEFT, 10, 10);
        lv_label_set_long_mode(t1_label, LV_LABEL_LONG_SCROLL);
        lv_obj_set_width(t1_label, lv_disp_get_hor_res(NULL) - 20);
    }

    // Tile 2: Historical Weather Data Screen
    {
        t2_label = lv_label_create(t2);
        lv_label_set_text(t2_label, "Loading historical data...");
        lv_obj_set_style_text_font(t2_label, &lv_font_montserrat_14, 0);
        lv_obj_align(t2_label, LV_ALIGN_TOP_LEFT, 10, 10);
        lv_label_set_long_mode(t2_label, LV_LABEL_LONG_SCROLL);
        lv_obj_set_width(t2_label, lv_disp_get_hor_res(NULL) - 20);
    }

    // Tile 3: Temperature Graph Screen
    {
        // Create title label
        lv_obj_t* chart_title = lv_label_create(t3);
        lv_label_set_text(chart_title, "Temperature History - Karlskrona");
        lv_obj_set_style_text_font(chart_title, &lv_font_montserrat_16, 0);
        lv_obj_align(chart_title, LV_ALIGN_TOP_MID, 0, 10);
        
        // Create chart
        temperature_chart = lv_chart_create(t3);
        lv_obj_set_size(temperature_chart, lv_disp_get_hor_res(NULL) - 20, 
                       lv_disp_get_ver_res(NULL) - 80);
        lv_obj_align(temperature_chart, LV_ALIGN_CENTER, 0, 10);
        
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
 * 5. Create UI with tiled navigation
 * 6. Connect to WiFi
 * 7. Fetch weather forecast data
 * 8. Fetch historical temperature data
 * 9. Update all screens with data
 */
void setup() {
    // Initialize serial communication for debugging
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=== Weather System Starting ===");
    Serial.printf("Version: %s\n", APP_VERSION);
    Serial.printf("Group: %s\n", GROUP_NUMBER);

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

    // Create main UI with three tiles
    create_ui();
    Serial.println("UI created successfully");

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
    Serial.println("Swipe left/right to navigate between screens");
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