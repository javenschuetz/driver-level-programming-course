
// libraries & header
#include "samsung_rx.h"

#include "xc.h"
#include "libpic30.h"
#include <stdint.h> // for uint32_t

// drivers
#include "UART2.h" // for testing / debugging only
#include "ChangeClk.h"
#include "timer.h"

// Magic Numbers // todo - move to shared header or use macros
static const char kSetPinToInput = 1;
static const char kPullUpMode = 1;
static const char kEnable = 1;
static const char kDisable = 0;
static const float kMagicNumber = 1.0/2.0; // for the timers, processor specific :p
static const int kPinValues = 0;
static const int kElapsedTimes = 1;

// statics
static volatile unsigned int signal_raw[200][2]; // can probably set to 68
static volatile int i = 0;

// ****************************************************************** prototypes
static void print_result(void);


// ******************************************************************** CN inits
static void init_CN0(void) {
        // note: pin 10 == RB4 == CN1
        TRISAbits.TRISA4 = kSetPinToInput; // configure IO pin 10/CN0 to be input
        // CNPU1bits.CN1PUE = kPullUpMode; // set pin to be 'pull up' (ie, connected to ground)
        CNPD1bits.CN1PDE = kPullUpMode; // set pin to be 'pull down' (ie, connected to ground)
        CNEN1bits.CN0IE = kEnable; // enable CN1 interrupts
}

void CN_init(void) {
        init_CN0();

        IFS1bits.CNIF = 0; // clear interrupt flag if it isn't already
        IPC4bits.CNIP = 3; // set CN interrupt to priority 5
        IEC1bits.CNIE = kEnable; // enable CN interrupts in general
}

// ***************************************************************** timer up
void timer_up(void) {
        Disp2Hex32(i); // debug
        XmitUART2('\r', 1);
        XmitUART2('\n', 1);
        XmitUART2('\r', 1);
        XmitUART2('\n', 1);
        print_result();
        i = 0; // due to increment below
}

// ************************************************************** process signal
static void convert_times_to_us(void) {
        int i;
        for (i = 0; i < 90; i++) {
                signal_raw[i][kElapsedTimes] = signal_raw[i][kElapsedTimes] * (1.0 / kMagicNumber) * (1.0 / 8.0); // 1/8 is us_per_s/freq
        }
}

static void print_result(void) {
        int i;
        convert_times_to_us();

        for (i = 0; i < 90; i++) {
                Disp2Hex32(signal_raw[i][kPinValues]); // signal values at interrupt time
                Disp2Hex32(signal_raw[i][kElapsedTimes]); // elapsed times before interrupt
                XmitUART2('\r', 1);
                XmitUART2('\n', 1);
        }
}

static char interpret_high_low_pattern(int low_duration, int high_duration) {
        // STUB
        //      todo
        //      - compare high to low
        //      - use 10% tolerance for timing errors
        //      - convert from cycles to ms or us
        //      - return 1 for 1, 0 for 0, 2 for start, 3 for stop, -1 for garbage
        return -1;
}

static void interpret_signal(void) {
        // step through signal array

        // for each value, check its polarity and look at time-elapsed

        // if high->low, confirm that the time-elapsed was either 4.5ms, or 0.56 ms.
        //      otherwise, we have garbage

        // the meaningful part of the pattern is low->high, high->low for all bits except for the final one.
        // the low->high marks the duration of the first half of a bit
        // the final high->low marks the duration of the second half of a bit
        //

        // if low->high, confirm that the time-elapsed was 0.56, 4.5, or 1.69
        // else, is garbage

        int i;
        for (i = 0; i < 66; i++) {
                Disp2Hex32(signal_raw[i][kElapsedTimes]);
        }
}

static void handle_CN_interrupt(uint32_t tmr2_snapshot) {
        static volatile uint32_t prev_tmr = 0;

        if (i == 0) {
                // note: 500kHz clock was too slow detect many of the transitions
                delay_us_32bit(4000000); // todo: give it watchdog functionality
                prev_tmr = 0;
        }

        if ( i < 90) {
                signal_raw[i][kPinValues] = PORTAbits.RA4==0; // have we switched to high?
                // take the difference so that we dont need to stop our pseudo watchdog
                // (catches edge cases where spurious transitions occur frequently)
                signal_raw[i][kElapsedTimes] = tmr2_snapshot - prev_tmr;
                prev_tmr = tmr2_snapshot; // this should happen ASAP, which is why its in the ISR
        } else {
                // ie, we have had more transitions than we should have!
                // print_result();
                // Disp2String("\n\rtoo many CN interrupts received!!");
                // i = -1; // due to increment below
        }

        i++;
}

// *********************************************************** interrupt handler
// in this version, we try to use CN interrupts to read the signal
void __attribute__((interrupt, no_auto_psv)) _CNInterrupt(void) {
        IFS1bits.CNIF = 0; // clear interrupt flag
        handle_CN_interrupt(TMR2);
}
