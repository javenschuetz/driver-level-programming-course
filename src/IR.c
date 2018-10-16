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

// magic numbers
static const float kMagicNumber = 1.0/2.0; // for the timers, processor specific :p
static const char kEnable = 1;
static const char kDisable = 0;
static const char kInternalClk = 0;
static const char kOutputEnable = 0;
static const char kOutputDisable = 1;

// codes for Samsung protocol
static uint32_t kPowerToggleBits = 0xE0E040BF;
static uint32_t kChannelUpBits = 0xE0E048B7;
static uint32_t kChannelDownBits = 0xE0E008F7;
static uint32_t kVolumeUpBits = 0xE0E0E01F;
static uint32_t kVolumeDownBits = 0xE0E0D02F;

// statics
static enum XMIT_STATE {kWaiting, kOneHigh, kZeroHigh, kStartHigh, kStopHigh, kLow};
static enum XMIT_STATE current_xmit_state = kWaiting;

static long int current_message = -1; // this will be one of the above bit sequences
static char message_index = 0;

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

static void delay_us_t1(uint16_t us) {
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

void xmit_power_on(void) {
        current_message = kPowerToggleBits;
        delay_us_t1(1); // hack to start interrupt
}

// this is the orchestration interrupt
// uses state machine and a state register to decide what to send next
// used for transmitting IR signals to samsung TVs
void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void) {
        IFS0bits.T1IF = 0; // clear interrupt flag
        IEC0bits.T1IE = kDisable; // disable the interrup
        T1CONbits.TON = kDisable; // stops the timer

        switch (current_xmit_state) {
        case kStartHigh:
                // transmit low portion of start bit
                current_xmit_state = kLow;
                IEC0bits.T2IE = kDisable; // turn off pulse
                TRISBbits.TRISB9 = 1; // set output to nothing
                message_index = 0; // ready to transmit momentarily
                delay_us_t1(4500);
                break;
        case kOneHigh:
                // transmit low portion of start bit
                current_xmit_state = kLow;
                IEC0bits.T2IE = kDisable; // turn off pulse
                TRISBbits.TRISB9 = 1; // set output to nothing
                delay_us_t1(1690);
                break;
        case kZeroHigh:
                // transmit low portion of start bit
                current_xmit_state = kLow;
                IEC0bits.T2IE = kDisable; // turn off pulse
                TRISBbits.TRISB9 = 1; // set output to nothing
                delay_us_t1(4500);
                break;
        case kStopHigh:
                current_message = -1; // no message remaining to transmit
                current_xmit_state = kWaiting; // done transmitting after this
                IEC0bits.T2IE = kDisable; // turn off pulse
                TRISBbits.TRISB9 = 1; // set output to nothing
                delay_us_t1(560);
                break;
        case kLow:
                if (message_index >= 31) { // message sent, so send stop bit
                        current_xmit_state = kStopHigh;
                        IEC0bits.T2IE = kEnable; // turn on pulse
                        delay_us_t1(560);
                } else if (current_message % 2) { // if next bit is 1
                        current_xmit_state = kOneHigh;
                        IEC0bits.T2IE = kEnable; // turn on pulse
                        delay_us_t1(560);
                } else {
                        current_xmit_state = kZeroHigh;
                        IEC0bits.T2IE = kEnable; // turn on pulse
                        delay_us_t1(560);
                }
                message_index++;
                current_message << 1;
                break;
        case kWaiting:
                if (current_message == -1) {
                        return; // no message is configured
                }
                // transmit high portion of start bit
                current_xmit_state = kStartHigh;
                IEC0bits.T2IE = kEnable; // turn on pulse
                delay_us_t1(4500);
                break;
        }
}

