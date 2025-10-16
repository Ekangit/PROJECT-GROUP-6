#ifndef SMHI_API_H
#define SMHI_API_H

#include <Arduino.h>
#include <vector>

using namespace std;


struct ForecastData
{
    String datum;
    float temperatur;
    String symbol;
    String nederbord;
};


// Initiera Wi-Fi
void smhi_initWiFi(const char* ssid, const char* password);


// Hämta prognosdata för Karlskrona
bool smhi_fetchForecast(vector<ForecastData>& forecast);





#endif