//-----------------------------------------------------------------------------
// The functions here are setup to retrieve and if necessary initialize
// the parameters needed for this sketch from a json file stored
// in the storage area of the flash.
//
// If any data changes also have function to save away the file after
// we have successfully queried the location data
//-----------------------------------------------------------------------------
#define JSON_SECURITY_FILENAME "/storage/security.jsn"
#define DEBUG_PRINT_SECURITY

void read_or_initialize_json_security_file() {
  Serial.println("\n*** Retrieving Wifi and Location settings ***");

  if ((read_json_security_file() != 0) || (strlen(g_wifi_ssid) == 0)) {
    Serial.println("Enter WiFi Information:");
    get_string("Enter Wifi SSID:", g_wifi_ssid);
    get_string("Password:", g_wifi_password);
  }

  // So far we will assume SSID and if ncess password given
  if (g_weather_city.length() == 0) {
    Serial.println("Enter Location Information:");
    char buffer[80];
    get_string("Weather city or zip:", buffer);
    g_weather_city = String(buffer);
    get_string("Weather time between samples in minutes:", buffer);
    g_weather_cycle_time_ms = atof(buffer);

    // Make sure we start by retrieving this information...
    g_weather_time_zone = "";
  }

#ifdef DEBUG_PRINT_SECURITY
  Serial_printf("g_wifi_ssid: %s\n", g_wifi_ssid);
  Serial_printf("g_wifi_password: %s\n", g_wifi_password);
  Serial_printf("g_weather_city: %s\n", g_weather_city.c_str());
  Serial_printf("weather.location: %s\n", weather.location.c_str());
  Serial_printf("g_weather_time_zone: %s\n", g_weather_time_zone.c_str());
  Serial_printf("g_weather_latitude: %f\n", g_weather_latitude);
  Serial_printf("g_weather_longitude: %f\n", g_weather_longitude);
  Serial_printf("g_weather_cycle_time_ms: %u\n", g_weather_cycle_time_ms);
#endif
}

void get_string(const char *title, char *sz) {
  while (Serial.read() != -1) {}
  int chr;
  Serial.println(title);
  while ((chr = Serial.read()) == -1) {}
  while (chr >= ' ') {
    *sz++ = chr;
    chr = Serial.read();
  }
  *sz = '\0';
}


class zephyrFileReader {
public:
  zephyrFileReader(struct fs_file_t *pfile) {
    _pfile = pfile;
  }

  // Reads one byte, or returns -1
  int read() {
    char ch;
    ssize_t cb_read = fs_read(_pfile, (void *)&ch, 1);
    return (cb_read == 1) ? ch : -1;
  }
  // Reads several bytes, returns the number of bytes read.
  size_t readBytes(char *buffer, size_t length) {
    return fs_read(_pfile, buffer, length);
  }
protected:
  struct fs_file_t *_pfile;
};


int read_json_security_file() {
  char buffer[1024];
  struct fs_file_t file;
  fs_file_t_init(&file);
  int ret;
  ret = fs_open(&file, JSON_SECURITY_FILENAME, FS_O_READ);
  if (ret < 0) {
    Serial.println("failed to open file");
    return ret;
  }


  JsonDocument doc;
#if 1
  zephyrFileReader zfsr(&file);
  DeserializationError error = deserializeJson(doc, zfsr);

  ret = fs_close(&file);
  if (ret < 0) {
    Serial.println("Failed to close file");
    return ret;
  }
#else
  ssize_t cb_read = fs_read(&file, (void *)buffer, sizeof(buffer));
  if (cb_read < 0) {
    Serial.println("Failed to read from file");
    return cb_read;
  }

  ret = fs_close(&file);
  if (ret < 0) {
    Serial.println("Failed to close file");
    return ret;
  }

  DeserializationError error = deserializeJson(doc, buffer, cb_read);
#endif
  if (error) {
    Serial.print("Failed to deserialize: ");
    Serial.println(error.c_str());
    return -1;
  }

#ifdef DEBUG_PRINT_SECURITY
  serializeJsonPretty(doc, Serial);
  Serial.println();
#endif

  const char *pssid = doc["wifi"]["ssid"];
  if (pssid) strcpy(g_wifi_ssid, pssid);

  const char *ppass = doc["wifi"]["password"];
  if (ppass) strcpy(g_wifi_password, ppass);

  const char *ploc = doc["weather"]["city"];
  if (ploc) g_weather_city = String(ploc);

  const char *pwname = doc["weather"]["name"];
  if (pwname) weather.location = String(pwname);

  const char *ptz = doc["weather"]["time_zone"];
  if (ptz) g_weather_time_zone = String(ptz);
  g_weather_latitude = doc["weather"]["lat"];
  g_weather_longitude = doc["weather"]["long"];

  double cycle_time = doc["weather"]["cycle_time"];
  g_weather_cycle_time_ms = cycle_time * 60.0 * 1000;
  return 0;
}

class zephyrFileWriter {
public:
  zephyrFileWriter(struct fs_file_t *pfile) {
    _pfile = pfile;
  }

  // Writes one byte, returns the number of bytes written (0 or 1)
  size_t write(uint8_t c) {
    return fs_write(_pfile, (void *)&c, 1);
  }
  // Writes several bytes, returns the number of bytes written
  size_t
  write(const uint8_t *buffer, size_t length) {
    return fs_write(_pfile, buffer, length);
  }
protected:
  struct fs_file_t *_pfile;
};



int write_json_security_and_location_file() {
  JsonDocument doc;
  struct fs_file_t file;
  fs_file_t_init(&file);
#if 1
  zephyrFileWriter zfsw(&file);
#else
  char buffer[1024];
#endif

  int ret;
  ret = fs_open(&file, JSON_SECURITY_FILENAME, FS_O_CREATE | FS_O_WRITE);
  if (ret < 0) {
    Serial.println("failed to create file");
    return ret;
  }


  JsonObject wifi = doc["wifi"].to<JsonObject>();
  wifi["ssid"] = g_wifi_ssid;
  wifi["password"] = g_wifi_password;

  JsonObject weather_loc = doc["weather"].to<JsonObject>();
  weather_loc["city"] = g_weather_city;
  weather_loc["name"] = weather.location;
  weather_loc["time_zone"] = g_weather_time_zone;
  weather_loc["lat"] = g_weather_latitude;
  weather_loc["long"] = g_weather_longitude;

  double cycle_time = (float)g_weather_cycle_time_ms / (60.0 * 1000);

  weather_loc["cycle_time"] = cycle_time;

#if 1
  serializeJson(doc, zfsw);
#else
  size_t len = serializeJson(doc, buffer, sizeof(buffer));


  ret = fs_write(&file, buffer, len);
  if (ret < 0) {
    Serial.println("Failed to write to file");
    return ret;
  }
#endif

  ret = fs_close(&file);
  if (ret < 0) {
    Serial.println("Failed to close file");
    return ret;
  }
  Serial.println("File written successfully\n");
  return 0;
}
