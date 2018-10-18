/*
 * File:   IR.c
 * Author: Tilo
 *
 * Created on October 5, 2018, 5:29 PM
 */

#include "xc.h"
#include "IR.h"
#include "IO.h"
#include "Timer.h"
#include "ChangeClk.h"

#define FCY 4000000UL; // for __delay32()
#include "libpic30.h"

// magic numbers
static const float kMagicNumber = 1.0/2.0; // for the timers, processor specific :p
static const char kEnable = 1;
static const char kDisable = 0;
static const char kInternalClk = 0;
static const char kOutputEnable = 0;
static const char kOutputDisable = 1;

static unsigned char envelope_flag = 0;
static const int US_516 = 2240;
static const int US_13 = 52;
static const int US_1690 = 1690*4;
static const int US_4500 = 4500*4;


// ************************************************************ helper functions
static inline void set_timer_priority(int priority) {
        if (priority >= 0 && priority <= 7) {
                IPC15bits.RTCIP = priority;
        }
}

static inline void init_timer1(int frequency) {
        // basic config
        NewClk(frequency); // Switch clock: 32 for 32kHz, 500 for 500 kHz, 8 for 8MHz

        T1CONbits.TCKPS = kDisable; // set pre-scaler (divides timer speed by );
        T1CONbits.TCS = kInternalClk; // use internal clock (ie, not external)
        T1CONbits.TSIDL = kDisable; // one could stop timer when processor idles
        T1CONbits.TGATE = kDisable; // one could replace interrupts with an accumulator

        set_timer_priority(5); // sets priority to 5

        TMR1 = 0; // just in case it doesn't happen automatically
}

void delay_us_t1(uint16_t us) {
        init_timer1(8);

        long int clock_freq = 8000000; // recall ints are 16 bit on this chip
        float prescaling_factor = 1.0;
        // pr is period of the timer before an interrupt occurs
        // note: pr2 is 16 bit unless we're combining timers
        PR1 = (clock_freq / US_PER_S) * prescaling_factor * kMagicNumber * us;

        // start the timer & enable the interrupt
        T1CONbits.TON = kEnable; // starts the timer
        IEC0bits.T1IE = kEnable; // enable the interrup
}

void xmit_bit(int cycles_high, int cycles_low) {
        // reset timer
        TMR1 = 0;
        PR1 = cycles_high;
        T1CONbits.TON = kEnable; // starts the timer
        IEC0bits.T1IE = kEnable; // enable the interrup
        envelope_flag = 0;

        while (!envelope_flag) {
                LATBbits.LATB9 = 1;
                __delay32(US_13);
                LATBbits.LATB9 = 0;
                __delay32(US_13);
        }

        envelope_flag = 0;
        __delay32(cycles_low);
}

void xmit_samsung_signal(uint32_t msg) {
        NewClk(8); // button debounce uses 32kHz clock. this resets it
        TRISBbits.TRISB9 = 0; // enable pin output
        LATBbits.LATB9 = 0;

        // start bit
        xmit_bit(US_4500, US_4500);

        // message bits
        int i;
        for (i = 0; i < 32; i++) {
                if ((msg >> (31-i) & 0b01) ) { // if next bit is 1
                        xmit_bit(US_516, US_1690);
                } else {
                        xmit_bit(US_516, US_516);
                }
        }

        // stop bit
        xmit_bit(US_516, US_516);
}

// this is the orchestration interrupt
// uses state machine and a state register to decide what to send next
// used for transmitting IR signals to samsung TVs
void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void) {
        IFS0bits.T1IF = 0; // clear interrupt flag
        envelope_flag = 1;
        IEC0bits.T1IE = kDisable; // disable the interrup
        T1CONbits.TON = kDisable; // stops the timer
}
