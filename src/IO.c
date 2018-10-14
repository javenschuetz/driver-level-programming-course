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


// ************************************************************ helper functions
static void init_CN0(void) {
        // note: pin 10 == RB4 == CN1
        TRISAbits.TRISA4 = 1; // configure IO pin 10/CN0 to be input
        CNPU1bits.CN1PUE = 1; // set pin to be 'pull up' (ie, connected to ground)
        CNEN1bits.CN0IE = 1; // enable CN1 interrupts
}

static void init_CN1(void) {
        // note: pin 9 == RA4 == CN0
        TRISBbits.TRISB4 = 1; // configure IO pin 9/CN1 to be input
        CNPU1bits.CN0PUE = 1; // set pin to be 'pull up' (ie, connected to ground)
        CNEN1bits.CN1IE = 1; // enable CN0 interrupts
}

// ************************************************************** core functions
void CN_init(void) {
        init_CN0();
        init_CN1();

        IFS1bits.CNIF = 0; // clear interrupt flag if it isn't already
        IPC4bits.CNIP = 0b101; // set CN interrupt to priority 5
        IEC1bits.CNIE = 1; // enable CN interrupts in general
}

// *********************************************************** interrupt handler
void __attribute__((interrupt, no_auto_psv)) _CNInterrupt(void)
{
        // we need to debounce, and we can simulaneously use this dead period
        // to check for double-pressing

        IEC1bits.CNIE = 0; // disable CN interrupts in general
        IFS1bits.CNIF = 0; // clear interrupt flag
        delay_ms(50);
        Idle(); // note: we're relying on timer to wake us up, but anything could wake us!

        // TODO - fix LED wrt to this delay_ms mess

        unsigned char CN0_just_pressed = get_button_state(BTN_CN0, PORTAbits.RA4==0) == kJustPressed;
        unsigned char CN1_just_pressed = get_button_state(BTN_CN1, PORTBbits.RB4==0) == kJustPressed;

        if(CN0_just_pressed && CN1_just_pressed) {
                Disp2String("\n\rBoth buttons pressed!");
        } else if (CN1_just_pressed) {
                Disp2String("\n\rRB4/CN1 pressed!");
        } else if (CN0_just_pressed) {
                Disp2String("\n\rRA4/CN0 pressed!");
        } else {
                // for future implementation
        }

        // re-enable CN interrupts
        IEC1bits.CNIE = 1;
}
