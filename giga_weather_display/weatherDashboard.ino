
/****************************************************/
// Returns the file name corresponding to the WMO Weather Code
// lets see if we can avoid the files...
#include "images/clear.h"
#include "images/dense-drizzle.h"
#include "images/dense-freezing-drizzle.h"
#include "images/fog.h"
#include "images/heavy-freezing-rain.h"
#include "images/heavy-rain.h"
#include "images/heavy-snowfall.h"
#include "images/light-drizzle.h"
#include "images/light-freezing-drizzle.h"
#include "images/light-freezing-rain.h"
#include "images/light-rain.h"
#include "images/moderate-drizzle.h"
#include "images/moderate-rain.h"
#include "images/moderate-snowfall.h"
#include "images/mostly-clear.h"
#include "images/overcast.h"
#include "images/partly-cloudy.h"
#include "images/rime-fog.h"
#include "images/slight-snowfall.h"
#include "images/snowflake.h"
#include "images/thunderstorm-with-hail.h"
#include "images/thunderstorm.h"
#include "images/keypad.h"

const char* getWeatherIconFilename(uint8_t wmo) {
  switch (wmo) {
    case 0:
      return (const char*)image_clear;  // Clear sky
    case 1:
      return (const char*)image_mostly_clear;
    case 2:
      return (const char*)image_partly_cloudy;
    case 3:
      return (const char*)image_overcast;  // Mainly clear, partly cloudy, overcast

    case 45:
      return (const char*)image_fog;
    case 48:
      return (const char*)image_rime_fog;  // Fog and depositing rime fog

    case 51:
      return (const char*)image_light_drizzle;
    case 53:
      return (const char*)image_moderate_drizzle;
    case 55:
      return (const char*)image_dense_drizzle;  // Drizzle: Light, moderate, dense

    case 56:
      return (const char*)image_light_freezing_drizzle;
    case 57:
      return (const char*)image_dense_freezing_drizzle;  // Freezing Drizzle

    case 61:
      return (const char*)image_light_rain;
    case 63:
      return (const char*)image_moderate_rain;
    case 65:
      return (const char*)image_heavy_rain;  // Rain: Slight, moderate, heavy

    case 66:
      return (const char*)image_light_freezing_rain;
    case 67:
      return (const char*)image_heavy_freezing_rain;  // Freezing Rain

    case 71:
      return (const char*)image_slight_snowfall;
    case 73:
      return (const char*)image_moderate_snowfall;
    case 75:
      return (const char*)image_heavy_snowfall;  // Snow fall: Slight, moderate, heavy

    case 77:
      return (const char*)image_snow_flake;  // Snow grains

    case 80:
      return (const char*)image_light_rain;
    case 81:
      return (const char*)image_moderate_rain;
    case 82:
      return (const char*)image_heavy_rain;
    case 83:
      return (const char*)image_heavy_rain;  // Rain showers: Slight, moderate, violent

    case 85:
      return (const char*)image_slight_snowfall;
    case 86:
      return (const char*)image_slight_snowfall;  // Snow showers: Slight and heavy

    case 95:
      return (const char*)image_thunderstorm;  // Thunderstorm: Slight or moderate

    case 96:
      return (const char*)image_thunderstorm_with_hail;
    case 99:
      return (const char*)image_thunderstorm_with_hail;  // Thunderstorm with hail

    case 100:
      return (const char*)image_keypad;

    default:
      return (const char*)image_clear;  // Fallback icon
  }
}

void drawWeatherDashboard() {
  tft.setTextWrap(false);

  // -------------------------------------------------------------
  // 1. HEADER ROW (Full Width)
  // -------------------------------------------------------------
  SetTFTFont(&FreeSansBold14pt7b);
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(20, 20);
  tft.print(weather.location);

  tft.setCursor(400, 20);
  tft.setTextColor(COLOR_ORANGE);
  tft.print(weather.currentTime);

  drawKeyboard(getWeatherIconFilename(100), 700, 20);


  tft.drawFastHLine(20, 42, 760, COLOR_GRAY);

  // -------------------------------------------------------------
  // 2. CURRENT CONDITIONS CARD (Left Side: X 20..380)
  // -------------------------------------------------------------
  tft.drawRoundRect(20, 55, 360, 240, 12, COLOR_YELLOW);

  SetTFTFont(&FreeSansBold12pt7b);
  tft.setCursor(35, 80);
  tft.setTextColor(COLOR_CYAN);
  tft.print("Current Conditions:");

  tft.setCursor(35, 110);
  tft.setTextColor(COLOR_WHITE);
  tft.print(weather.conditionText);

  // Current Temperature
  SetTFTFont(&FreeSansBold12pt7b);
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(35, 160);
  tft_printf("%.1f deg F", (double)weather.currentTemp);

  // Weather Icon (positioned right inside the box)
  drawPNG(getWeatherIconFilename(weather.currentWmoCode), 230, 125);

  // Air Quality Index Badge
  drawAQIMetric(35, 230, weather.AQI);

  // -------------------------------------------------------------
  // 3. METRICS GRID (Right Side: X 400..780, 2 Columns)
  // -------------------------------------------------------------
  tft.drawRoundRect(400, 55, 380, 240, 12, COLOR_GRAY);

  struct Metric {
    const char* label;
    String val;
  };
  Metric metrics[] = {
    { "Sunrise:", weather.sunrise },
    { "Sunset:", weather.sunset },
    { "Humidity:", String(weather.humidity) + "%" },
    { "Wind:", String(weather.windSpeed, 1) + " mph" },
    { "Wind Dir:", String(weather.windDirection) + " deg" },
    { "Pressure:", String(weather.pressure, 1) + " hPa" },
    { "Rain:", String(weather.rain, 2) + " in" },
    { "Snow:", String(weather.snow, 1) + " in" },
    { "Precip:", String(weather.precip, 2) + " in" }
  };

  int col1X = 415;  //was 415
  int col2X = 600;
  int startY = 70;
  int lineSpacing = 42;

  for (int i = 0; i < 9; i++) {
    int col = i / 5;  // Left column (0-4), Right column (5-8)
    int row = i % 5;
    int x = (col == 0) ? col1X : col2X;
    int y = startY + (row * lineSpacing);

    SetTFTFont(&FreeSansBold10pt7b);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(x, y);
    tft.print(metrics[i].label);

    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(x + 95, y);
    tft.print(metrics[i].val);
  }

  // -------------------------------------------------------------
  // 4. 5-DAY FORECAST CARDS (Bottom: Y 310..465 across 800px)
  // -------------------------------------------------------------
  int colWidth = 140;
  int startX = 20;
  int spacing = 15;
  int cardY = 310;
  int cardHeight = 155;

  SetTFTFont(&FreeSansBold10pt7b);
  for (int i = 0; i < 5; i++) {
    int x = startX + (i * (colWidth + spacing));

    // Card Container
    tft.drawRoundRect(x, cardY, colWidth, cardHeight, 10, COLOR_YELLOW);

    // Weather Icon
    const char* icon = getWeatherIconFilename(weather.wmoCodes[i]);
    drawPNG(icon, x + 70, cardY + 15);

    // Date & Day
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(x + 10, cardY + 15);
    tft.print(weather.dates[i]);

    tft.setCursor(x + 10, cardY + 35);
    tft.print(weather.days[i]);

    // High / Low Temperatures
    tft.setTextColor(COLOR_ORANGE);
    tft.setCursor(x + 10, cardY + 110);
    tft_printf("H: %.1fF", (double)weather.tempsHigh[i]);

    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(x + 10, cardY + 135);
    tft_printf("L: %.1fF", (double)weather.tempsLow[i]);
  }
}

void showMainDashboard() {
  currentScreen = SCREEN_MAIN;
  tft.fillScreen(COLOR_BG);
  drawWeatherDashboard();
}

void showDayDetailScreen(int dayIndex) {
  currentScreen = SCREEN_DETAIL;
  currentSelectedDay = dayIndex;

  tft.fillScreen(COLOR_BG);

  // -------------------------------------------------------------
  // 1. TOP HEADER BAR
  // -------------------------------------------------------------
  SetTFTFont(&FreeSansBold14pt7b);
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(20, 20);
  tft_printf("Forecast: %s (%s)", weather.days[dayIndex].c_str(), weather.dates[dayIndex].c_str());

  tft.drawFastHLine(20, 50, 760, COLOR_GRAY);

  // -------------------------------------------------------------
  // 2. MAIN CONDITION CARD (LEFT: X 20..380)
  // -------------------------------------------------------------
  tft.drawRoundRect(20, 65, 360, 330, 12, COLOR_YELLOW);

  SetTFTFont(&FreeSansBold14pt7b);
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(40, 85);
  tft.print(getWeatherDescription(weather.wmoCodes[dayIndex]));

  const char* icon = getWeatherIconFilename(weather.wmoCodes[dayIndex]);
  drawPNG(icon, 140, 140);

  SetTFTFont(&FreeSansBold12pt7b);
  tft.setTextColor(COLOR_ORANGE);
  tft.setCursor(40, 310);
  tft_printf("High: %.1f deg F", (double)weather.tempsHigh[dayIndex]);

  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(40, 340);
  tft_printf("Low:  %.1f deg F", (double)weather.tempsLow[dayIndex]);

  // -------------------------------------------------------------
  // 3. DETAILED METRIC LIST (RIGHT: X 400..780)
  // -------------------------------------------------------------
  tft.drawRoundRect(400, 65, 380, 330, 12, COLOR_YELLOW);

  int startX = 420;
  int startY = 85;
  int spacing = 30;

  struct Metric {
    const char* label;
    String val;
    uint16_t color;
  };
  Metric metrics[] = {
    { "Sunrise:", weather.dailySunrise[dayIndex], COLOR_YELLOW },
    { "Sunset:", weather.dailySunset[dayIndex], COLOR_ORANGE },
    { "Humidity:", String(weather.dailyHumidity[dayIndex]) + "%", COLOR_CYAN },
    { "PoP:", String(weather.pop[dayIndex]) + "%", COLOR_CYAN },
    { "Precip:", String(weather.precipSum[dayIndex], 2) + " in", COLOR_WHITE },
    { "Rain:", String(weather.rainSum[dayIndex], 2) + " in", COLOR_WHITE },
    { "Snow:", String(weather.snowSum[dayIndex], 2) + " in", COLOR_WHITE },
    { "Max Wind:", String(weather.windMax[dayIndex], 1) + " mph", COLOR_YELLOW },
    { "Max Gusts:", String(weather.gustsMax[dayIndex], 1) + " mph", COLOR_ORANGE }
  };
  for (unsigned int i = 0; i < (sizeof(metrics) / sizeof(metrics[0])); i++) {
    int y = startY + (i * spacing);

    SetTFTFont(&FreeSansBold12pt7b);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(startX, y);
    tft.print(metrics[i].label);

    tft.setTextColor(metrics[i].color);
    tft.setCursor(startX + 160, y);
    tft.print(metrics[i].val);
  }

  // -------------------------------------------------------------
  // 4. BOTTOM ACTION BAR (BACK BUTTON)
  // -------------------------------------------------------------
  tft.fillRoundRect(20, 410, 760, 55, 10, COLOR_WHITE);
  SetTFTFont(&FreeSansBold14pt7b);
  tft.setTextColor(COLOR_BLACK);
  tft.setCursor(360, 428);
  tft.print("BACK");
}

/***************** AQI ANALYSIS **********************/
// Retrieve category information from AQI score
AQI_Category getAQIInfo(int aqi) {
  aqi = constrain(aqi, 0, 500);
  for (int i = 0; i < 6; i++) {
    if (aqi >= AQI_TABLE[i].aqiMin && aqi <= AQI_TABLE[i].aqiMax) {
      return AQI_TABLE[i];
    }
  }
  return AQI_TABLE[5];  // Default to Hazardous fallback
}

// Linear Interpolation Equation: I = ((I_high - I_low)/(C_high - C_low)) * (C - C_low) + I_low
int calculatePM25AQI(float pm25) {
  if (pm25 < 0.0f) return 0;
  if (pm25 > 500.4f) return 500;

  for (int i = 0; i < DAY_COUNT; i++) {
    if (pm25 >= PM25_BREAKPOINTS[i].cLow && pm25 <= PM25_BREAKPOINTS[i].cHigh) {
      float cLow = PM25_BREAKPOINTS[i].cLow;
      float cHigh = PM25_BREAKPOINTS[i].cHigh;
      int iLow = PM25_BREAKPOINTS[i].iLow;
      int iHigh = PM25_BREAKPOINTS[i].iHigh;

      return round(((float)(iHigh - iLow) / (cHigh - cLow)) * (pm25 - cLow) + iLow);
    }
  }
  return 500;
}

void drawAQIMetric(int x, int y, int aqiVal) {
  AQI_Category aqi = getAQIInfo(aqiVal);

  // Background badge
  tft.fillRoundRect(x, y, 200, 24, 6, aqi.color);

  // Text label
  SetTFTFont(&FreeSansBold10pt7b);
#if defined(ARDUINO_GIGA) || defined(ARDUINO_PORTENTA_H7_M7)
  printk("AQI Font: %p F:%u L:%u, YI:%u\n", cur_gfx_font, cur_gfx_font->first, cur_gfx_font->last, cur_gfx_font->yAdvance);
  const GFXglyph* gA = &cur_gfx_font->glyph['A' - cur_gfx_font->first];
  printf("\tA - W:%u H:%u XA:%u X:%d y:%d\n", gA->width, gA->height, gA->xAdvance, gA->xOffset, gA->yOffset);
#endif

  tft.setTextColor(COLOR_BLACK);
  tft.setCursor(x + 8, y + 15);
  tft_printf("AQI: %d", aqiVal);

  // Category status readout next to badge
  tft.setCursor(x + 80, y + 15);
  tft.setTextColor(COLOR_BLACK);
  tft.print(aqi.label);
}

#if defined(USE_KEYBOARD)
bool showKeyboard() {
  currentScreen = SCREEN_KEYBOARD;
  Serial.println(g_weather_city);
  strcpy(MyKeyboard.data, "");
  tft.fillScreen(COLOR_BG);
  // -------------------------------------------------------------
  // 4. BOTTOM ACTION BAR (BACK BUTTON)
  // -------------------------------------------------------------
  //tft.drawRoundRect(20, 255, 120, 45, 8, COLOR_WHITE);
  //SetTFTFont(&FreeSansBold12pt7b);
  //tft.setTextColor(COLOR_WHITE);
  //tft.setCursor(55, 500);
  //tft.print("BACK");

  MyKeyboard.getInput();
  Serial.print("New City is: ");
  Serial.println(MyKeyboard.data);

  //check if blank
  String test = MyKeyboard.data;
  if(test.length() < 2) {
    currentScreen = SCREEN_MAIN;
    g_weather_city = g_weather_city;
    Serial.println(g_weather_city);
    appState = FETCH_MAP_CITY_TO_LOCATION; 
    return true;
  }

  //only update on new city
  if(g_weather_city != MyKeyboard.data) {
    currentScreen = SCREEN_MAIN;
    g_weather_city = MyKeyboard.data;
    g_weather_city.trim();
    g_weather_city.replace(' ', '+');
    //Serial.print("New City: ");
    //Serial.println(g_weather_city);
    appState = FETCH_MAP_CITY_TO_LOCATION; 
    return true;
  }

  return false;
}
#endif //use keyboard