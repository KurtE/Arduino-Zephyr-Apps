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

#include "USBCVideo_GFX.h"
#include "SparkFun_Qwiic_Keypad_Arduino_Library.h"  //Click here to get the library: http://librarymanager/All#SparkFun_keypad

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
  struct POINTS {
    int x;
    int y;
    int contacts;
  };
  volatile POINTS p;

  Keyboard(USBCVideo_GFX *Display, KEYPAD *qkeypad);
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


  void clearInput();

  uint8_t getKeyMod() {return (_key_mod); }
  void setKeyMod(uint8_t key_mod);
private:

  // There are 46 buttons defined 0-9 Q--p, a-l, z-m... plus 6 modifier/action keys

  int16_t xT, yT;
  uint16_t wT, hT;

  struct BUTTON {
    uint8_t row;
    uint8_t col;
    uint8_t w;
    signed char ch;
    signed char ch_spec;
  };

  BUTTON Buttons[44];
  USBCVideo_GFX *d;
  KEYPAD *k;

  enum { Row0 = 24,
         Col1 = 8,
         BUTTON_SIZE = 56,
         ROW_DELTA = 58,
         CNT_BUTTONS = 44, 
         CNT_TEXT_BUTTONS = 38,
         MOD_NORMAL = 0,
         MOD_UPCASE = 1, 
         MOD_SPECIAL_CASE = 2,
         SELECT_UP = -1, 
         SELECT_DOWN = -2, 
         SELECT_LEFT = -3,
         SELECT_RIGHT = -4,
         SELECT_NONE = -5
  };

  const int Rows[7] = {
    Row0,
    Row0 + ROW_DELTA,
    Row0 + 2 * ROW_DELTA,
    Row0 + 3 * ROW_DELTA,
    Row0 + 4 * ROW_DELTA,
    Row0 + 5 * ROW_DELTA,
    Row0 + 2 + 6 * ROW_DELTA
  };
  //xpos
  const int Cols[10] = {
    Col1,
    Col1 + ROW_DELTA,
    Col1 + 2 * ROW_DELTA,
    Col1 + 3 * ROW_DELTA,
    Col1 + 4 * ROW_DELTA,
    Col1 + 5 * ROW_DELTA,
    Col1 + 6 * ROW_DELTA,
    Col1 + 7 * ROW_DELTA,
    Col1 + 8 * ROW_DELTA,
    Col1 + 9 * ROW_DELTA
  };
#define Col10 (Cols[9])
#define Row1 (Rows[1])

  uint8_t xm = 0, ym = 0;
  void selectButton(int button_index);
  int _selected_button = -1;
  uint8_t mapXYToButtonindex();
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
  const GFXfont *bfont;
  bool CapsLock = false;
  uint8_t _key_mod = MOD_NORMAL;
  void BuildButton(BUTTON *temp, int Col, int Row, signed char ch, signed char sp, uint8_t Wide=1);
  void DrawButton(BUTTON *temp, uint8_t State);
  bool Pressed(BUTTON *temp, uint8_t ASCII);
  void DisplayInput();
};


#endif
