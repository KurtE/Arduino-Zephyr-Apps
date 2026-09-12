#include <Arduino.h>
#include "GigaMapTouch.h"
#include <Wire.h>
int GigaDisplayTouchMap::s_rotation = 1;
int GigaDisplayTouchMap::s_screen_width = 800;
int GigaDisplayTouchMap::s_screen_height = 480;
bool GigaDisplayTouchMap::s_alt_config = false;


void GigaDisplayTouchMap::setScreeninfo(int rot, int width, int height) {
  s_rotation = rot;
  s_screen_width = width;
  s_screen_height = height;

// check to see what version of Display we have.
// Assume Wire1 and address 0x5D
#define _addr 0x5D
#define GT911_REG_CONFIG_MODULE_SWITCH1 0x804D

  uint8_t status = 0;
  Wire1.beginTransmission(_addr);
  Wire1.write(GT911_REG_CONFIG_MODULE_SWITCH1 >> 8);   /* Register H */
  Wire1.write(GT911_REG_CONFIG_MODULE_SWITCH1 & 0xFF); /* Register L */
  status = Wire1.endTransmission();
  if (!status) {
    uint8_t switch_1;
    Wire1.requestFrom(_addr, 1);
    if (Wire1.available()) {
      switch_1 = Wire1.read();
      s_alt_config = (switch_1 & 0x40) ? false : true;
      if (s_alt_config) Serial.println("GT911 Alternate Config (Newer?)");
      else Serial.println("GT911 Oririginal config");
    }
  }
}



void GigaDisplayTouchMap::mapTouchPoint(int xRaw, int yRaw, volatile int &touch_x, volatile int &touch_y) {
  if (s_alt_config) {
    switch (s_rotation) {
      case 0:
        touch_x = xRaw;
        touch_y = yRaw;
        break;
      case 1:
        touch_x = yRaw;                     //s_screen_width - xRaw;
        touch_y = s_screen_height - xRaw;  // s_screen_height - yRaw;
        break;
      case 2:
        touch_x = s_screen_width - xRaw;
        touch_y = s_screen_height - yRaw;
        break;
      case 3:
        touch_x = s_screen_width - yRaw;
        touch_y = xRaw;
        break;
    }

  } else {
    switch (s_rotation) {
      case 0:
        touch_y = xRaw;
        touch_x = s_screen_width - yRaw;
        break;
      case 1:
        touch_x = xRaw;  //s_screen_width - xRaw;
        touch_y = yRaw;  // s_screen_height - yRaw;
        break;
      case 2:
        touch_x = yRaw;
        touch_y = s_screen_height - xRaw;
        break;
      case 3:
        touch_x = s_screen_width - xRaw;
        touch_y = s_screen_height - yRaw;
        break;
    }
  }
}
