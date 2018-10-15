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
#include "UART2.h" // for debugging

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
static uint32_t current_message = 0; // this will be one of the above bit sequences
static int xmit_in_progress = 0;
static int continue_pls = 0;

// ************************************************************ helper functions
static inline void set_timer1_priority(int priority) {
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

        set_timer1_priority(7); // sets priority to highest (7)

        TMR1 = 0; // just in case it doesn't happen automatically
}

static void delay_us_t1(uint16_t us) {
        init_timer1(8);

        long int clock_freq = 8000000; // recall ints are 16 bit on this chip
        float prescaling_factor = 1.0;
        // pr is period of the timer before an interrupt occurs
        // note: pr2 is 16 bit unless we're combining timers
        PR1 = (clock_freq / (double) US_PER_S) * prescaling_factor * kMagicNumber * us;

        // start the timer & enable the interrupt
        T1CONbits.TON = kEnable; // starts the timer
        IEC0bits.T1IE = kEnable; // enable the interrup
}

static void xmit_start_bit(void) {
        IEC0bits.T2IE = kEnable; // enable the interrupt
        delay_us_t1(4500);
        while (!continue_pls) {
                Idle();
        }
        continue_pls = 0; // clear the flag

        IEC0bits.T2IE = kDisable; // stop the interrup
        LATBbits.LATB9 = 1; // set to low
        delay_us_t1(4500);
        while (!continue_pls) {
                Idle();
        }
        continue_pls = 0; // clear the flag
}

// static void xmit_stop_bit(void) {
//         // IEC0bits.T2IE = kEnable; // enable the interrupt
//         delay_us_t1(560);
//         while (!IFS0bits.T1IF) {
//                 Idle();
//         }
//         IFS0bits.T1IF = 0; // clear the flag

//         IEC0bits.T2IE = kDisable; // disable the interrupt
//         LATBbits.LATB9 = 0; // set to low
//         delay_us_t1(560);
//         while (!IFS0bits.T1IF) {
//                 Idle();
//         }
//         IFS0bits.T1IF = 0; // clear the flag
// }

// static void xmit_zero(void) {
//         // IEC0bits.T2IE = kEnable; // enable the interrupt
//         delay_us_t1(560);
//         while (!IFS0bits.T1IF) {
//                 Idle();
//         }
//         IFS0bits.T1IF = 0; // clear the flag

//         IEC0bits.T2IE = kDisable; // disable the interrupt
//         LATBbits.LATB9 = 0; // set to low
//         delay_us_t1(560);
//         while (!IFS0bits.T1IF) {
//                 Idle();
//         }
//         IFS0bits.T1IF = 0; // clear the flag
// }

// static void xmit_one(void) {
//         // IEC0bits.T2IE = kEnable; // enable the interrupt
//         delay_us_t1(560);
//         while (!IFS0bits.T1IF) {
//                 Idle();
//         }
//         IFS0bits.T1IF = 0; // clear the flag

//         IEC0bits.T2IE = kDisable; // disable the interrupt
//         LATBbits.LATB9 = 0; // set to low
//         delay_us_t1(1690);
//         while (!IFS0bits.T1IF) {
//                 Idle();
//         }
//         IFS0bits.T1IF = 0; // clear the flag
// }

void xmit_power_on(void) {
        current_message = kPowerToggleBits;
        delay_us_t1(1000);
}

// this is the orchestration interrupt
void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void) {
        continue_pls = 1;
        IFS0bits.T1IF = 0;
        T1CONbits.TON = kDisable; // stop timer 1
        IEC0bits.T1IE = kDisable; // stop the t1 interrup

        if (xmit_in_progress) {
                return;
        }
        continue_pls = 0;
        xmit_in_progress = 1;

        // // XmitUART2('x', 1);
        xmit_start_bit();
        xmit_start_bit();
        xmit_start_bit();
        xmit_start_bit();
        xmit_start_bit();
        xmit_start_bit();
        xmit_start_bit();
        xmit_start_bit();
        xmit_start_bit();
        xmit_start_bit();
        xmit_start_bit();
        xmit_start_bit();
        xmit_start_bit();

        // // int i;
        // // while (i < 34) {
        // //         if (i == 0) {
        // //                 xmit_start_bit();
        // //                 i++;
        // //         } else if (i < 33) {
        // //                 i++;
        // //         } else {
        // //                 xmit_stop_bit();
        // //                 i++;
        // //         }
        // // }
        // // // XmitUART2('z', 1);

        xmit_in_progress = 0;
        delay_us_t1(1000);
}
