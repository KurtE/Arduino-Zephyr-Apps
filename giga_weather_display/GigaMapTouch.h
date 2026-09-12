#pragma once

class GigaDisplayTouchMap {
public:
  static void setScreeninfo(int rot, int width, int height);
  static void mapTouchPoint(int xRaw, int yRaw, volatile int &touch_x, volatile int &touch_y);


  static int s_rotation;
  static int s_screen_width;
  static int s_screen_height;
  static bool s_alt_config;
};

//GigaDisplayTouchMap display_map_touch;