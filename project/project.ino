
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LilyGo_AMOLED.h>
#include <LV_Helper.h>
#include <lvgl.h>
#include <Preferences.h>
#include <time.h>

// BYT TILL VÅR HEADER FÖR LVGL-IKONER
#include "weatherIcons.hpp"

// ================= CONFIG =================
static const char* WIFI_SSID     = "Oppmannas spaningsbil";
static const char* WIFI_PASSWORD = "Orientering4";
static const char* APP_VERSION   = "4.0";
static const char* GROUP_NAME    = "GROUP-6";

// ================= CITY CONFIG =================
struct CityConfig {
    const char* name;
    float lat;
    float lon;
    int stationId;
};
static const CityConfig CITIES[] = {
    {"Karlskrona", 56.1621, 15.5866, 65090},
    {"Stockholm",  59.3293, 18.0686, 97400},
    {"Goteborg",   57.7089, 11.9746, 72420},
    {"Malmo",      55.6050, 13.0038, 53300},
    {"Kiruna",     67.8558, 20.2253, 180940}
};

// ================= PARAMETER CONFIG =================
struct ParamConfig {
    const char* name;
    int id;
};
static const ParamConfig PARAMETERS[] = {
    {"Temperature", 1},
    {"Humidity",    6},
    {"Wind Speed",  4},
    {"Air Pressure",9}
};
static const char* PARAM_NAMES_SV[] = {
    "Temperatur", "Luftfuktighet", "Vindhastighet", "Lufttryck"
};
static const char* PARAM_UNITS[] = {
    "°C", "%", "m/s", "hPa"
};

// ================= GLOBALS =================
LilyGo_Class amoled;
Preferences prefs;

static lv_obj_t* tileview;
static lv_obj_t *t0, *t1, *t2, *t3;

// Forecast screen
static lv_obj_t *forecastLabel, *forecastStatusLabel, *forecastCityLabel;
// NY: container som håller raderna (ikon + datum + temp)
static lv_obj_t *forecastList = nullptr;

// Historical screen
static lv_obj_t *historyCityLabel, *historyStatusLabel, *pointInfoLabel, *slider, *historyChart;
static lv_chart_series_t *historySeries, *markerSeries;

// Axis labels (Historikskärmen)
static lv_obj_t* xAxisLabel;
static lv_obj_t* yAxisLabel;

// Settings screen
static lv_obj_t *cityDropdown, *paramDropdown;

// Current / default selections
static int currentCity  = 0;
static int currentParam = 0;
static int defaultCity  = 0;
static int defaultParam = 0;

// API URLs
static String forecastAPI;
static String historyAPI;

// Data storage
struct ForecastData {
    String date;
    float  temp;
    int    weatherSymbol; // SMHI Wsymb2
    String icon;          // emoji (gamla funktionen)
};
static ForecastData forecast[7];
static int forecastCount = 0;

struct HistoricalData {
    String date;
    float  value;
};
static HistoricalData history[720];
static int historyCount = 0;

static int chartPointCount = 0;
static int lastStart = 0;

// ================= WEATHER ICON MAPPING  =================
String getWeatherIcon(int symbolId) {
    switch (symbolId) {
        case 1:  return "☀️";
        case 2:  return "🌤";
        case 3:  return "⛅";
        case 4:  return "☁️";
        case 5:  return "☁️";
        case 6:  return "☁️";
        case 7:  return "🌫";
        case 8:  return "🌦";
        case 9:  return "🌦";
        case 10: return "🌧";
        case 11: return "⛈";
        case 12: return "❄️";
        case 13: return "❄️";
        case 14: return "❄️";
        case 15: return "🌨";
        case 16: return "🌨";
        case 17: return "🌨";
        case 18: return "🌧";
        case 19: return "🌧";
        case 20: return "🌧";
        case 21: return "🌨";
        case 22: return "🌨";
        case 23: return "🌨";
        case 24: return "🌩";
        case 25: return "🌧";
        case 26: return "🌧";
        case 27: return "🌧";
        default: return "❔";
    }
}

// ================= API FUNCTIONS =================
void updateAPIUrls() {
    const CityConfig& city = CITIES[currentCity];

    forecastAPI = String("http://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/")
                + String(city.lon, 4)
                + "/lat/" + String(city.lat, 4)
                + "/data.json";

    historyAPI = String("http://opendata-download-metobs.smhi.se/api/version/1.0/parameter/")
               + String(PARAMETERS[currentParam].id)
               + "/station/" + String(city.stationId)
               + "/period/latest-months/data.json";
}

// ================= FORMATTER =================
static String formatDateTime(const String& raw) {
    bool allDigits = true;
    for (size_t i = 0; i < raw.length(); ++i) {
        if (raw[i] < '0' || raw[i] > '9') { allDigits = false; break; }
    }
    if (allDigits && raw.length() >= 10) {
        unsigned long long ms = strtoull(raw.c_str(), NULL, 10);
        time_t t = (time_t)(ms / 1000ULL);
        struct tm tm_info;
        localtime_r(&t, &tm_info);
        char buf[20];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm_info);
        return String(buf);
    }

    int tpos = raw.indexOf('T');
    if (tpos == 10 && raw.length() >= 16) {
        String datePart = raw.substring(0, 10);
        String timePart = raw.substring(11, 16);
        return datePart + " " + timePart;
    }
    return raw;
}

// ================= FETCH FUNCTIONS =================
bool fetchForecast() {
    WiFiClient client;
    HTTPClient http;
    http.useHTTP10(true);
    http.setTimeout(15000);

    if (!http.begin(client, forecastAPI)) {
        http.end();
        return false;
    }
    int code = http.GET();
    if (code != 200) {
        http.end();
        return false;
    }

    WiFiClient& stream = http.getStream();
    DynamicJsonDocument doc(4096);
    auto err = deserializeJson(doc, stream);
    http.end();
    if (err) {
        return false;
    }

    forecastCount = 0;
    JsonArray ts = doc["timeSeries"].as<JsonArray>();
    for (JsonObject obj : ts) {
        if (forecastCount >= 7) break;

        String time = obj["validTime"];
        if (time.indexOf("T12:00") != -1) {
            float temp = NAN;
            int   wsymb = -1;

            for (JsonObject p : obj["parameters"].as<JsonArray>()) {
                const char* n = p["name"];
                if (!n) continue;
                if (strcmp(n, "t") == 0) {
                    temp = p["values"][0];
                } else if (strcmp(n, "Wsymb2") == 0) {
                    wsymb = p["values"][0];
                }
            }

            if (!isnan(temp)) {
                forecast[forecastCount].date          = time.substring(0, 10);
                forecast[forecastCount].temp          = temp;
                forecast[forecastCount].weatherSymbol = wsymb;
                forecast[forecastCount].icon          = getWeatherIcon(wsymb); // kvar för kompatibilitet
                forecastCount++;
            }
        }
    }

    return forecastCount > 0;
}

// ==== HJÄLPARE: Rensa barn i container (för prognoslistan) ====
static void clear_children(lv_obj_t* parent) {
    if (!parent) return;
    while (lv_obj_get_child_cnt(parent) > 0) {
        lv_obj_del(lv_obj_get_child(parent, 0));
    }
}

// ================= UI FUNCTIONS =================
// ERSATT: bygg rader med LVGL-ikoner + datum + temp i stället för en samlad text
void updateForecastScreen() {
    if (forecastCount == 0) {
        lv_label_set_text(forecastLabel, "Ingen prognosdata!");
        if (forecastList) clear_children(forecastList);
        return;
    }

    // Rubrik överst
    lv_label_set_text(forecastLabel, "Prognos kl 12:00 (7 dagar)");

    // Töm listan före omritning
    if (forecastList) clear_children(forecastList);

    // Skapa en rad per dag: [ikon] [datum] [temperatur]
    for (int i = 0; i < forecastCount; ++i) {
        // Radcontainer
        lv_obj_t* row = lv_obj_create(forecastList);
        lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_bg_opa(row, LV_OPA_0, 0);
        lv_obj_set_style_bg_color(row, lv_palette_main(LV_PALETTE_GREY), 0);
        lv_obj_set_style_radius(row, 8, 0);
        lv_obj_set_style_pad_all(row, 8, 0);
        lv_obj_set_style_outline_width(row, 0, 0);   // <— ny
        lv_obj_set_style_shadow_width(row, 0, 0);    // <— ny
        lv_obj_set_style_pad_row(row, 0, 0);
        lv_obj_set_style_pad_column(row, 10, 0);

        // LVGL-väderikon baserad på Wsymb2 (rätt symboler)
        lv_obj_t* icon = weather_icon_create(row, forecast[i].weatherSymbol, 42);
        LV_UNUSED(icon);

        // Datumlabel
        lv_obj_t* dateLbl = lv_label_create(row);
        lv_label_set_text_fmt(dateLbl, "%s", forecast[i].date.c_str());
        lv_obj_set_style_text_font(dateLbl, &lv_font_montserrat_24, 0);
        lv_obj_align(dateLbl, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_align(dateLbl, LV_TEXT_ALIGN_CENTER, 0);

        // Temperaturlabel – högerjusterad i raden
        lv_obj_t* tempLbl = lv_label_create(row);
        lv_label_set_text_fmt(tempLbl, "%.1f°C", forecast[i].temp);
        lv_obj_set_style_text_font(tempLbl, &lv_font_montserrat_24, 0);
        lv_obj_set_flex_grow(tempLbl, 1);
        lv_obj_align(tempLbl, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_set_style_text_align(tempLbl, LV_TEXT_ALIGN_RIGHT, 0);
        // Om du vill ha högerställd text:
        // lv_obj_set_style_text_align(tempLbl, LV_TEXT_ALIGN_RIGHT, 0);
    }
}

bool fetchHistorical() {
    DynamicJsonDocument doc(65536);
    WiFiClient client;
    HTTPClient http;
    http.setTimeout(15000);

    if (!http.begin(client, historyAPI)) {
        if (historyStatusLabel) lv_label_set_text(historyStatusLabel, "HTTP start fail");
        http.end();
        return false;
    }

    int code = http.GET();
    if (historyStatusLabel) lv_label_set_text_fmt(historyStatusLabel, "HTTP: %d", code);
    if (code != 200) {
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    auto err = deserializeJson(doc, payload);
    if (err) {
        if (historyStatusLabel) lv_label_set_text(historyStatusLabel, "JSON error");
        return false;
    }

    historyCount = 0;
    JsonArray vals = doc["value"].as<JsonArray>();
    for (JsonObject obj : vals) {
        if (historyCount >= 720) break;
        if (obj.containsKey("value")) {
            history[historyCount].date  = obj["date"].as<String>();
            history[historyCount].value = obj["value"].as<float>();
            historyCount++;
        }
    }

    return historyCount > 0;
}

// ================= UI FUNCTIONS (Historik) =================
void updateHistoricalChart(int sliderVal) {
    if (!historyChart || !historySeries || !markerSeries || !pointInfoLabel) return;

    chartPointCount = min(historyCount, 100);
    lv_chart_set_point_count(historyChart, chartPointCount);
    lv_chart_set_all_value(historyChart, historySeries, LV_CHART_POINT_NONE);
    lv_chart_set_all_value(historyChart, markerSeries, LV_CHART_POINT_NONE);

    lastStart = map(sliderVal, 0, 100, 0, max(0, historyCount - chartPointCount));

    for (int i = 0; i < chartPointCount; i++) {
        int idx = lastStart + i;
        if (idx >= historyCount) break;
        lv_chart_set_next_value(historyChart, historySeries, (int)history[idx].value);
    }

    if (lastStart >= 0 && lastStart < historyCount) {
        String info = formatDateTime(history[lastStart].date) + " = "
                    + String(history[lastStart].value, 1) + " "
                    + PARAM_UNITS[currentParam];
        lv_label_set_text(pointInfoLabel, info.c_str());
    }

    lv_chart_refresh(historyChart);
}

// === Axeltexter ===
static void updateAxisLabels() {
    if (!xAxisLabel || !yAxisLabel) return;

    // X-axel: alltid tid i timmar
    lv_label_set_text(xAxisLabel, "X: Tid (h)");

    // Y-axel: parameter + enhet utifrån currentParam
    String yText = "Y: " + String(PARAM_NAMES_SV[currentParam]) + " (" + String(PARAM_UNITS[currentParam]) + ")";
    lv_label_set_text(yAxisLabel, yText.c_str());
}

// ================= EVENTS =================
static void slider_event_cb(lv_event_t*) {
    if (slider) updateHistoricalChart(lv_slider_get_value(slider));
}

static void chart_event_cb(lv_event_t*) {
    if (!historyChart || chartPointCount <= 0) return;

    lv_point_t p;
    lv_indev_get_point(lv_indev_get_act(), &p);

    int w = lv_obj_get_width(historyChart);
    int dispIdx   = map(p.x, 0, w, 0, chartPointCount - 1);
    int actualIdx = lastStart + dispIdx;
    actualIdx     = constrain(actualIdx, 0, historyCount - 1);

    String info = formatDateTime(history[actualIdx].date) + " = "
                + String(history[actualIdx].value, 1) + " "
                + PARAM_UNITS[currentParam];
    lv_label_set_text(pointInfoLabel, info.c_str());

    lv_chart_set_all_value(historyChart, markerSeries, LV_CHART_POINT_NONE);
    lv_chart_set_value_by_id(historyChart, markerSeries, dispIdx, (int)history[actualIdx].value);
    lv_chart_refresh(historyChart);
}

static void tileview_event_cb(lv_event_t* e) {
    LV_UNUSED(e);
    lv_obj_t* act = lv_tileview_get_tile_act(tileview);

    if (act == t1) {
        updateAPIUrls();
        if (fetchForecast()) updateForecastScreen();
    }
    else if (act == t2) {
        updateAPIUrls();
        if (fetchHistorical()) updateHistoricalChart(lv_slider_get_value(slider));
        updateAxisLabels(); // Se till att axeltexterna stämmer på historikskärmen
    }
}

static void city_dropdown_event_cb(lv_event_t*) {
    currentCity = lv_dropdown_get_selected(cityDropdown);
    if (forecastCityLabel)
        lv_label_set_text_fmt(forecastCityLabel, "Stad: %s", CITIES[currentCity].name);
    if (historyCityLabel)
        lv_label_set_text_fmt(historyCityLabel, "Stad: %s", CITIES[currentCity].name);
}

static void param_dropdown_event_cb(lv_event_t*) {
    currentParam = lv_dropdown_get_selected(paramDropdown);
    // Uppdatera axeltexterna när parameter ändras
    updateAxisLabels();
    
  // Uppdatera grafens Y-range direkt när parametern byts
  switch (currentParam) {
      case 0: lv_chart_set_range(historyChart, LV_CHART_AXIS_PRIMARY_Y, -30, 40);  break;
      case 1: lv_chart_set_range(historyChart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);   break;
      case 2: lv_chart_set_range(historyChart, LV_CHART_AXIS_PRIMARY_Y, 0, 30);    break;
      case 3: lv_chart_set_range(historyChart, LV_CHART_AXIS_PRIMARY_Y, 975, 1025);break;
  }

    // (Valfritt) Ladda om historik/graf direkt vid ändring:
    // updateAPIUrls();
    // if (fetchHistorical()) updateHistoricalChart(lv_slider_get_value(slider));
}

static void reset_defaults_event_cb(lv_event_t*) {
    currentCity  = defaultCity;
    currentParam = defaultParam;
    if (cityDropdown)  lv_dropdown_set_selected(cityDropdown,  currentCity);
    if (paramDropdown) lv_dropdown_set_selected(paramDropdown, currentParam);
}

static void popup_close_cb(lv_event_t* e) {
    lv_msgbox_close(lv_event_get_target(e));
}

static void save_defaults_event_cb(lv_event_t*) {
    defaultCity  = currentCity;
    defaultParam = currentParam;
    prefs.putInt("defaultCity",  defaultCity);
    prefs.putInt("defaultParam", defaultParam);

    static const char* btns[] = {"OK", ""};
    lv_obj_t* msg = lv_msgbox_create(NULL, "Saved!", "Default saved!", btns, true);
    lv_obj_center(msg);
    lv_obj_add_event_cb(msg, popup_close_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

// ================= CREATE UI =================

void createUI() {
    tileview = lv_tileview_create(lv_scr_act());
    t0 = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_HOR);
    t1 = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_HOR);
    t2 = lv_tileview_add_tile(tileview, 2, 0, LV_DIR_HOR);
    t3 = lv_tileview_add_tile(tileview, 3, 0, LV_DIR_HOR);

    // -------------------------
    // Startskärm (t0)
    // -------------------------
    lv_obj_t* startLabel = lv_label_create(t0);
    lv_label_set_text_fmt(startLabel, "Weather App\nVersion: %s\nGroup: %s", APP_VERSION, GROUP_NAME);
    lv_obj_center(startLabel);

    // -------------------------
    // Prognosskärm (t1)
    // -------------------------
    forecastLabel = lv_label_create(t1);
    lv_obj_align(forecastLabel, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_text_font(forecastLabel, &lv_font_montserrat_24, 0);
    lv_label_set_text(forecastLabel, "Swipe for forecast");

    forecastCityLabel = lv_label_create(t1);
    lv_label_set_text_fmt(forecastCityLabel, "Stad: %s", CITIES[currentCity].name);
    lv_obj_align(forecastCityLabel, LV_ALIGN_TOP_LEFT, 10, 50);

    // Container för prognosrader (ikon + datum + temp)
    forecastList = lv_obj_create(t1);
    lv_obj_set_size(
        forecastList,
        lv_disp_get_hor_res(NULL) - 20,
        lv_disp_get_ver_res(NULL) - 120 // lämna plats ovan för rubriker
    );
    lv_obj_align(forecastList, LV_ALIGN_TOP_MID, 0, 90);

    // Flex-layouter: kolumn med auto-höjd rader
    lv_obj_set_flex_flow(forecastList, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(forecastList, 8, 0);
    lv_obj_set_style_pad_column(forecastList, 8, 0);
    lv_obj_set_style_pad_all(forecastList, 6, 0);

    // Gör container transparent och utan outline/skuggor
    lv_obj_set_style_bg_opa(forecastList, LV_OPA_0, 0);
    lv_obj_set_style_border_width(forecastList, 0, 0);
    lv_obj_set_style_outline_width(forecastList, 0, 0);
    lv_obj_set_style_shadow_width(forecastList, 0, 0);

    // ---- FIX: stäng av scrollbar helt (tar bort stora grå strecket) ----
    lv_obj_set_scrollbar_mode(forecastList, LV_SCROLLBAR_MODE_OFF);
    // extra: gör scrollbar osynlig om tema ändå försöker rita den



   
    // -------------------------
    // Historikskärm (t2) – enkel och robust layout
    // -------------------------

    // Stad-label (överst till vänster)
    historyCityLabel = lv_label_create(t2);
    lv_label_set_text_fmt(historyCityLabel, "Stad: %s", CITIES[currentCity].name);
    lv_obj_align(historyCityLabel, LV_ALIGN_TOP_LEFT, 10, 10);

    // Y-axelns label – ovanför grafen, centrerad
    yAxisLabel = lv_label_create(t2);
    // Text sätts av updateAxisLabels() nedan
    lv_obj_align(yAxisLabel, LV_ALIGN_TOP_LEFT, 10, 50);
    // (Valfritt) större font:
    // lv_obj_set_style_text_font(yAxisLabel, &lv_font_montserrat_20, 0);

    // Själva grafen
    historyChart = lv_chart_create(t2);

    // Beräkna robusta dimensioner för grafen så den alltid syns
    lv_coord_t scrW = lv_disp_get_hor_res(NULL);
    lv_coord_t scrH = lv_disp_get_ver_res(NULL);

    // Lämna luft upptill (stad + yLabel), nedtill (xLabel + slider)
    const lv_coord_t top_margin    = 80;   // 10 (stad) + ~30–40 (yLabel) + buffert
    const lv_coord_t bottom_margin = 90;   // ~xLabel + slider + buffert

    lv_coord_t chart_w = scrW - 20;        // 10px marginal per sida
    lv_coord_t chart_h = scrH - (top_margin + bottom_margin);
    if (chart_h < 100) chart_h = 100;      // fall-back så grafen inte blir 0/negativ

    lv_obj_set_size(historyChart, chart_w, chart_h);
    lv_obj_align(historyChart, LV_ALIGN_TOP_MID, 0, top_margin);
    lv_chart_set_type(historyChart, LV_CHART_TYPE_LINE);

    // Serier
    historySeries = lv_chart_add_series(historyChart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);
    markerSeries  = lv_chart_add_series(historyChart, lv_palette_main(LV_PALETTE_RED),  LV_CHART_AXIS_PRIMARY_Y);

    // Click event för punktinfo
    lv_obj_add_event_cb(historyChart, chart_event_cb, LV_EVENT_CLICKED, NULL);

    // Punktinfo (uppe till höger, lämnas som tidigare)
    pointInfoLabel = lv_label_create(t2);
    lv_label_set_text(pointInfoLabel, "Tryck på punkt i grafen");
    lv_obj_align(pointInfoLabel, LV_ALIGN_TOP_RIGHT, -10, 10);

    // X-axelns label – under grafen, centrerad
    xAxisLabel = lv_label_create(t2);
    lv_label_set_text(xAxisLabel, "X: Tid (h)");
    lv_obj_align_to(xAxisLabel, historyChart, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);

    // Slider längst ner
    slider = lv_slider_create(t2);
    lv_obj_set_width(slider, scrW - 40);
    lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 100, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Initiera etiketttexterna (så yAxisLabel får korrekt text)
    updateAxisLabels();


    // -------------------------
    // Inställningar (t3)
    // -------------------------
    lv_obj_t* settingsTitle = lv_label_create(t3);
    lv_label_set_text(settingsTitle, "Settings");
    lv_obj_align(settingsTitle, LV_ALIGN_TOP_MID, 0, 10);

    cityDropdown = lv_dropdown_create(t3);
    lv_dropdown_set_options(cityDropdown, "Karlskrona\nStockholm\nGoteborg\nMalmo\nKiruna");
    lv_obj_align(cityDropdown, LV_ALIGN_TOP_LEFT, 10, 60);
    lv_dropdown_set_selected(cityDropdown, currentCity);
    lv_obj_add_event_cb(cityDropdown, city_dropdown_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    paramDropdown = lv_dropdown_create(t3);
    lv_dropdown_set_options(paramDropdown, "Temperatur\nLuftfuktighet\nVindhastighet\nLufttryck");
    lv_obj_align(paramDropdown, LV_ALIGN_TOP_LEFT, 10, 120);
    lv_dropdown_set_selected(paramDropdown, currentParam);
    lv_obj_add_event_cb(paramDropdown, param_dropdown_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t* resetDefaultsBtn = lv_btn_create(t3);
    lv_obj_align(resetDefaultsBtn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_event_cb(resetDefaultsBtn, reset_defaults_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* resetDefaultsLabel = lv_label_create(resetDefaultsBtn);
    lv_label_set_text(resetDefaultsLabel, "Reset");

    lv_obj_t* saveDefaultsBtn = lv_btn_create(t3);
    lv_obj_align(saveDefaultsBtn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_add_event_cb(saveDefaultsBtn, save_defaults_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* saveDefaultsLabel = lv_label_create(saveDefaultsBtn);
    lv_label_set_text(saveDefaultsLabel, "Save");

    // TileView events och initial tile
    lv_obj_add_event_cb(tileview, tileview_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_set_tile(tileview, t0, LV_ANIM_OFF);
}


// ================= SETUP =================
void setup() {
    Serial.begin(115200);
    delay(1500);

    if (!amoled.begin()) {
        Serial.println("Display init fail");
        while (true) delay(1000);
    }
    beginLvglHelper(amoled);

    prefs.begin("weather", false);
    defaultCity  = prefs.getInt("defaultCity", 0);
    defaultParam = prefs.getInt("defaultParam", 0);
    currentCity  = defaultCity;
    currentParam = defaultParam;

    createUI();

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

// ================= LOOP =================
void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        updateAPIUrls();
    }
    lv_timer_handler();
    delay(5);
}