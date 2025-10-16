#include "smhi_api.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Koordinator för Karlskrona
const char* SMHI_URL = "https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/15.586/lat/56.162/data.json";

void smhi_initWiFi(const char* ssid, const char* password){
    Serial.println("Ansluter till Wi-Fi...");
    WiFi.begin(ssid,password);
    while(WiFi.status() != WL_CONNECTED){
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n Wi-Fi Anslutet!");
    Serial.print("Ip Adress: ");
    Serial.println(WiFi.localIP());
}

bool smhi_fetchForecast(vector<ForecastData>& forecast){
    if(WiFi.status() != WL_CONNECTED){
        Serial.println("Ingen Wi-Fi Anslutning.");
        return false;
    }

    HTTPClient http;
    http.begin(SMHI_URL);
    int httpCode = http.GET();
}

