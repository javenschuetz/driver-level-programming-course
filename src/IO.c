// IO.c

// libraries & header
#include "xc.h"
#include "IO.h"

// for debounce
#include "Timer.h"

// other
#include "UART2.h" // for testing / debugging only
#include "button_state.h" // state machine

// Magic Numbers
static const char kSetPinToInput = 1;
static const char kSetPinToOutput = 0;
static const char kPullUpMode = 1;
static const char kEnable = 1;
static const char kDisable = 0;

// statics
static char btn_verbose_mode = 0;

// ************************************************************ helper functions
static void init_CN0(void) {
        // note: pin 10 == RB4 == CN1
        TRISAbits.TRISA4 = kSetPinToInput; // configure IO pin 10/CN0 to be input
        CNPU1bits.CN1PUE = kPullUpMode; // set pin to be 'pull up' (ie, connected to ground)
        CNEN1bits.CN0IE = kEnable; // enable CN1 interrupts
}

static void init_CN1(void) {
        // note: pin 9 == RA4 == CN0
        TRISBbits.TRISB4 = kSetPinToInput; // configure IO pin 9/CN1 to be input
        CNPU1bits.CN0PUE = kPullUpMode; // set pin to be 'pull up' (ie, connected to ground)
        CNEN1bits.CN1IE = kEnable; // enable CN0 interrupts
}

// *************************************************************** API functions
void set_btn_verbose_mode(unsigned char verbose_on) {
        if (verbose_on == 1) {
                btn_verbose_mode = 1;
        } else {
                btn_verbose_mode = 0;
        }
}

void CN_init(void) {
        init_CN0();
        init_CN1();

        IFS1bits.CNIF = 0; // clear interrupt flag if it isn't already
        IPC4bits.CNIP = 0b101; // set CN interrupt to priority 5
        IEC1bits.CNIE = kEnable; // enable CN interrupts in general
}

// *********************************************************** interrupt handler
void __attribute__((interrupt, no_auto_psv)) _CNInterrupt(void)
{
        // button was pressed. Lets debounce, then investigate

        IEC1bits.CNIE = kDisable; // disable CN interrupts in general
        IFS1bits.CNIF = 0; // clear interrupt flag
        delay_ms(50);
        Idle(); // note: we're relying on timer to wake us up, but anything could wake us!

        unsigned char CN0_just_pressed = get_button_state(BTN_CN0, PORTAbits.RA4==0) == kJustPressed;
        unsigned char CN1_just_pressed = get_button_state(BTN_CN1, PORTBbits.RB4==0) == kJustPressed;

        if(CN0_just_pressed && CN1_just_pressed) {
                if (btn_verbose_mode) {
                        Disp2String("\n\rBoth buttons pressed!");
                }
        } else if (CN1_just_pressed) {
                if (btn_verbose_mode) {
                        Disp2String("\n\rRB4/CN1 pressed!");
                }
        } else if (CN0_just_pressed) {
                if (btn_verbose_mode) {
                        Disp2String("\n\rRA4/CN0 pressed!");
                }
        } else {
                // for future implementation
        }

        // re-enable CN interrupts
        IEC1bits.CNIE = kEnable;
}
