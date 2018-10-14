/*
 * File:   Timer.c
 * Author: Nathan
 *
 * Created on September 15, 2018, 8:46 PM
 */

#include "xc.h"
#include "Timer.h"
#include "ChangeClk.h"

#include "UART2.h" // for testing / debugging only

// Magic Numbers
static const char kEnable = 1;
static const char kDisable = 0;
static const char INTERNAL = 0;
static const float kMagicNumber = 1.0/2.0; // for the timers, processor specific :p

// statics
static char toggle_LED_in_t2interrupt = 0;
static char toggle_LED_in_t3interrupt = 0;
static char toggle_IR_in_t2interrupt = 0;

// ************************************************************ helper functions
static inline void set_timer_priority(int priority) {
	if (priority >= 0 && priority <= 7) {
		IPC15bits.RTCIP = priority;
	}
}

static inline void init_timer2(int frequency) {
	// basic config
	NewClk(frequency); // Switch clock: 32 for 32kHz, 500 for 500 kHz, 8 for 8MHz

	T2CONbits.T32 = kDisable; // one could combine timers 2 & 3 into 32 bit timer
	T2CONbits.TCKPS = kDisable; // set pre-scaler (divides timer speed by );
	T2CONbits.TCS = INTERNAL; // use internal clock (ie, not external)
	T2CONbits.TSIDL = kDisable; // one could stop timer when processor idles
	T2CONbits.TGATE = kDisable; // one could replace interrupts with an accumulator

	set_timer_priority(5); // sets priority to 5

	TMR2 = 0; // just in case it doesn't happen automatically
}

static inline void init_timer2_32(int frequency) {
	// basic config
	NewClk(frequency); // Switch clock: 32 for 32kHz, 500 for 500 kHz, 8 for 8MHz

	T2CONbits.T32 = kEnable; // one could combine timers 2 & 3 into 32 bit timer
	T2CONbits.TCKPS = kDisable; // set pre-scaler (divides timer speed by );
	T2CONbits.TCS = INTERNAL; // use internal clock (ie, not external)
	T2CONbits.TSIDL = kDisable; // one could stop timer when processor idles
	T2CONbits.TGATE = kDisable; // one could replace interrupts with an accumulator

	set_timer_priority(5); // sets priority to 5

	TMR3 = 0; // just in case it doesn't happen automatically
}

void toggle_io_if_necessary(void) {
	// XmitUART2('x',1); // for debugging

	if (toggle_LED_in_t2interrupt) {
		LATBbits.LATB8 = !LATBbits.LATB8; // this toggles the LED
	}

	if (toggle_IR_in_t2interrupt) {
		LATBbits.LATB9 = !LATBbits.LATB9; // this toggles the IR
	}
}

// ************************************************************** Setting Timers
void set_LED_toggles_on_t2interrupt(unsigned char perform_toggles) {
	if (perform_toggles == 1) {
		toggle_LED_in_t2interrupt = 1;
	} else {
		toggle_LED_in_t2interrupt = 0;
	}
}

void set_IR_toggles_on_t2interrupt(unsigned char perform_toggles) {
        if (perform_toggles == 1) {
                toggle_IR_in_t2interrupt = 1;
        } else {
                toggle_IR_in_t2interrupt = 0;
        }
}

void set_LED_toggles_on_t3interrupt(unsigned char perform_toggles) {
	if (perform_toggles == 1) {
		toggle_LED_in_t3interrupt = 1;
	} else {
		toggle_LED_in_t3interrupt = 0;
	}
}

void delay_ms(uint16_t ms) {
	init_timer2(32);

	int clock_freq = 32000;
	float prescaling_factor = 1.0;
	// pr is period of the timer before an interrupt occurs
	PR2 = (clock_freq/ MS_PER_S) * prescaling_factor * kMagicNumber * ms;

	// start the timer & enable the interrupt
	T2CONbits.TON = kEnable; // starts the timer
	IEC0bits.T2IE = kEnable; // enable the interrupt
}


void delay_us(uint16_t us) {
	init_timer2(8);

	long int clock_freq = 8000000; // recall ints are 16 bit on this chip
	float prescaling_factor = 1.0;
	// pr is period of the timer before an interrupt occurs
	// note: pr2 is 16 bit unless we're combining timers
	PR2 = (clock_freq / US_PER_S) * prescaling_factor * kMagicNumber * us;

	// start the timer & enable the interrupt
	T2CONbits.TON = kEnable; // starts the timer
	IEC0bits.T2IE = kEnable; // enable the interrup
}

void delay_us_32bit(uint32_t us) {
	init_timer2_32(8);

	double clock_freq = 8000000; // recall ints are 16 bit on this chip
	double prescaling_factor = 1.0;
	// pr is period of the timer before an interrupt occurs
	// note: pr2 is 16 bit unless we're combining timers
	long int period = (clock_freq / US_PER_S) * prescaling_factor * kMagicNumber * us;
	PR3 = (period >> 16) & 0x0000ffff;
	PR2 = period & 0x0000ffff;

	// start the timer & enable the interrupt
	T2CONbits.TON = kEnable; // starts the timer
	IEC0bits.T3IE = kEnable; // enable the interrupt
}

// *********************************************************** interrupt handler
void __attribute__((interrupt, no_auto_psv)) _T2Interrupt(void) {
	IFS0bits.T2IF = kDisable; // disable interrupt

	toggle_io_if_necessary();
	T2CONbits.TON = kDisable; // stop the timer
	IEC0bits.T2IE = kDisable; // stop the interrup
}

void __attribute__((interrupt, no_auto_psv)) _T3Interrupt(void) {
	IFS0bits.T3IF = kDisable; // disable interrupt
	toggle_io_if_necessary();

	T2CONbits.TON = kDisable; // stop the combined timer
	IEC0bits.T3IE = kDisable; // stop the timer 2/3 combined interrupt
}
