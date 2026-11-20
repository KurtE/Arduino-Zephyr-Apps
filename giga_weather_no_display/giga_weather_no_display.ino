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
#define USE_ETHERNET

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
// SET These if you don't want to be prompted.
char g_wifi_ssid[80] = "";
char g_wifi_password[80] = "";

int keyIndex = 0;  // your network key Index number (needed only for WEP)

ZephyrClient client;


String g_weather_city = "Disneyland";
uint32_t g_weather_cycle_time_ms = (uint32_t)(15 * 60 * 1000);  // cycle time in MS 10 * 60 *1000;
uint32_t g_last_cycle_time_ms = 0;
String g_weather_time_zone = "";
double g_weather_latitude = 0;
double g_weather_longitude = 0;

/*************************************************/
void Serial_printf(const char *format, ...) {
  char buffer[256];
  va_list ap;
  va_start(ap, format);
  int cb_ret = vsnprintf(buffer, sizeof(buffer), format, ap);
  Serial.write(buffer, cb_ret);
}


/*************************************************/
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);
  while (!Serial && millis() < 4000) {}
  delay(1000); // give some time to start up after


// in Zephyr system check if Ethernet is ready before proceeding to initialize
#ifdef USE_ETHERNET
  while (Ethernet.linkStatus() != LinkON) {
    Serial.println("Waiting for link on");
    delay(100);
  }

  // start the Ethernet connection:
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin() == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
      while (true) {
        delay(1);  // do nothing, no point running without Ethernet hardware
      }
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(ip, myDns);
  } else {
    Serial.print("  DHCP assigned IP ");
    Serial.println(Ethernet.localIP());
  }

#else
  int status = WL_IDLE_STATUS;
  Serial.print("Network: ");

  if (strlen(g_wifi_ssid) == 0) {
    get_string("Enter WiFi network SSid:", g_wifi_ssid);
  }

  Serial.println(g_wifi_ssid);

  if (strlen(g_wifi_password) == 0) {
    get_string("Enter WiFi network Password:", g_wifi_password);
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
#endif
  appState = FETCH_MAP_CITY_TO_LOCATION;
}


void get_string(const char *prompt, char *str) {
  Serial.println(prompt);
  int ich;
  while ((ich = Serial.read()) == -1) {}
  while (ich >= ' ') {
    *str++ = ich;
    ich = Serial.read();
  }
  while (Serial.read() != -1) {}
  *str = 0;
}


#ifndef USE_ETHERNET
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
#endif

void loop() {
  //Serial.print("loop appstate: ");
  //Serial.println(appState);
  //Serial.flush();
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
        //write_json_security_and_location_file();
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
      //Serial.println("After printCurrentData");
      Serial.flush();
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
      Serial.flush();
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


  if (Serial.available()) {
    String input = Serial.readString();
    input.trim();
    if (input.equals("!")) {
      // lets try OTA update
      Serial.println("OTA update requested");
      TryOTAUpdate(client, __FILE__, &Serial, true);
    } else if (input.equals("#")) {
      // lets try OTA update
      Serial.println("OTA update requested");
      TryOTAUpdate(client, __FILE__, &Serial, false);
    } else {
      g_weather_city = input;
      g_weather_city.replace(' ', '+');
      Serial.print("New City: ");
      Serial.println(g_weather_city);
      appState = FETCH_MAP_CITY_TO_LOCATION;
    }
  }

  // see if we timed out and should start a new read cycle
  uint32_t delta_time = millis() - g_last_cycle_time_ms;
  if (delta_time > g_weather_cycle_time_ms) {
    Serial.println("\n*** Start new read cycle ***");
    appState = (g_weather_time_zone.length() == 0) ? FETCH_MAP_CITY_TO_LOCATION : FETCH_CURRENT;
    g_last_cycle_time_ms = millis();  // don't keep hitting this
  }
}
