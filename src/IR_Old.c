/*
 * File:   IR.c
 * Author: Tilo
 *
 * Created on October 5, 2018, 5:29 PM
 */

// libraries and header
#include "IR.h"
#include "xc.h"

// project files
#include "IO.h"
#include "Timer.h"
#include "ChangeClk.h"

// magic numbers
static const float kMagicNumber = 1.0/2.0; // for the timers, processor specific :p
static const char kEnable = 1;
static const char kDisable = 0;
static const char kInternalClk = 0;
static const char kOutputEnable = 0;
static const char kOutputDisable = 1;

// codes for Samsung protocol (reversed, due to implementatoin in ISR)
static uint32_t kPowerToggleBits = 0xfb040e0e;
static uint32_t kChannelUpBits = 0x7b840e0e;
static uint32_t kChannelDownBits = 0x7f800e0e;
static uint32_t kVolumeUpBits = 0xf10e0e0e;
static uint32_t kVolumeDownBits = 0xf20d0e0e;

// statics
//static enum XMIT_STATE {kWaiting, kOneHigh, kZeroHigh, kStartHigh, kStopHigh, kLow};
//static enum XMIT_STATE current_xmit_state = kLow;//kWaiting;

static long int current_message = -1; // this will be one of the above bit sequences
static int message_index = 0;

// ************************************************************ helper functions
static inline void set_timer_priority(int priority) {
        if (priority >= 0 && priority <= 7) {
                IPC15bits.RTCIP = priority;
        }
}

static inline void init_timer1(int frequency) {
        // basic config
        // NewClk(frequency); // Switch clock: 32 for 32kHz, 500 for 500 kHz, 8 for 8MHz

        T1CONbits.TCKPS = kDisable; // set pre-scaler (divides timer speed by );
        T1CONbits.TCS = kInternalClk; // use internal clock (ie, not external)
        T1CONbits.TSIDL = kDisable; // one could stop timer when processor idles
        T1CONbits.TGATE = kDisable; // one could replace interrupts with an accumulator

        set_timer_priority(5); // sets priority to 5

        TMR1 = 0; // just in case it doesn't happen automatically
}

static inline void delay_us_t1(uint16_t us) {
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

void xmit_samsung_signal(enum CURRENT_MESSAGE msg) {
        switch (msg) {
        case kPowerToggle:
                current_message = kPowerToggleBits;
                break;
        case kChannelUp:
                current_message = kChannelUpBits;
                break;
        case kChannelDown:
                current_message = kChannelDownBits;
                break;
        case kVolumeUp:
                current_message = kVolumeUpBits;
                break;
        case kVolumeDown:
                current_message = kVolumeDownBits;
                break;
        }
        delay_us_t1(1); // hack to start interrupt
}

// this is the orchestration interrupt
// uses state machine and a state register to decide what to send next
// used for transmitting IR signals to samsung TVs
void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void) {
        IFS0bits.T1IF = 0; // clear interrupt flag
        IEC0bits.T1IE = kDisable; // disable the interrupt
        T1CONbits.TON = kDisable; // stops the timer
        // delay_us(13, 1);

        switch (current_xmit_state) {
        case kStartHigh:
                // transmit low portion of start bit
                current_xmit_state = kLow;
                TRISBbits.TRISB9 = 1; // disable pin output
                delay_us_t1(4500);
                break;
        case kOneHigh:
                // transmit low portion of start bit
                current_xmit_state = kLow;
                LATBbits.LATB9 = 0;
                TRISBbits.TRISB9 = 1; // disable pin output
                delay_us_t1(1390);//(1690);
                break;
        case kZeroHigh:
                // transmit low portion of start bit
                current_xmit_state = kLow;
                LATBbits.LATB9 = 0;
                TRISBbits.TRISB9 = 1; // disable pin output
                // XmitUART2('0', 1);
                delay_us_t1(560);
                break;
        case kLow:
                current_xmit_state = kOneHigh;

                TRISBbits.TRISB9 = 0; // enable pin output
                LATBbits.LATB9 = 1;
                delay_us_t1(260);//(560);
//                if (message_index % 4 == 0) {
//                        // XmitUART2(' ', 1);
//                }
//                if (message_index >= 32) { // message sent, so send stop bit
//                        current_xmit_state = kStopHigh;
//                        delay_us_t1(560);
//                        TRISBbits.TRISB9 = 0; // enable pin output
//                } else if (current_message % 2) { // if next bit is 1
//                        current_xmit_state = kOneHigh;
//                        delay_us_t1(560);
//                        TRISBbits.TRISB9 = 0; // enable pin output
//                } else {
//                        current_xmit_state = kZeroHigh;
//                        delay_us_t1(560);
//                        TRISBbits.TRISB9 = 0; // enable pin output
//                }
//                message_index++;
//                current_message = current_message >> 1;
                break;
        case kStopHigh:
                current_message = -1; // no message remaining to transmit
                current_xmit_state = kWaiting; // done transmitting after this
                LATBbits.LATB9 = 0;
                TRISBbits.TRISB9 = 1; // disable pin output
                delay_us_t1(560);
                break;
        case kWaiting:
                current_xmit_state = kOneHigh;
                delay_us_t1(560);
                TRISBbits.TRISB9 = 0; // enable pin output
                LATBbits.LATB9 = 1;
//                if (current_message == -1) {
//                        LATBbits.LATB9 = 0; // set to low
//                        TRISBbits.TRISB9 = 1; // disable pin output
//                        return; // no message is configured
//                }
//                message_index = 0;
//                // transmit high portion of start bit
//                current_xmit_state = kStartHigh;
//                delay_us_t1(4500);
//                // XmitUART2('x', 1);
//                TRISBbits.TRISB9 = 0; // enable pin output
//                break;
        }
}
