

#define USE_KEYBOARD
#define USE_JSON_SECURITY_FILE

#include <SPI.h>

#include <ZephyrClient.h>
#include <zephyr/fs/fs.h>
#include <WiFi.h>
#include "Arduino_GigaDisplay_GFX.h"

/******************** GFX FONTS ***********************/
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
//Custom Fonts, https:/rop.nl/truetype2gfx/
#include "customFonts/FreeSansBold14pt7b.h"
#include "customFonts/FreeSansBold20pt7b.h"
#include "customFonts/FreeSans14pt7b.h"
#include "customFonts/FreeSans20pt7b.h"
#include "customFonts/FreeSans16pt7b.h"
#include "customFonts/FreeSansBold16pt7b.h"
#include "customFonts/FreeSans10pt7b.h"
#include "customFonts/FreeSansBold10pt7b.h"

//Touch library
#include "Arduino_GigaDisplayTouch.h"
#include "GigaMapTouch.h"
#include "GFX_Keyboard.h"
Arduino_GigaDisplayTouch ts;

#include <ArduinoJson.h>
#include "forwardDecs.h"

// Allocated document capacity
JsonDocument doc;

const char *server = "api.open-meteo.com";
const char *geocoding_api_server = "geocoding-api.open-meteo.com";
const char *air_quality_server = "air-quality-api.open-meteo.com";

const int port = 80;
constexpr uint32_t kDHCPTimeout = 15000;

/***********************************************************/

char g_wifi_ssid[80];
char g_wifi_password[80] = "";

int keyIndex = 0;  // your network key Index number (needed only for WEP)

ZephyrClient client;


String g_weather_city = "";
uint32_t g_weather_cycle_time_ms = (uint32_t)(15 * 60 * 1000);  // cycle time in MS 10 * 60 *1000;
uint32_t g_last_cycle_time_ms = 0;
String g_weather_time_zone = "";
double g_weather_latitude = 0;
double g_weather_longitude = 0;


enum {
  FONT_10,
  FONT_11,
  FONT_12,
  FONT_14,
  FONT_20,
  FONT_10_Bold,
  FONT_11_Bold,
  FONT_12_Bold,
  FONT_14_Bold,
  FONT_18_BOLD,
  FONT_20_BOLD
};

GigaDisplay_GFX tft;

#define FONT_BUTTON FreeSans14pt7b //FreeSans18pt7b  // font for keypad buttons
uint16_t ScreenLeft = 9, ScreenRight = 790, ScreenTop = 15, ScreenBottom = 472;

Keyboard MyKeyboard(&tft, &ts);

/*************************************************/
void Serial_printf(const char *format, ...) {
  char buffer[256];
  va_list ap;
  va_start(ap, format);
  int cb_ret = vsnprintf(buffer, sizeof(buffer), format, ap);
  Serial.write(buffer, cb_ret);
}

void tft_printf(const char *format, ...) {
  char buffer[256];
  va_list ap;
  va_start(ap, format);
  int cb_ret = vsnprintf(buffer, sizeof(buffer), format, ap);
  tft.write(buffer, cb_ret);
}


const GFXfont *cur_gfx_font = nullptr;
inline void SetTFTFont(int f) {
  switch (f) {
    case FONT_10:
    case FONT_11:
      cur_gfx_font = &FreeSans10pt7b;
      tft.setFont(&FreeSans10pt7b);
      return;
    case FONT_12:
      cur_gfx_font = &FreeSans14pt7b;
      tft.setFont(&FreeSans14pt7b);
      return;

    case FONT_10_Bold:
    case FONT_11_Bold:
      cur_gfx_font = &FreeSansBold10pt7b;
      tft.setFont(&FreeSansBold10pt7b);
      return;
    case FONT_12_Bold:
      cur_gfx_font = &FreeSansBold12pt7b;
      tft.setFont(&FreeSansBold12pt7b);
    case FONT_14_Bold:
      cur_gfx_font = &FreeSansBold14pt7b;
      tft.setFont(&FreeSansBold14pt7b);
      return;
  }

}


/*************************************************/
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 4000) {}

  int status = WL_IDLE_STATUS;

  tft.begin();
  tft.setRotation(1);  // Landscape (480x320)
  tft.fillScreen(COLOR_BG);
  SetTFTFont(FONT_10);

drawWeatherDashboard();
/**/
#ifdef USE_JSON_SECURITY_FILE
  read_or_initialize_json_security_file();
#endif

  Serial.print("Network: ");
  Serial.println(g_wifi_ssid);

  if (strlen(g_wifi_password) == 0) {
    Serial.println("Enter WiFi network Password:");
    Serial.setTimeout(10000);
    String password_string = Serial.readString();
    password_string.trim();
    password_string.toCharArray(g_wifi_password, password_string.length() + 1);
    Serial.print("<");
    Serial.print(g_wifi_password);
    Serial.println(">");
  }
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true)
      ;
  }

  // attempt to connect to Wifi network:
  uint32_t time_before_wifi_begin = millis();
  while (true) {
    Serial.print("Attempting to connect to WiFi: ");
    Serial.println(g_wifi_ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(g_wifi_ssid, g_wifi_password);
    // wait 3 seconds for connection:
    if (status == WL_CONNECTED) break;
    delay(3000);
  }
  // print your board's IP address:
  Serial.print("Time elapsed in WiFi.begin loop in ms: ");
  Serial.println((uint32_t)(millis() - time_before_wifi_begin));
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  appState = (g_weather_time_zone.length() == 0) ? FETCH_MAP_CITY_TO_LOCATION : FETCH_CURRENT;


  if (ts.begin()) {
    Serial.println("Touch controller init - OK");
  } else {
    Serial.println("Touch controller init - FAILED");
    while (1)
      ;
  }

  // Initialize the touch screen mapping...
  GigaDisplayTouchMap::setScreeninfo(1, tft.width(), tft.height());

  MyKeyboard.init(COLOR_BLACK, COLOR_WHITE, COLOR_BLUE, COLOR_DARKGREY, COLOR_DARKGREY, COLOR_NAVY, COLOR_BLACK, &FONT_BUTTON);
  MyKeyboard.setTouchLimits( ScreenLeft, ScreenRight, ScreenTop, ScreenBottom);
  // optional methods
  // max input characters is controlled by in the .h file
  // #define MAX_KEYBOARD_CHARS 18
  // change input display color
  MyKeyboard.setDisplayColor(COLOR_WHITE, COLOR_BLUE);

}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void loop() {
  switch (appState) {
    case FETCH_MAP_CITY_TO_LOCATION:
      Serial.println("\n[0/3] Map City to Location...");
      if (sendMapCityRequest()) {
        appState = READ_MAP_CITY_TO_LOCATION;
      } else {
        Serial.println("Connection failed.");
        appState = DONE_APP;
      }
      break;
    case READ_MAP_CITY_TO_LOCATION:
      if (processIncomingStream(printMapCityData)) {
        appState = FETCH_CURRENT;
        // Save away the data
        write_json_security_and_location_file();
      }
      break;

    case FETCH_CURRENT:
      Serial.println("\n[1/3] Requesting Current Weather...");
      if (sendCurrentRequest()) {
        appState = READ_CURRENT;
      } else {
        Serial.println("Connection failed.");
        appState = DONE_APP;
      }
      break;

    case READ_CURRENT:
      if (processIncomingStream(printCurrentData)) {
        appState = FETCH_DAILY;  //was FETCH_HOURLY
      }
      break;

    case FETCH_HOURLY:
      Serial.println("\n[2/3] Requesting Hourly Forecast...");
      if (sendHourlyRequest()) {
        appState = READ_HOURLY;
      } else {
        Serial.println("Connection failed.");
        appState = DONE_APP;
      }
      break;

    case READ_HOURLY:
      if (processIncomingStream(printHourlyData)) {
        appState = FETCH_DAILY;
      }
      break;

    case FETCH_DAILY:
      Serial.println("\n[3/3] Requesting Daily Forecast...");
      if (sendDailyRequest()) {
        appState = READ_DAILY;
      } else {
        Serial.println("Connection failed.");
        appState = DONE_APP;
      }
      break;

    case READ_DAILY:
      if (processIncomingStream(printDailyData)) {
        appState = FETCH_AIR_QUALITY;
      }
      break;

    case FETCH_AIR_QUALITY:
      Serial.println("\n[4/4] Requesting Air Quality...");
      if (sendAirQualityRequest()) {
        appState = READ_AIR_QUALITY;
      } else {
        Serial.println("Connection failed.");
        appState = DONE_APP;
      }
      break;

    case READ_AIR_QUALITY:
      if (processIncomingStream(printAirQualityData)) {
        drawWeatherDashboard();
        Serial.println("All data successfully fetched!");
        Serial.println("Enter City name:");
        appState = DONE_APP;
        g_last_cycle_time_ms = millis();
      }
      break;
    case DONE_APP:
      break;

    default:
      break;
  }


  uint8_t contacts;
  GDTpoint_t points[5];

  contacts = ts.getTouchPoints(points);

  if (contacts > 0) {
    int touchX;
    int touchY;
    GigaDisplayTouchMap::mapTouchPoint(points[0].x,points[0].y, touchX, touchY);
    //int touchX = points[0].x;
    //int touchY = points[0].y;

    // STATE 1: Processing touches on the Main Dashboard
    if (currentScreen == SCREEN_MAIN) {
      int selectedDay = getTouchedForecastCard(touchX, touchY);
      if (selectedDay != -1) {
        Serial_printf("Opening Detail View for Day %d\n", selectedDay);
        showDayDetailScreen(selectedDay);
        delay(300);  // Debounce touch
      }
#if defined(USE_KEYBOARD)
      if(isKeyboardClicked(touchX, touchY)) {
        if (showKeyboard()) {
          return; // new city typed in.
        }
      }
#endif
    }

    // STATE 2: Processing touches on the Detail Screen
    else if (currentScreen == SCREEN_DETAIL) {
      if (isBackButtonClicked(touchX, touchY)) {
        Serial.println("Back Button Pressed! Returning to Main Dashboard...");
        showMainDashboard();
        delay(300);  // Debounce touch
      }
    }
    else if (currentScreen == SCREEN_KEYBOARD) {
      if (isBackButtonClicked(touchX, touchY)) {
        //Serial.println("Back Button Pressed! Returning to Main Dashboard...");
        showMainDashboard();
        delay(50); // Debounce touch
      }
    }
  }

  if (Serial.available()) {
    g_weather_city = Serial.readString();
    g_weather_city.trim();
    g_weather_city.replace(' ', '+');
    Serial.print("New City: ");
    Serial.println(g_weather_city);
    appState = FETCH_MAP_CITY_TO_LOCATION;
  }

  // see if we timed out and should start a new read cycle
  uint32_t delta_time = millis() - g_last_cycle_time_ms;
  if (delta_time > g_weather_cycle_time_ms) {
    Serial.println("\n*** Start new read cycle ***");
    appState = (g_weather_time_zone.length() == 0) ? FETCH_MAP_CITY_TO_LOCATION : FETCH_CURRENT;
    g_last_cycle_time_ms = millis();  // don't keep hitting this
  }
}


// Returns card index (0 to 4) if pressed, or -1 if touch is outside cards
int getTouchedForecastCard(int touchX, int touchY) {
  int colWidth = 140;
  int startX = 20;
  int spacing = 15;
  int cardY = 310;
  int cardH = 155;

  // Loop through all 5 cards and check bounding boxes
  for (int i = 0; i < 5; i++) {
    int cardX = startX + (i * (colWidth + spacing));

    if (touchX >= cardX && touchX <= (cardX + colWidth) && touchY >= cardY && touchY <= (cardY + cardH)) {
      return i;  // Touched card index
    }
  }

  return -1;  // No forecast card touched
}

bool isBackButtonClicked(int touchX, int touchY) {
  // Matches full-width bottom bar: fillRoundRect(20, 410, 760, 55, 10, COLOR_WHITE)
  int btnX = 20;
  int btnY = 410;
  int btnW = 760;
  int btnH = 55;

  return (touchX >= btnX && touchX <= (btnX + btnW) && touchY >= btnY && touchY <= (btnY + btnH));
}

#if defined(USE_KEYBOARD)
bool isKeyboardClicked(int touchX, int touchY) {
//Serial.print(touchX); Serial.print(", "); Serial.println(touchY);

  int btnX = 700;
  int btnY = 0;
  int btnW = 100;
  int btnH = 28;

  return (touchX >= btnX && touchX <= (btnX + btnW) &&
          touchY >= btnY && touchY <= (btnY + btnH));
}
#endif  //end keyboard
