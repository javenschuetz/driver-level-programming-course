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

static const enum {kLow, kHigh};
static uint32_t kPowerToggle = 0xE0E040BF;
static uint32_t kChannelUp = 0xE0E048B7;
static uint32_t kChannelDown = 0xE0E008F7;
static uint32_t kVolumeUp = 0xE0E0E01F;
static uint32_t kVolumeDown = 0xE0E0D02F;
// statics
static unsigned char envelope_on = 0;

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
	// E
	xmit_one();
	xmit_one();
	xmit_one();
	xmit_zero();

	// O
	xmit_zero();
	xmit_zero();
	xmit_zero();
	xmit_zero();

	// E
	xmit_one();
	xmit_one();
	xmit_one();
	xmit_zero();

	// O
	xmit_zero();
	xmit_zero();
	xmit_zero();
	xmit_zero();
}

void xmit_power_on(void) {
	while(1) {
		xmit_start_bit();
		xmit_EOEO();
		xmit_stop_bit();
	}
}



void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void) {
	IFS0bits.T1IF = kDisable;
	if (envelope_on) {
		LATBbits.LATB9 = !LATBbits.LATB9;
	}
}

