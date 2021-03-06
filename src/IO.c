// IO.c

// libraries & header
#include "IO.h"
#include "libpic30.h"
#include "xc.h"

// project files
#include "button_state.h" // state machine
#include "ChangeClk.h"
#include "IR.h"
#include "Timer.h"
#include "UART2.h" // for testing / debugging only

// Magic Numbers
static const char kSetPinToInput = 1;
static const char kSetPinToOutput = 0;
static const char kPullUpMode = 1;
static const char kEnable = 1;
static const char kDisable = 0;
static const char INTERNAL = 0;
static const float kMagicNumber = 1.0/2.0; // for the timers, processor specific :p

// statics
static char btn_verbose_mode = 0;
static char xmit_mode = 1;
static volatile unsigned char bothButtonsPushed = 0;
static volatile char power_is_on = 0;



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

static void init_CN8(void) {
        // note: pin 14 == RA6 == CN8
        TRISAbits.TRISA6 = kSetPinToInput; // configure IO pin 8/CN8 to be input
        CNPU1bits.CN8PUE = kPullUpMode; // set pin to be 'pull up' (ie, connected to ground)
        CNEN1bits.CN8IE = kEnable; // enable CN8 interrupts
}

static inline void set_timer_priority(int priority) {
        if (priority >= 0 && priority <= 7) {
                IPC2bits.T3IP = priority;
        }
}

static inline void init_timer3(int frequency) {
        // basic configuration
        NewClk(frequency); // Switch clock: 32 for 32kHz, 500 for 500 kHz, 8 for 8MHz

        T3CONbits.TCKPS = kDisable; // set pre-scaler (divides timer speed by );
        T3CONbits.TCS = INTERNAL; // use internal clock (ie, not external)
        T3CONbits.TSIDL = kDisable; // one could stop timer when processor idles
        T3CONbits.TGATE = kDisable; // one could replace interrupts with an accumulator

        set_timer_priority(5); // sets priority to 5

        TMR3 = 0; // just in case it doesn't happen automatically
}

static void delay_us_t3(uint16_t us) {
        init_timer3(8);

        long int clock_freq = 8000000; // recall ints are 16 bit on this chip
        float prescaling_factor = 1.0;
        // pr is period of the timer before an interrupt occurs
        // note: pr2 is 16 bit unless we're combining timers
        PR3 = (clock_freq / US_PER_S) * prescaling_factor * kMagicNumber * us;

        // start the timer & enable the interrupt
        T3CONbits.TON = kEnable; // starts the timer
        IEC0bits.T3IE = kEnable; // enable the interrup
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
//        init_CN8();

        IFS1bits.CNIF = 0; // clear interrupt flag if it isn't already
        IPC4bits.CNIP = 0b101; // set CN interrupt to priority 5
        IEC1bits.CNIE = kEnable; // enable CN interrupts in general
}

extern volatile int countTarget;
static int countButton = 0;



// *********************************************************** interrupt handler
void __attribute__((interrupt, no_auto_psv)) _CNInterrupt(void) {
        IEC1bits.CNIE = kDisable; // disable CN interrupts in general
        IFS1bits.CNIF = 0; // clear interrupt flag

        /**
        * Debounce counter:
        * 20 is calculated by using numbers from calculating clock
        * to 1/2 is from the magic number var, 1/10 is from how we
        * want 1/10 seconds length
        */
        __delay32(16000/20);

        //check the state of the buttons
        unsigned char CN0_just_pressed = get_button_state(BTN_CN0, PORTAbits.RA4==0) == kJustPressed;
        unsigned char CN1_just_pressed = get_button_state(BTN_CN1, PORTBbits.RB4==0) == kJustPressed;

        //count number of button being pressed
        if(CN0_just_pressed) {
                countButton++;
        }
        //set the global var countTarget to count button
        else if (CN1_just_pressed){
                countTarget = countButton;
                Disp2Hex(countTarget);      //display the mount of times freq. is divided
                countButton = 0;            //resets the push button count
        }

/**************************DON'T DELETE - Assignment 3*************************/
//    unsigned char CN0_just_pressed = get_button_state(BTN_CN0, PORTAbits.RA4==0) == kJustPressed;
//    unsigned char CN1_just_pressed = get_button_state(BTN_CN1, PORTBbits.RB4==0) == kJustPressed;
//    unsigned char CN8_just_pressed = get_button_state(BTN_CN8, PORTAbits.RA6==0) == kJustPressed;
//
//    if(CN0_just_pressed && CN1_just_pressed) {
//            if (btn_verbose_mode) {
//                    Disp2String("\n\rBoth buttons pressed!");
//            }
//            if (power_is_on) {
//                xmit_mode = !xmit_mode;
//            } else {
//                    bothButtonsPushed = 1;
//            }
//    } else if (CN1_just_pressed) {
//            if (btn_verbose_mode) {
//                    Disp2String("\n\rRB4/CN1 pressed!");
//            }
//            if (xmit_mode) {
//                    xmit_samsung_signal(kChannelUpBits);
//            } else {
//                    xmit_samsung_signal(kVolumeUpBits);
//            }
//    } else if (CN0_just_pressed) {
//            if (btn_verbose_mode) {
//                    Disp2String("\n\rRA4/CN0 pressed!");
//            }
//            if (xmit_mode) {
//                    xmit_samsung_signal(kChannelDownBits);
//            } else {
//                    xmit_samsung_signal(kVolumeDownBits);
//            }
//    } else {
//            // for future implementation
//    }
//
//    if(CN8_just_pressed){
//        if(power_is_on){
//            __delay32(4000000*3);
//            if(get_button_state(BTN_CN8, PORTAbits.RA6==0) == kPressed){
//                xmit_samsung_signal(kPowerToggleBits);
//                power_is_on = 0;
//            }
//        }
//        else{
//            xmit_samsung_signal(kPowerToggleBits);
//            power_is_on = 1;
//        }
//    }
/*********************************DON'T DELETE********************************/


        // re-enable CN interrupts
        IEC1bits.CNIE = kEnable;

        // bothButtonsPushed = 0;
}
