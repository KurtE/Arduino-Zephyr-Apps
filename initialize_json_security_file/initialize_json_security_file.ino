#include <ArduinoJson.h>
#include <zephyr/fs/fs.h>
#define USE_JSON_SECURITY_FILE


char g_wifi_ssid[80];
char g_wifi_password[80] = "";
String g_weather_city = "";
uint32_t g_weather_cycle_time_ms = (uint32_t)(15 * 60 * 1000);  // cycle time in MS 10 * 60 *1000;
uint32_t g_last_cycle_time_ms = 0;
String g_weather_time_zone = "";
double g_weather_latitude = 0;
double g_weather_longitude = 0;

struct WeatherData {
  // Header / Current
  String location = "Loading...";
  //...
} weather;


void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000) {}
  read_or_initialize_json_security_file();
  write_json_security_and_location_file();

}

void loop() {
  Serial.println()

}

/*************************************************/
void Serial_printf(const char *format, ...) {
  char buffer[256];
  va_list ap;
  va_start(ap, format);
  int cb_ret = vsnprintf(buffer, sizeof(buffer), format, ap);
  Serial.write(buffer, cb_ret);
}


