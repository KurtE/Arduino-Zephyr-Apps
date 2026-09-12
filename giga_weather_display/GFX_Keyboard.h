/*
  The MIT License (MIT)

  library writen by Kris Kasprzak

  Permission is hereby granted, free of charge, to any person obtaining a copy of
  this software and associated documentation files (the "Software"), to deal in
  the Software without restriction, including without limitation the rights to
  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
  the Software, and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
  FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  On a personal note, if you develop an application or product using this library
  and make millions of dollars, I'm happy for you!

	rev   date      author        change
	1.0   2/12/2023     	kasprzak      	initial code
	1.1   12/08/2024     	kasprzak      	added ability to set screen size for touch processing
	2.0   6/2025		kasprzak 	optimized keypad from button class to internal buttons to reduce memory
 	2.1   7/2025     	kasprzak 	improved text centering and implemented ability to set numpad center and button sizes

*/

// Note: This version was hacked up by KurtE to run on Arduino GIGA display Zephyr and maybe others

#ifndef GFX_KEYBOARD_H
#define GFX_KEYBOARD_H

#include "Arduino.h"
#include "Print.h"

#include "Arduino_GigaDisplay_GFX.h"
#include "Arduino_GigaDisplayTouch.h"

#define BTNS 30
#define BTNM 2

#define ILI9341_KEYPAD_BLUE 0x001F
#define ILI9341_KEYPAD_RED 0xF800
#define ILI9341_KEYPAD_GREEN 0x07E0

#define MAX_KEYBOARD_CHARS 18

#define BUTTON_PRESSED 0
#define BUTTON_RELEASED 1


class Keyboard {

public:
  struct POINTS{
    int x;
    int y;
    int contacts;
  };
 volatile POINTS p;

  Keyboard(GigaDisplay_GFX *Display, Arduino_GigaDisplayTouch *Touch);
  void init(uint16_t BackColor, uint16_t TextColor, uint16_t ButtonColor, uint16_t BorderColor,
            uint16_t PressedTextColor, uint16_t PressedButtonColor, uint16_t PressedBorderColor,
            const GFXfont *ButtonFont);


  void getInput();

  void setDisplayColor(uint16_t TextColor, uint16_t BackColor);

  void hideInput();

  void setInitialText(const char *Text);

  void setClickPin(int Value);

  void setCornerRadius(uint8_t Radius);

  void setColors(
    uint16_t BackColor,
    uint16_t TextColor,
    uint16_t ButtonColor,
    uint16_t BorderColor,
    uint16_t PressedTextColor,
    uint16_t PressedButtonColor,
    uint16_t PressedBorderColor);

  char data[MAX_KEYBOARD_CHARS + 1];

  void setTouchLimits(uint16_t ScreenLeft, uint16_t ScreenRight, uint16_t ScreenTop, uint16_t ScreenBottom);

  void clearInput();

private:

  int16_t xT, yT;
  uint16_t wT, hT;

  struct BUTTON {
    uint16_t x;
    uint16_t y;
    uint8_t w;
  };

  GigaDisplay_GFX *d;
  Arduino_GigaDisplayTouch *t;

#define BUTTON_SIZE 56
#define ROW_DELTA 58;  //size + 2
//Ypos
  const int Row0 = 24;
  const int Row1 = Row0 + ROW_DELTA ;
  const int Row2 = Row0 + 2*ROW_DELTA;
  const int Row3 = Row0 + 3*ROW_DELTA;
  const int Row4 = Row0 + 4*ROW_DELTA;
  const int Row5 = Row0 + 5*ROW_DELTA;
  const int Row6 = Row0 + 2 + 6*ROW_DELTA;

//xpos
  const int Col1 = 8;
  const int Col2 = Col1 + ROW_DELTA;
  const int Col3 = Col1 + 2*ROW_DELTA;
  const int Col4 = Col1 + 3*ROW_DELTA;
  const int Col5 = Col1 + 4*ROW_DELTA;
  const int Col6 = Col1 + 5*ROW_DELTA;
  const int Col7 = Col1 + 6*ROW_DELTA;
  const int Col8 = Col1 + 7*ROW_DELTA;
  const int Col9 = Col1 + 8*ROW_DELTA;
  const int Col10 = Col1 + 9*ROW_DELTA;

  uint8_t xm = 0, ym = 0;
  void ProcessTouch();
  char dn[MAX_KEYBOARD_CHARS + 2];
  char hc[MAX_KEYBOARD_CHARS + 2];
  char inittext[MAX_KEYBOARD_CHARS + 2];  // display initial text
  //bool ProcessButtonPress(Button TheButton);
  bool aclear = false;
  bool hideinput = false;
  bool hasinittext = false;
  int BtnX, BtnY;
  uint16_t kcolor;
  uint16_t tcolor;
  uint16_t bcolor;
  uint16_t rcolor;
  uint16_t pbcolor;
  uint16_t ptcolor;
  uint16_t brcolor;
  int16_t inputb;
  int16_t inputt;
  uint8_t rad;
  int8_t Size;
  int8_t clickpin = -1;
  int16_t screenX0, screenX320;
  int16_t screenY0, screenY240;
  const GFXfont *bfont;
  bool CapsLock = false;
  void BuildButton(BUTTON *temp, int Col, int Row);
  void BuildButton(BUTTON *temp, int Col, int Row, uint8_t Wide);
  void DrawButton(BUTTON *temp, uint8_t ASCII, uint8_t State);
  bool Pressed(BUTTON *temp, uint8_t ASCII);
  void DisplayInput();
  bool isTouched();
};


#endif
