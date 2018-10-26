#ifndef BUTTON_STATE_H
#define BUTTON_STATE_H

#include <stdint.h>

enum BUTTON_STATE {kNotPressed = 0b00, kJustPressed = 0b01, kPressed = 0b11, kJustReleased = 0b10};
enum BUTTON_NAME {BTN_CN0, BTN_CN1, BTN_CN8};

enum BUTTON_STATE get_button_state(enum BUTTON_NAME, unsigned char);

#endif
