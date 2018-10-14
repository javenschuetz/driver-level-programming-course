
// libraries & header
#include <stdint.h>
#include "button_state.h"

// other
#include "UART2.h" // just for debugging

// Magic Numbers
static unsigned int CN0_val = 0;
static unsigned int CN1_val = 0;

// ************************************************************ helper functions
enum BUTTON_STATE get_CN0_state(unsigned char current_val) {
	unsigned char prev_val = CN0_val;
	unsigned char combined_val = (prev_val << 1) + current_val;
	CN0_val = current_val;

	// note: 2 bits, first is prev, second is current. Enums defined in this way in header
	switch (combined_val) {
	case kJustPressed:
		return kJustPressed;
		break;
	case kPressed:
		return kPressed;
		break;
	case kJustReleased:
		return kJustReleased;
		break;
	case kNotPressed:
		return kNotPressed;
		break;
	default:
		return 0; // should never get here
		XmitUART2('q',2); // just to give a hint something happened
	}
}

static enum BUTTON_STATE get_CN1_state(unsigned char current_val) {
	unsigned char prev_val = CN1_val;
	unsigned char combined_val = (prev_val << 1) + current_val;
	CN1_val = current_val;

	// note: 2 bits, first is prev, second is current. Enums defined in this way in header
	switch (combined_val) {
	case kJustPressed:
		return kJustPressed;
		break;
	case kPressed:
		return kPressed;
		break;
	case kJustReleased:
		return kJustReleased;
		break;
	case kNotPressed:
		return kNotPressed;
		break;
	default:
		return 0; // should never get here
		XmitUART2('r',2); // just to give a hint something happened
	}

	return kNotPressed;
}


// ************************************************************ core functions
enum BUTTON_STATE get_button_state(enum BUTTON_NAME button, unsigned char value) {
	enum BUTTON_STATE state;
	switch(button) {
	case BTN_CN0:
		state = get_CN0_state(value);
		break;
	case BTN_CN1:
		state = get_CN1_state(value);
		break;
	default:
		state = 0; // should never get here
		XmitUART2('m', 1);
	}

	return state;
}
