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
static const char DISABLE = 0;
static const char ENABLE = 1;
static const char INTERNAL = 0;
static const float NO_SCALING = 1.0;
static const float MAGIC_NUMBER = 1.0/2.0; // for the timers, processor specific :p

// ************************************************************ helper functions
static inline void set_timer_priority(int priority) {
	if (priority >= 0 && priority <= 7) {
		IPC15bits.RTCIP = priority;
	}
}

static inline void init_timer2(int frequency) {
	// basic config
	NewClk(frequency); // Switch clock: 32 for 32kHz, 500 for 500 kHz, 8 for 8MHz

	T2CONbits.T32 = DISABLE; // one could combine timers 2 & 3 into 32 bit timer
	T2CONbits.TCKPS = 0; // set pre-scaler (divides timer speed by );
	T2CONbits.TCS = INTERNAL; // use internal clock (ie, not external)
	T2CONbits.TSIDL = DISABLE; // one could stop timer when processor idles
	T2CONbits.TGATE = DISABLE; // one could replace interrupts with an accumulator

	set_timer_priority(5); // sets priority to 5

	TMR2 = 0; // just in case it doesn't happen automatically
}

static inline void init_timer2_32(int frequency) {
	// basic config
	NewClk(frequency); // Switch clock: 32 for 32kHz, 500 for 500 kHz, 8 for 8MHz

	T2CONbits.T32 = ENABLE; // one could combine timers 2 & 3 into 32 bit timer
	T2CONbits.TCKPS = 0; // set pre-scaler (divides timer speed by );
	T2CONbits.TCS = INTERNAL; // use internal clock (ie, not external)
	T2CONbits.TSIDL = DISABLE; // one could stop timer when processor idles
	T2CONbits.TGATE = DISABLE; // one could replace interrupts with an accumulator

	set_timer_priority(5); // sets priority to 5

	TMR3 = 0; // just in case it doesn't happen automatically
}

// ************************************************************** Setting Timers
void delay_ms(uint16_t ms) {
	init_timer2(32);

	int clock_freq = 32000;
	float prescaling_factor = 1.0;
	// pr is period of the timer before an interrupt occurs
	PR2 = (clock_freq/ MS_PER_S) * prescaling_factor * MAGIC_NUMBER * ms;

	// start the timer & enable the interrupt
	T2CONbits.TON = 1; // starts the timer
	IEC0bits.T2IE = 1; // enable the interrupt
}


void delay_us(uint16_t us) {
	init_timer2(8);

	long int clock_freq = 8000000; // recall ints are 16 bit on this chip
	float prescaling_factor = 1.0;
	// pr is period of the timer before an interrupt occurs
	// note: pr2 is 16 bit unless we're combining timers
	PR2 = (clock_freq / US_PER_S) * prescaling_factor * MAGIC_NUMBER * us;

	// start the timer & enable the interrupt
	T2CONbits.TON = 1; // starts the timer
	IEC0bits.T2IE = 1; // enable the interrup
}

void delay_us_32bit(uint32_t us) {
	init_timer2_32(8);

	double clock_freq = 8000000; // recall ints are 16 bit on this chip
	double prescaling_factor = 1.0;
	// pr is period of the timer before an interrupt occurs
	// note: pr2 is 16 bit unless we're combining timers
	long int period = (clock_freq / US_PER_S) * prescaling_factor * MAGIC_NUMBER * us;
	PR3 = (period >> 16) & 0x0000ffff;
	PR2 = period & 0x0000ffff;

	// start the timer & enable the interrupt
	T2CONbits.TON = 1; // starts the timer
	IEC0bits.T3IE = 1; // enable the interrupt
}


// *********************************************************** interrupt handler
void __attribute__((interrupt, no_auto_psv)) _T2Interrupt(void) {
	IFS0bits.T2IF = 0;
	// XmitUART2('x',1); // for debugging
	// LATBbits.LATB9 = !LATBbits.LATB9; // this toggles the IR
	LATBbits.LATB8 = !LATBbits.LATB8; // this toggles the LED
	// T2CONbits.TON = 0; // starts the timer
	// IEC0bits.T2IE = 0; // enable the interrup
}

void __attribute__((interrupt, no_auto_psv)) _T3Interrupt(void) {
	IFS0bits.T3IF = 0;
	// XmitUART2('x',1); // for debugging
	LATBbits.LATB9 = !LATBbits.LATB9; // this toggles the IR
	// LATBbits.LATB8 = !LATBbits.LATB8; // this toggles the LED
	T2CONbits.TON = 0; // starts the timer
	IEC0bits.T2IE = 0; // enable the interrup
}
