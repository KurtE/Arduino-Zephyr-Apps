#include <ota.h>

#define STAGING_PATH_SKETCH CONFIG_OTA_SKETCH_UPDATE_PATH CONFIG_OTA_SKETCH_TEMP_PATH_POSTFIX
#define STAGING_PATH_LOADER CONFIG_OTA_LOADER_UPDATE_PATH CONFIG_OTA_LOADER_TEMP_PATH_POSTFIX

const char *server_ota = "192.168.2.231";  //"10.10.10.10";
const uint16_t port_ota = 8000;
//char *path = "/UPDATE.bin";

static long httpGet(Client &c, const char *host, uint16_t port_ota,
                    const char *path, struct fs_file_t *out, Print *status_print) {
  if (!c.connect(host, port_ota)) {
    status_print->println("Connection failed");
    return -1;
  }

  c.print("GET ");
  c.print(path);
  c.print(" HTTP/1.1\r\nHost: ");
  c.print(host);
  c.print("\r\nConnection: close\r\n\r\n");

  char line[128];
  bool first = true;
  long content_length = -1;
  uint32_t last = millis();

  while (true) {
    if (!c.available()) {
      if (!c.connected() || millis() - last > 10000) {
        c.stop();
        return -1;
      }
      continue;
    }
    last = millis();

    size_t n = c.readBytesUntil('\n', line, sizeof(line) - 1);
    line[n] = '\0';
    if (n > 0 && line[n - 1] == '\r') {
      line[n - 1] = '\0';
    }

    if (first) {
      if (strncmp(line, "HTTP/1.", 7) != 0 || atoi(line + 9) != 200) {
        status_print->print("HTTP error: ");
        status_print->println(line);
        c.stop();
        return -1;
      }
      first = false;
      continue;
    }

    if (line[0] == '\0') {
      break;
    }

    if (strncasecmp(line, "Content-Length:", 15) == 0) {
      content_length = atol(line + 15);
    }
  }

  if (content_length <= 0) {
    status_print->println("Missing or zero Content-Length");
    c.stop();
    return -1;
  }

  uint8_t buf[512];
  long received = 0;
  last = millis();

  while (received < content_length) {
    int avail = c.available();
    if (avail <= 0) {
      if (!c.connected() || millis() - last > 10000) {
        break;
      }
      continue;
    }

    size_t want = (size_t)(content_length - received);
    if (want > sizeof(buf)) {
      want = sizeof(buf);
    }
    int got = c.read(buf, want);
    if (got <= 0) {
      continue;
    }

    if (fs_write(out, buf, got) != got) {
      status_print->println("Filesystem write failed");
      c.stop();
      return -1;
    }
    received += got;
    last = millis();
  }

  c.stop();
  return received == content_length ? received : -1;
}

bool TryOTAUpdate(ZephyrClient &client, const char *sketch_path_name, Print *status_print, bool sketch_only) {
  char path[128] = "/";

  // lets see if we can get away from having to copy our sketch to UPDATE.pin
  // The
  const char *sketch_name = strrchr(sketch_path_name, '\\');
  if (sketch_name) {
    sketch_name++;
  } else {
    sketch_name = strrchr(sketch_path_name, '/');
    if (sketch_name) {
      sketch_name++;
    } else {
      sketch_name = sketch_path_name;
    }
  }
  strcat(path, sketch_name);

  if (sketch_only) {
    status_print->println("Attempt Sketch only OAT update");
    strcat(path, ".elf-zsk.bin");
  } else {
    status_print->println("Attempt Full OTA update (loader + sketch)");
    strcat(path, ".elf-zsk.bin-bundle.bin");
  }

  const char *STAGING_PATH = sketch_only ? STAGING_PATH_SKETCH : STAGING_PATH_LOADER;

  fs_unlink(STAGING_PATH);

  struct fs_file_t file;
  fs_file_t_init(&file);
  if (fs_open(&file, STAGING_PATH, FS_O_CREATE | FS_O_WRITE) < 0) {
    status_print->println("Cannot open staging file");
    return false;
  }

  status_print->print("Downloading http://");
  status_print->print(server_ota);
  status_print->print(":");
  status_print->print(port_ota);
  status_print->println(path);
  status_print->print("Staging path: ");
  status_print->println(STAGING_PATH);
  long len = httpGet(client, server_ota, port_ota, path, &file, status_print);
  fs_close(&file);

  if (len < 0) {
    status_print->println("Download failed");
    fs_unlink(STAGING_PATH);
    return false;
  }

  status_print->print("Downloaded ");
  status_print->print(len);
  status_print->println(" bytes");

  if (sketch_only) {
    if (ota_sketch_ready() < 0) {
      status_print->println("Failed to mark sketch update as ready");
      return false;
    }
  } else {
    if (ota_loader_ready() < 0) {
      status_print->println("Failed to mark loader update as ready");
      return false;
    }
  }
  status_print->println("Rebooting...");
  status_print->flush();
  delay(100);

  if (sketch_only) {
    ota_sketch_start();
  } else {
    ota_loader_start();
  }
  return true;
}
