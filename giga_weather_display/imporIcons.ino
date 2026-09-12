
// Helper to draw PNG files from SD card at (x, y) coordinates
void drawPNG(const char *filename, int x, int y) {
#if 1
    const uint16_t *image = (const uint16_t *)filename;
 #if defined(ARDUINO_GIGA) || defined(ARDUINO_PORTENTA_H7_M7)
    tft.drawRGBBitmap(x, y, image, 64, 64);
#else
    tft.writeRect(x, y, 64, 64, image);
#endif    

 
 #else
  pngX = x;
  pngY = y;

  // Optional: print image info to Serial before drawing
  //getPNGInfo(filename);

  int rc = png.open(filename, myOpen, myClose, myRead, mySeek, pngDrawCallback);
  if (rc == PNG_SUCCESS) {
    //tft.startWrite();
    png.decode(NULL, 0); // Decode and render via pngDrawCallback
    //tft.endWrite();
    png.close();
  } else {
    Serial_printf("PNG Decode Error [%d] on file: %s\n", rc, filename);
  }
 #endif  
}


void drawKeyboard(const char *filename, int x, int y) {
    const uint16_t *image = (const uint16_t *)filename;
    tft.drawRGBBitmap(x, y, image, 40, 14);
 
}