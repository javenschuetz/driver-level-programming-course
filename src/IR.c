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

static const float MAGIC_NUMBER = 1.0/2.0; // for the timers, processor specific :p
static const char DISABLE = 0;
static const char ENABLE = 1;
static const char INTERNAL = 0;
static const float NO_SCALING = 1.0;

static unsigned char envelope_on = 0;

static inline void set_timer_priority(int priority) {
	if (priority >= 0 && priority <= 7) {
		IPC15bits.RTCIP = priority;
	}
}

static inline void init_timer1(int frequency) {
	// basic config
	NewClk(frequency); // Switch clock: 32 for 32kHz, 500 for 500 kHz, 8 for 8MHz

	//T1CONbits.T32 = DISABLE; // one could combine timers 2 & 3 into 32 bit timer
	T1CONbits.TCKPS = 0; // set pre-scaler (divides timer speed by );
	T1CONbits.TCS = INTERNAL; // use internal clock (ie, not external)
	T1CONbits.TSIDL = DISABLE; // one could stop timer when processor idles
	T1CONbits.TGATE = DISABLE; // one could replace interrupts with an accumulator

	set_timer_priority(5); // sets priority to 5

	TMR2 = 0; // just in case it doesn't happen automatically
}

void delay_us_t1(uint16_t us) {
	init_timer1(8);

	long int clock_freq = 8000000; // recall ints are 16 bit on this chip
	float prescaling_factor = 1.0;
	// pr is period of the timer before an interrupt occurs
	// note: pr2 is 16 bit unless we're combining timers
	PR1 = (clock_freq / US_PER_S) * prescaling_factor * MAGIC_NUMBER * us;

	// start the timer & enable the interrupt
	T1CONbits.TON = 1; // starts the timer
	IEC0bits.T1IE = 1; // enable the interrup
}

void startIREnvelope(unsigned int timeMs){
	// delay_us_t1(timeMs * 1000);
}

static void xmit_start_bit(void) {
	envelope_on = 1;
	delay_us(4500);
	Idle();
	envelope_on = 0;
	delay_us(4500);
	Idle();
}

static void xmit_stop_bit(void) {
	envelope_on = 1;
	delay_us(560);
	Idle();
	envelope_on = 0;
	delay_us(560);
	Idle();
}

static void xmit_zero(void) {
	envelope_on = 1;
	delay_us(560);
	Idle();
	envelope_on = 0;
	delay_us(560);
	Idle();
}

static void xmit_one(void) {
	envelope_on = 1;
	delay_us(560);
	Idle();
	envelope_on = 0;
	delay_us(1690);
	Idle();
}

static void xmit_EOEO(void) {
	xmit_one();
	xmit_one();
	xmit_one();

	xmit_zero();
	xmit_zero();
	xmit_zero();
	xmit_zero();
	xmit_zero();

	xmit_one();
	xmit_one();
	xmit_one();


	xmit_zero();
	xmit_zero();
	xmit_zero();
	xmit_zero();
	xmit_zero();
}
// libpic
void xmit_power_on(void) {
	while(1) {
		// start bit
		xmit_start_bit();

		// EOEO
		xmit_EOEO();

		// stop bit
		xmit_stop_bit();
	}
}

void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void) {
	IFS0bits.T1IF = 0;
	// XmitUART2('x',1); // for debugging
	if (envelope_on) {
		LATBbits.LATB9 = !LATBbits.LATB9;
	}
}

