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

  Warning:
  Note: This is a version hacked up by KurtE to run on Arduino GIGA display Shield
  and maybe other boards based off of Adafruit GFX library

*/

#include <Arduino.h>
#include "GFX_Keyboard.h"
#include "USBCVideo_GFX.h"

//#define debug

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// keyboard

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Keyboard::Keyboard(USBCVideo_GFX *Display, KEYPAD *qkeypad) {
  d = Display;
  k = qkeypad;
}
void Keyboard::init(uint16_t BackColor, uint16_t TextColor, uint16_t ButtonColor, uint16_t BorderColor, uint16_t PressedTextColor, uint16_t PressedButtonColor, uint16_t PressedBorderColor, const GFXfont *ButtonFont) {
  kcolor = BackColor;
  tcolor = TextColor;
  bcolor = ButtonColor;
  rcolor = BorderColor;
  inputt = BackColor;
  inputb = TextColor;
  ptcolor = PressedTextColor;
  pbcolor = PressedButtonColor;
  brcolor = PressedBorderColor;
  bfont = ButtonFont;
  clickpin = -1;
  rad = 0;
  Size = BUTTON_SIZE;
  _key_mod = MOD_NORMAL;
}


void Keyboard::setDisplayColor(uint16_t TextColor, uint16_t BackColor) {
  inputt = TextColor;
  inputb = BackColor;
}

//void Keyboard::BuildButton(BUTTON *temp, int Col, int Row) {
//  temp->col = Col;
//  temp->row = Row;
//  temp->w = 1;
//}

void Keyboard::BuildButton(BUTTON *temp, int Row, int Col, signed char ch, signed char sp, uint8_t Wide) {
  temp->col = Col;
  temp->row = Row;
  temp->ch = ch;
  temp->ch_spec = sp;
  temp->w = Wide;
}


static const char *special_buttons_text[] = {
  "Aa", "Space", "#$%^", "Done", "Cancel", "Back"
};

void Keyboard::DrawButton(BUTTON *temp, uint8_t State) {

  int button_ch = temp->ch;
  if (_key_mod == MOD_SPECIAL_CASE) {
    button_ch = temp->ch_spec;
  } else if (_key_mod == MOD_UPCASE) {
    if ((button_ch >= 'a') && (button_ch <= 'z')) button_ch = (button_ch - 'a') + 'A';
  }
  if (button_ch == 0) return;  // Don't draw this one

  if (State == BUTTON_PRESSED) {
    if (rad > 0) {
      d->fillRoundRect(Cols[temp->col], Rows[temp->row], Size + ((temp->w - 1) * (2 + Size)), Size, rad, pbcolor);
      d->drawRoundRect(Cols[temp->col], Rows[temp->row], Size + ((temp->w - 1) * (2 + Size)), Size, rad, brcolor);
    } else {
      d->fillRect(Cols[temp->col], Rows[temp->row], Size + ((temp->w - 1) * (2 + Size)), Size, pbcolor);
      d->drawRect(Cols[temp->col], Rows[temp->row], Size + ((temp->w - 1) * (2 + Size)), Size, brcolor);
    }
    d->setTextColor(ptcolor, bcolor);
  } else {
    if (rad > 0) {
      d->fillRoundRect(Cols[temp->col], Rows[temp->row], Size + ((temp->w - 1) * (2 + Size)), Size, rad, bcolor);
      d->drawRoundRect(Cols[temp->col], Rows[temp->row], Size + ((temp->w - 1) * (2 + Size)), Size, rad, rcolor);
    } else {
      d->fillRect(Cols[temp->col], Rows[temp->row], Size + ((temp->w - 1) * (2 + Size)), Size, bcolor);
      d->drawRect(Cols[temp->col], Rows[temp->row], Size + ((temp->w - 1) * (2 + Size)), Size, rcolor);
    }
    d->setTextColor(tcolor, bcolor);
  }

  d->setFont(bfont);
  if (button_ch < 0) {
    const char *szText = special_buttons_text[-1 - button_ch];
    d->getTextBounds(szText, 0, 0, &xT, &yT, &wT, &hT);
    xm = wT;  // returns inconsistent values
    ym = hT;
    //Serial.print(ym); Serial.print(" "); Serial.println(Rows[temp->row]);
    d->setCursor(Cols[temp->col] + ((4 * Size) - xm) / 2, Rows[temp->row] + (Size - ym) + ym / 4);
    d->print(szText);

  } else {
    d->getTextBounds("0", 0, 0, &xT, &yT, &wT, &hT);
    xm = wT;  // returns inconsistent values
    ym = hT;
    d->setCursor(Cols[temp->col] + (Size - xm) / 2, Rows[temp->row] + (Size - ym) + ym / 4);
    d->print((char)button_ch);
  }
}

bool Keyboard::Pressed(BUTTON *temp, uint8_t ASCII) {
  return false;
}

void Keyboard::DisplayInput() {
  d->getTextBounds("A", 0, 0, &xT, &yT, &wT, &hT);

  ym = hT;

  d->fillRect(Col1, Row0, Col10 + (2 * 9), Size, inputt);
  d->setCursor(Col1 + 5, Row0 + (Size - ym));
  d->setFont(bfont);
  d->setTextColor(inputb, inputt);

  if (hideinput) {
    d->print(hc);
  } else {
    d->print(dn);
  }
}

void Keyboard::hideInput() {

  hideinput = true;
}


void Keyboard::setCornerRadius(uint8_t Radius) {
  rad = Radius;
  if (Radius > 10) {
    rad = 10;
  }
}

void Keyboard::getInput() {

  uint8_t np = 0;
  uint16_t b = 0, i = 0;
  CapsLock = false;
  bool SpecialChar = false;
  bool KeepIn = true;
  bool found = false;
  memset(dn, '\0', MAX_KEYBOARD_CHARS + 2);
  memset(hc, '\0', MAX_KEYBOARD_CHARS + 2);
  _selected_button = -1;

  strcpy(dn, data);

  if (strlen(data) > 0) {
    strcpy(dn, data);
    np = strlen(dn);
  }

  if (hideinput) {
    for (i = 0; i < strlen(dn); i++) {
      hc[i] = '*';
    }
  }


#define BB(row, col, cc, cs, nc) BuildButton(&Buttons[row * 10 + col], row + 1, col, cc, cs, nc)

  BB(0, 0, '0', '!', 1);
  BB(0, 1, '1', '@', 1);
  BB(0, 2, '2', '#', 1);
  BB(0, 3, '3', '$', 1);
  BB(0, 4, '4', '%', 1);
  BB(0, 5, '5', '^', 1);
  BB(0, 6, '6', '&', 1);
  BB(0, 7, '7', '*', 1);
  BB(0, 8, '8', '(', 1);
  BB(0, 9, '9', ')', 1);
  BB(1, 0, 'q', '`', 1);
  BB(1, 1, 'w', '~', 1);
  BB(1, 2, 'e', '-', 1);
  BB(1, 3, 'r', '_', 1);
  BB(1, 4, 't', '=', 1);
  BB(1, 5, 'y', '+', 1);
  BB(1, 6, 'u', '[', 1);
  BB(1, 7, 'i', ']', 1);
  BB(1, 8, 'o', '{', 1);
  BB(1, 9, 'p', '}', 1);
  BB(2, 0, 0, '\\', 1);
  BB(2, 1, 'a', '|', 1);
  BB(2, 2, 's', ';', 1);
  BB(2, 3, 'd', ':', 1);
  BB(2, 4, 'f', '\'', 1);
  BB(2, 5, 'g', '\"', 1);
  BB(2, 6, 'h', ',', 1);
  BB(2, 7, 'j', '.', 1);
  BB(2, 8, 'k', '<', 1);
  BB(2, 9, 'l', '>', 1);
  BB(3, 0, 0, 0, 1);
  BB(3, 1, 'z', 0, 1);
  BB(3, 2, 'x', 0, 1);
  BB(3, 3, 'c', '/', 1);
  BB(3, 4, 'v', '?', 1);
  BB(3, 5, 'b', 0, 1);
  BB(3, 6, 'n', 0, 1);
  BB(3, 7, 'm', 0, 1);

  // Special Buttons
  BuildButton(&Buttons[38], 5, 0, -1, -1, 3);  // Aa
  BuildButton(&Buttons[39], 5, 3, -2, -2, 4);  // Space
  BuildButton(&Buttons[40], 5, 7, -3, -3, 3);  // #$%^
  BuildButton(&Buttons[41], 6, 0, -4, -4, 3);  // Done
  BuildButton(&Buttons[42], 6, 3, -5, -5, 4);  // Cancel
  BuildButton(&Buttons[43], 6, 7, -6, -6, 3);  // Back


  d->startBuffering();
  DisplayInput();

  if (hasinittext) {
    d->fillRect(Col1, Row0, Col10 + (2 * 9), Size, inputt);
    d->setCursor(Col1 + 5, Row0 + 6);
    d->setFont(bfont);
    d->setTextColor(inputb, inputt);
    Serial.println(inittext);
    d->print(inittext);
  }

  // Now lets draw all of the Buttons
  // draw numbers
  for (i = 0; i < CNT_BUTTONS; i++) {
    DrawButton(&Buttons[i], BUTTON_RELEASED);
  }

  d->endBuffering();

  // lets start of selecting row1, col1...
  selectButton(0);

  while (KeepIn) {
    // kets first try sparkfun keypad to see if we can move around in the keyboard object.
    k->updateFIFO();  // necessary for keypad to pull button from stack to readable register
    char button = k->getButton();
    switch (button) {
      case '*': KeepIn = false; break;  // return from the keypad.
      case '#':
        {
          uint8_t keymod = getKeyMod();
          setKeyMod((keymod == 2) ? 0 : keymod + 1);
          break;
        }
      case '5':  // Process the selected character
        {
          int button_ch = Buttons[_selected_button].ch;
          if (_key_mod == MOD_SPECIAL_CASE) {
            button_ch = Buttons[_selected_button].ch_spec;
          } else if (_key_mod == MOD_UPCASE) {
            if ((button_ch >= 'a') && (button_ch <= 'z')) button_ch = (button_ch - 'a') + 'A';
          }
          bool input_updated = false;
          switch (button_ch) {
            // one of the command buttons:
            case -1:  // Aa
              setKeyMod((getKeyMod() == MOD_NORMAL) ? MOD_UPCASE : MOD_NORMAL);
              break;
            case -2:  // Space
              if (np < MAX_KEYBOARD_CHARS) {
                input_updated = true;
                dn[np] = ' ';
                hc[np] = '*';
                np++;
                break;
              }
              break;

            case -3:  // #$%^
              setKeyMod((getKeyMod() == MOD_SPECIAL_CASE) ? MOD_NORMAL : MOD_SPECIAL_CASE);
              break;

            case -4:  // Done
              dn[np] = '\0';
              strcpy(data, dn);
              KeepIn = false;
              break;

            case -5:  //Cancel
              KeepIn = false;
              break;
            case -6:  // Backspace
              if (np > 0) {
                input_updated = true;
                --np;
                dn[np] = '\0';
                hc[np] = '\0';
              }
              DisplayInput();
              break;
            default:
              if (np < MAX_KEYBOARD_CHARS) {
                input_updated = true;
                dn[np] = button_ch;
                hc[np] = '*';
                np++;
                break;
              }
          }
          if (input_updated) {
            d->startBuffering();
            DisplayInput();
            d->endBuffering();
          }
        }
        break;
      case '2':  // up
        selectButton(SELECT_UP);
        break;
      case '8':  // down
        selectButton(SELECT_DOWN);
        break;
      case '4':  // left
        selectButton(SELECT_LEFT);
        break;
      case '6':  // right
        selectButton(SELECT_RIGHT);
        break;
    }

    delay(10);
  }
}

// simple helper function to deal with row and col wrap around....
//int Keyboard::mapRowColToIndex(int row, int col) {
//}

void Keyboard::selectButton(int button_index) {
  // remember which one was selected
  if (button_index < 0) {
    if (_selected_button < 0) {
      button_index = (button_index == SELECT_NONE) ? -1 : 0;
    } else {
      switch (button_index) {
        case SELECT_UP:
          {
            // These are quick and dirty
            int row = Buttons[_selected_button].row - 1;
            int col = Buttons[_selected_button].col;
            if (row < 1) row = 6;

            if ((row >= 1) && (row <= 4)) {
              button_index = (row - 1) * 10 + col;
              // Try to find one to right in same row, else try to left
              if (((_key_mod == MOD_SPECIAL_CASE) ? Buttons[button_index].ch_spec : Buttons[button_index].ch) == 0) {
                if (col < 3) {
                  while (((_key_mod == MOD_SPECIAL_CASE) ? Buttons[button_index].ch_spec : Buttons[button_index].ch) == 0) {
                    button_index++;
                  }
                } else {
                  while (((_key_mod == MOD_SPECIAL_CASE) ? Buttons[button_index].ch_spec : Buttons[button_index].ch) == 0) {
                    button_index--;
                  }
                }
              }
            } else {
              if (row == 5) button_index = CNT_TEXT_BUTTONS;
              else button_index = CNT_TEXT_BUTTONS + 3;
              if (col >= 7) button_index += 2;
              else if (col >= 3) button_index++;
            }
          }
          break;
        case SELECT_DOWN:
          {
            // These are quick and dirty
            int row = Buttons[_selected_button].row + 1;
            int col = Buttons[_selected_button].col;
            if (row > 6) row = 1;

            if ((row >= 1) && (row <= 4)) {
              button_index = (row - 1) * 10 + col;
              // Try to find one to right in same row, else try to left
              if (((_key_mod == MOD_SPECIAL_CASE) ? Buttons[button_index].ch_spec : Buttons[button_index].ch) == 0) {
                if (col < 3) {
                  while (((_key_mod == MOD_SPECIAL_CASE) ? Buttons[button_index].ch_spec : Buttons[button_index].ch) == 0) {
                    button_index++;
                  }
                } else {
                  while (((_key_mod == MOD_SPECIAL_CASE) ? Buttons[button_index].ch_spec : Buttons[button_index].ch) == 0) {
                    button_index--;
                  }
                }
              }
            } else {
              if (row == 5) button_index = CNT_TEXT_BUTTONS;
              else button_index = CNT_TEXT_BUTTONS + 3;
              if (col >= 7) button_index += 2;
              else if (col >= 3) button_index++;
            }
          }
          break;
        case SELECT_LEFT:
          {
            int button_ch;
            button_index = _selected_button;
            do {
              button_index--;
              if (button_index < 0) button_index = CNT_BUTTONS - 1;
              button_ch = (_key_mod == MOD_SPECIAL_CASE) ? Buttons[button_index].ch_spec : Buttons[button_index].ch;
            } while (button_ch == 0);
          }
          break;
        case SELECT_RIGHT:
          {
            int button_ch;
            button_index = _selected_button;
            do {
              button_index++;
              if (button_index >= CNT_BUTTONS) button_index = 0;
              button_ch = (_key_mod == MOD_SPECIAL_CASE) ? Buttons[button_index].ch_spec : Buttons[button_index].ch;
            } while (button_ch == 0);
          }
          break;
        case SELECT_NONE:
          button_index = -1;
          break;
      }
    }
  }

  // button did not change so nothing to do.
  if (button_index == _selected_button) return;

  d->startBuffering();
  if (_selected_button != -1) DrawButton(&Buttons[_selected_button], BUTTON_RELEASED);
  if (button_index != -1) DrawButton(&Buttons[button_index], BUTTON_PRESSED);
  d->endBuffering();
  _selected_button = button_index;
}

void Keyboard::setInitialText(const char *Text) {

  uint8_t i;

  for (i = 0; i < (MAX_KEYBOARD_CHARS); i++) {
    inittext[i] = Text[i];
  }
  hasinittext = true;
  Serial.println(inittext);
}

void Keyboard::setClickPin(int Value) {

  clickpin = Value;
}


uint8_t Keyboard::mapXYToButtonindex() {
  // first compute row.
  int btn_index;
  if ((BtnY < Row0) || (BtnX < Col1)) return 0xff;
  int row = (BtnY - Row0) / ROW_DELTA;
  int col = (BtnX - Col1) / ROW_DELTA;

  if ((row > 6) || (col > 9)) return 0xff;

  // Normal rows have 10 btns defined...
  if (row < 5) {
    btn_index = (row - 1) * 10 + col;
    if (btn_index >= CNT_TEXT_BUTTONS) return 0xff;
  } else {
    // hack up the last two rows.
    btn_index = CNT_TEXT_BUTTONS + (row - 4) * 3;
    if (col >= 7) btn_index += 2;
    else if (col >= 3) btn_index++;
  }

  // now see if that button index is an actual active button.
  return btn_index;
}

void Keyboard::setKeyMod(uint8_t key_mod) {
  if (key_mod != _key_mod) {
    _key_mod = key_mod;

    d->startBuffering();

    // clear out space of buttons that may be there or not there
    d->fillRect(Col1, Row1, Col10 + Size, 4 * (Size + 2), kcolor);

    DisplayInput();

    if (hasinittext) {
      d->fillRect(Col1, Row0, Col10 + (2 * 9), Size, inputt);
      d->setCursor(Col1 + 5, Row0 + 6);
      d->setFont(bfont);
      d->setTextColor(inputb, inputt);
      Serial.println(inittext);
      d->print(inittext);
    }


    // Now lets draw all of the Buttons
    // draw numbers
    for (int i = 0; i < CNT_BUTTONS; i++) {
      DrawButton(&Buttons[i], (i == _selected_button)? BUTTON_PRESSED : BUTTON_RELEASED);
    }
    d->endBuffering();
  }
}
