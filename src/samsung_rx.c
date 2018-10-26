
// libraries & header
#include "samsung_rx.h"
#include <string.h>         //string conc
#include <stdlib.h>         //string conc
#include "xc.h"
#include "libpic30.h"
#include <stdint.h> // for uint32_t

// drivers
#include "UART2.h" // for testing / debugging only
#include "ChangeClk.h"
#include "timer.h"

// values for messages
#define POWER_SWITCH 0xe0e040bf
#define CHANNEL_UP 0xe0e048b7
#define CHANNEL_DOWN 0xe0e008f7
#define VOLUME_UP 0xe0e0e01f
#define VOLUME_DOWN 0xe0e0d02f

// constants
#define LOW_MULT 0.8               //lower limit multiplicator (define used to provide const_expr)
#define HIGH_MULT 1.2              //upper limit multiplicator (define used to provide const_expr)
static const char kSetPinToInput = 1;
static const char kEnable = 1;
static const char kDisable = 0;
static const float kMagicNumber = 1.0/2.0; // for the timers, processor specific :p
static const int kPinValues = 0;
static const int kElapsedTimes = 1;

//constants to calculate limits
static const int startHigh1 = 4500.0 * LOW_MULT;
static const int startHigh2 = 4500.0 * HIGH_MULT;
static const int bitLow1 = 560.0 * LOW_MULT;
static const int bitLow2 = 560.0 * HIGH_MULT;
static const int bitHigh1 = 1600.0 * LOW_MULT;
static const int bitHigh2 = 1600.0 * HIGH_MULT;

// statics
static volatile unsigned int signal_raw[70][2]; // can probably set to 68
static volatile unsigned int raw_i = 0;
static volatile char isStarted = 0;       //started interpretation of the signal


// ****************************************************************** prototypes
static void print_result(void);


// ******************************************************************** CN inits
static void init_CN0(void) {
        // note: pin 10 == RA4 == CN0
        TRISAbits.TRISA4 = kSetPinToInput; // configure IO pin 10/CN0 to be input
        // CNPU1bits.CN1PUE = kPullUpMode; // set pin to be 'pull up' (ie, connected to ground)
        CNPD1bits.CN1PDE = kEnable; // set pin to be 'pull down' (ie, connected to ground)
        CNEN1bits.CN0IE = kEnable; // enable CN1 interrupts
}

void CN_init(void) {
        init_CN0();

        IFS1bits.CNIF = 0; // clear interrupt flag if it isn't already
        IPC4bits.CNIP = 3; // set CN interrupt to priority 3
        IEC1bits.CNIE = kEnable; // enable CN interrupts in general
}


// ******************************************************************** timer up
void timer_up(void) {
        // print 2 blank lines
        XmitUART2('\r', 1);
        XmitUART2('\n', 1);
        XmitUART2('\r', 1);
        XmitUART2('\n', 1);
        raw_i = 0;
        print_result();
}

void timer3_callback(void) {
        T2CONbits.TON = kDisable; // note: to stop the combined timer, use t2con
        IEC0bits.T3IE = kDisable; // stop the timer 2/3 combined interrupt
        TMR2 = TMR3 = 0;

        timer_up();
}


// ************************************************************** process signal
static void convert_times_to_us(void) {
        int j;
        for (j = 0; j < 90; j++) {
                signal_raw[j][kElapsedTimes] = signal_raw[j][kElapsedTimes]
                                * (1.0 / kMagicNumber) * (1.0 / 8.0); // 1/8 is us_per_s/freq
        }
}

static int interpret_high_low_pattern(int low_duration, int high_duration, int iters) {
    if (iters > 67){
        isStarted = 0;
    }

    if (isStarted == 0){
        if ((low_duration > startHigh1) && (high_duration < startHigh2)){
            isStarted = 1;
            return 2;
        } else {
            isStarted = 0;
            return -1;
        }
    }

    if ((low_duration > bitLow1) && (low_duration < bitLow2)){
        if ((high_duration > bitLow1) && (high_duration < bitLow2)){
            return 0x0;         //somehow works better this way
        }
        if ((high_duration > bitHigh1) && (high_duration < bitHigh2)){
            return 0x1;         //somehow works better this way
        }
    } else {
        return -1;
    }

    return -1;
}

static void interpret_signal(void) {
    int i;
    unsigned long int output = 0x0;
    unsigned int token = 0x0;
    int startedConv = 0;

    for (i = 1; i < 67; i= i + 2) {      //first one is unnecessary
        token = interpret_high_low_pattern(signal_raw[i][kElapsedTimes], signal_raw[i+1][kElapsedTimes], i);
        if (token == 2){
            startedConv = 1;
        }
        if (startedConv && token != 2){
            output = output ^ token;
            if (i < 65)
                output = output << 1;
        }
    }

    Disp2Hex32(output);
   switch (output){
       case POWER_SWITCH:
           Disp2String("Power Switched");
           break;
       case VOLUME_DOWN:
           Disp2String("Volume Down");
           break;
       case VOLUME_UP:
           Disp2String("Volume Up");
           break;
       case CHANNEL_DOWN:
           Disp2String("Channel Down");
           break;
       case CHANNEL_UP:
           Disp2String("Channel Up");
           break;
       default:
           Disp2String("Wrong Message");
           break;
   }
}

static void print_result(void) {
    // int i;
    convert_times_to_us();

    //sends hex values
    // for (i = 0; i < 90; i++) {
    //         Disp2Hex32(signal_raw[i][kPinValues]); // signal values at interrupt time
    //         Disp2Hex32(signal_raw[i][kElapsedTimes]); // elapsed times before interrupt
    //         XmitUART2('\r', 1);
    //         XmitUART2('\n', 1);
    // }
    interpret_signal();
}

static void handle_CN_interrupt(uint32_t tmr2_snapshot) {

        static volatile uint32_t prev_tmr;

        if (raw_i == 0) {
                // note: 500kHz clock was too slow detect many of the transitions
                delay_us_32bit(140000, timer3_callback); // todo: give it watchdog functionality
                prev_tmr = 0;
        }

        if ( raw_i < 67) {
                signal_raw[raw_i][kPinValues] = PORTAbits.RA4==0; // have we switched to high?
                // take the difference so that we don't need to stop our pseudo watchdog
                signal_raw[raw_i][kElapsedTimes] = tmr2_snapshot - prev_tmr;
                prev_tmr = tmr2_snapshot;
        } else {
                // nop
        }

        raw_i++;
}

// *********************************************************** interrupt handler
// in this version, we try to use CN interrupts to read the signal
void __attribute__((interrupt, no_auto_psv)) _CNInterrupt(void) {
        IFS1bits.CNIF = 0; // clear interrupt flag
        handle_CN_interrupt(TMR2);
}
