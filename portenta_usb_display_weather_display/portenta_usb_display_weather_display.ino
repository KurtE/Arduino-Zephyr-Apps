//=============================================================================
// A Weather display app for the Arduino GIGA board and GIGA Display shield
// that runs on the Arduino Zephyr setup (Boards->Arduino Zephyr Boards)
// This sketch was developed to test out different features of the
// ArduinoCore-zephyr code base on the GIGA.
//
// Developed by: mjs513 and KurtE
//
// It uses the boards built in Wifi controller to communicate with the
// website open-meteo.com to request location, weather and air quality
// information, which is returned in a JSON format, which is displayed
// on the screen.
//
// This sketch stores, the Wifi-security information as well as
// site location into a JSON type file, that is stored in the
// /storage section of the Flash memory.  This may require you to run
// the Example: Storage->FlashFormat to both setup the storage area
// as well as preserve/install the Wifi information.
//
// Note: we have found some GIGAs appear to have issues at times
// connecting to the Wifi.  In those cases, we have found that if you
// turn it off and try again several minutes later it may work again.
//
// This sketch also uses the touch controller on the display.  If you
// click on the 5-day forecast area on a day, the display will be
// updated to show that day’s information, clicking the back button,
// returns you to the main screen.  If you click on the keyboard
// icon toward the upper right, it brings up a keyboard entry
// area which allows you to type in a new location, which can
// be any location known by open-meteo, including zip codes.
// Alternatively, you can also type in a new location using
// the serial monitor.
//
// The Keyboard code is a modified version of:
// https://github.com/KrisKasprzak/ILI9341_t3_Keypad
//
// Note: We have found at least two configurations of the GT911
// touch controller, and this code includes code to detect
// the two we know of and maps the touch point to the display
// points for the different orientations of the display.
//
// While doing this we have done similar programs on some different hardware
// specifically on Teensy 4.x boards using different Wifi Setups.
//     https://github.com/mjs513/Teensy-WiFi-Apps
//
// Thes different sketches are discussed on a few different forum threads:
// https://forum.arduino.cc/t/playing-with-zephyr-v1-0-0-on-giga-and-wifi/1458103
// https://forum.pjrc.com/index.php?threads/teensy-4-x-esp32-stack.78071/
// https://forum.pjrc.com/index.php?threads/call-to-arms-teensy-wifi-true.77099/
//
// Warning: There are no guarantees or warrantees with this sketch.
// Use it at your own risk or hopefully fun.
//=============================================================================


#define USE_KEYBOARD
#define USE_ETHERNET
#define USE_JSON_SECURITY_FILE

#include <SPI.h>

#include <ZephyrClient.h>
#include <zephyr/fs/fs.h>
#ifdef USE_ETHERNET
#include "ZephyrEthernet.h"
// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 2, 177);
IPAddress myDns(192, 168, 2, 1);
#else
#include <WiFi.h>
int status = WL_IDLE_STATUS;
#endif
#include "Arduino_Video.h"
#include "USBCVideo_GFX.h"

/******************** GFX FONTS ***********************/
//#include <Fonts/FreeSans9pt7b.h>
//#include <Fonts/FreeSans12pt7b.h>
//#include <Fonts/FreeSans18pt7b.h>
//#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
//#include <Fonts/FreeSansBold18pt7b.h>
//Custom Fonts, https:/rop.nl/truetype2gfx/
#include "customFonts/FreeSansBold14pt7b.h"
//#include "customFonts/FreeSansBold20pt7b.h"
#include "customFonts/FreeSans14pt7b.h"
//#include "customFonts/FreeSans20pt7b.h"
//#include "customFonts/FreeSans16pt7b.h"
//#include "customFonts/FreeSansBold16pt7b.h"
#include "customFonts/FreeSans10pt7b.h"
#include "customFonts/FreeSansBold10pt7b.h"

//Touch library
#include "GFX_Keyboard.h"
#ifdef USE_GT911_TOUCH
#include "Arduino_GigaDisplayTouch.h"
#include "GigaMapTouch.h"
#include "GFX_Keyboard.h"
Arduino_GigaDisplayTouch ts;
#endif

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

// lets try to add a Quick button.
#include <SparkFun_Qwiic_Button.h>
#include "SparkFun_Qwiic_Keypad_Arduino_Library.h"  //Click here to get the library: http://librarymanager/All#SparkFun_keypad
QwiicButton qbutton;
KEYPAD qkeypad;  //Create instance of this object
bool qbutton_found;
bool qkeypad_found;

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

USBCVideo_GFX tft;
Keyboard MyKeyboard(&tft, &qkeypad);

#define FONT_BUTTON FreeSans14pt7b  //FreeSans18pt7b  // font for keypad buttons
uint16_t ScreenLeft = 9, ScreenRight = 790, ScreenTop = 15, ScreenBottom = 472;

#ifdef USE_GT911_TOUCH
Keyboard MyKeyboard(&tft, &ts);
#endif

/*************************************************/
#define Serial_printf printk
#if 0
void Serial_printf(const char *format, ...) {
  char buffer[256];
  va_list ap;
  va_start(ap, format);
  int cb_ret = vsnprintf(buffer, sizeof(buffer), format, ap);
  Serial.write(buffer, cb_ret);
}
#endif

class SerialToprintk : public Print {
public:
  SerialToprintk() : cb_buffer(0) {}
  virtual size_t write(uint8_t c) {
    buffer[cb_buffer++] = c;
    if (cb_buffer >= (sizeof(buffer) - 1)) {
      buffer[sizeof(buffer) - 1] = 0;
      printk("%s", buffer);
      cb_buffer = 0;
    }
    return 1;
  }
  virtual int availableForWrite() {
    return 1000;
  }
  void flush() {
    if (cb_buffer && (cb_buffer < sizeof(buffer))) {
      buffer[cb_buffer] = 0;
      printk("%s", buffer);
      cb_buffer = 0;
    }
  }

  uint8_t buffer[80];
  uint8_t cb_buffer = 0;  // can only go to 255
};


void tft_printf(const char *format, ...) {
  char buffer[256];
  va_list ap;
  va_start(ap, format);
  int cb_ret = vsnprintf(buffer, sizeof(buffer), format, ap);
  tft.write(buffer, cb_ret);
}


const GFXfont *cur_gfx_font = nullptr;
inline void SetTFTFont(const GFXfont *f) {
  cur_gfx_font = f;
  tft.setFont(f);
}


/*************************************************/
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 4000) {}


  tft.begin();
  tft.setRotation(0);  // Landscape (480x320)
  tft.fillScreen(COLOR_BG);
  SetTFTFont(&FreeSans10pt7b);

  drawWeatherDashboard();
  printk("after call: drawWeatherDashboard\n");

#ifdef USE_JSON_SECURITY_FILE
  read_or_initialize_json_security_file();
  printk("after call: read_or_initialize_json_security_file\n");
#endif
#ifdef USE_ETHERNET
  while (Ethernet.linkStatus() != LinkON) {
    Serial_printf("Waiting for link on\n");
    delay(100);
  }

  // start the Ethernet connection:
  Serial_printf("Initialize Ethernet with DHCP:\n");
  if (Ethernet.begin() == 0) {
    Serial_printf("Failed to configure Ethernet using DHCP\n");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial_printf("Ethernet shield was not found.  Sorry, can't run without hardware. :(\n");
      while (true) {
        delay(1);  // do nothing, no point running without Ethernet hardware
      }
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial_printf("Ethernet cable is not connected.\n");
    }
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(ip, myDns);
  } else {
    Serial_printf("  DHCP assigned IP %s\n", Ethernet.localIP().toString());
  }

#else
  printk("Network: %s\n", g_wifi_ssid);

  if (strlen(g_wifi_password) == 0) {
    printk("Enter WiFi network Password:\n");
    Serial.setTimeout(10000);
    String password_string = Serial.readString();
    password_string.trim();
    password_string.toCharArray(g_wifi_password, password_string.length() + 1);
    printk("<%s>\n", g_wifi_password);
  }
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_SHIELD) {
    printk("Communication with WiFi module failed!\n");
    // don't continue
    while (true)
      ;
  }

  // attempt to connect to Wifi network:
  uint32_t time_before_wifi_begin = millis();
  while (true) {
    printk("Attempting to connect to WiFi: %s\n", g_wifi_ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(g_wifi_ssid, g_wifi_password);
    // wait 3 seconds for connection:
    if (status == WL_CONNECTED) break;
    delay(3000);
  }
  // print your board's IP address:
  printk("Time elapsed in WiFi.begin loop in ms: %u\n", (uint32_t)(millis() - time_before_wifi_begin));
  IPAddress ip = WiFi.localIP();
  printk("IP Address: %s\n", ip.toString().c_str());

#endif
  if (g_weather_city.equals("")) {
    g_weather_city = "Disneyland";
    g_weather_time_zone = "";
  }
  appState = (g_weather_time_zone.length() == 0) ? FETCH_MAP_CITY_TO_LOCATION : FETCH_CURRENT;

#ifdef USE_GT911_TOUCH
  if (ts.begin()) {
    printk("Touch controller init - OK\n");
  } else {
    printk("Touch controller init - FAILED\n");
    while (1)
      ;
  }

#endif
  // Initialize the touch screen mapping...
  //GigaDisplayTouchMap::setScreeninfo(1, tft.width(), tft.height());

  MyKeyboard.init(COLOR_BLACK, COLOR_WHITE, COLOR_BLUE, COLOR_DARKGREY, COLOR_DARKGREY, COLOR_NAVY, COLOR_BLACK, &FONT_BUTTON);
  // optional methods
  // max input characters is controlled by in the .h file
  // #define MAX_KEYBOARD_CHARS 18
  // change input display color
  MyKeyboard.setDisplayColor(COLOR_WHITE, COLOR_BLUE);

  // Lets try to see if we have a sparkfun Q button.
  printk("Call qbutton.begin()\n");
  Wire.begin();  // warning will Fault the processor if we call qbutton.begin with wire without this call
  qbutton_found = qbutton.begin(SFE_QWIIC_BUTTON_DEFAULT_ADDRESS, Wire);
  printk("QButton begin: %u\n", qbutton_found);
  if (qbutton_found) qbutton.LEDon(64);

  // and keypad while we are at it...
  qkeypad_found = qkeypad.begin();
  printk("Qkeypad begin: %u\n", qkeypad_found);
}

#ifndef USE_ETHERNET
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  printk("SSID: %s\n", WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  printk("IP Address: %s\n", ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  printk("signal strength (RSSI): %s dBm\n", rssi);
}
#endif

void loop() {
  if (qbutton_found) {
    if (qbutton.hasBeenClicked()) {
      tft.fillScreen(COLOR_BG);
      SetTFTFont(&FreeSans10pt7b);
      tft.setTextColor(COLOR_YELLOW);
      tft.setCursor(20, 20);
      tft.println("QButton pressed - try OTA");
      if (!TryOTAUpdate(client, __FILE__, &tft, false)) {
        tft.fillScreen(COLOR_BG);
      }
      qbutton.clearEventBits();
    }
  }
  if (qkeypad_found) {
    qkeypad.updateFIFO();  // necessary for keypad to pull button from stack to readable register
    char button = qkeypad.getButton();
    switch (button) {
      case '*': 
        showKeyboard();
        break;
      case '0':
         showMainDashboard();
         break;
        
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
        showDayDetailScreen(button - '1');
        break;
    }
  }

  switch (appState) {
    case FETCH_MAP_CITY_TO_LOCATION:
      printk("\n[0/3] Map City to Location...\n");
      if (sendMapCityRequest()) {
        appState = READ_MAP_CITY_TO_LOCATION;
      } else {
        printk("Connection failed.\n");
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
      printk("\n[1/3] Requesting Current Weather...\n");
      if (sendCurrentRequest()) {
        appState = READ_CURRENT;
      } else {
        printk("Connection failed.\n");
        appState = DONE_APP;
      }
      break;

    case READ_CURRENT:
      if (processIncomingStream(printCurrentData)) {
        appState = FETCH_DAILY;  //was FETCH_HOURLY
      }
      break;

    case FETCH_HOURLY:
      printk("\n[2/3] Requesting Hourly Forecast...\n");
      if (sendHourlyRequest()) {
        appState = READ_HOURLY;
      } else {
        printk("Connection failed.\n");
        appState = DONE_APP;
      }
      break;

    case READ_HOURLY:
      if (processIncomingStream(printHourlyData)) {
        appState = FETCH_DAILY;
      }
      break;

    case FETCH_DAILY:
      printk("\n[3/3] Requesting Daily Forecast...\n");
      if (sendDailyRequest()) {
        appState = READ_DAILY;
      } else {
        printk("Connection failed.\n");
        appState = DONE_APP;
      }
      break;

    case READ_DAILY:
      if (processIncomingStream(printDailyData)) {
        appState = FETCH_AIR_QUALITY;
      }
      break;

    case FETCH_AIR_QUALITY:
      printk("\n[4/4] Requesting Air Quality...\n");
      if (sendAirQualityRequest()) {
        appState = READ_AIR_QUALITY;
      } else {
        printk("Connection failed.\n");
        appState = DONE_APP;
      }
      break;

    case READ_AIR_QUALITY:
      if (processIncomingStream(printAirQualityData)) {
        drawWeatherDashboard();
        printk("All data successfully fetched!\n");
        printk("Enter City name:\n");
        appState = DONE_APP;
        g_last_cycle_time_ms = millis();
      }
      break;
    case DONE_APP:
      break;

    default:
      break;
  }


  if (Serial.available()) {
    g_weather_city = Serial.readString();
    g_weather_city.trim();
    g_weather_city.replace(' ', '+');
    printk("New City: %s\n", g_weather_city.c_str());
    appState = FETCH_MAP_CITY_TO_LOCATION;
  }

  // see if we timed out and should start a new read cycle
  uint32_t delta_time = millis() - g_last_cycle_time_ms;
  if (g_weather_cycle_time_ms < (5 * 60 * 1000)) g_weather_cycle_time_ms = 5 * 60 * 1000;
  if (delta_time > g_weather_cycle_time_ms) {
    printk("\n*** Start new read cycle ***\n");
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

