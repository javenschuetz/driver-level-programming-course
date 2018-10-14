#ifndef TIMER_H
#define	TIMER_H

#include <xc.h> // include processor files - each processor file is guarded.
#include <stdint.h> // for uint16_t

// globals
#define MS_PER_S 1000
#define US_PER_S 1000000

void delay_ms(uint16_t ms); // begin firing interrupt via Timer 2 per interval
void delay_us(uint16_t us);
void delay_us_32bit(uint32_t us);
void __attribute__ ((interrupt, no_auto_psv)) _T2Interrupt(void); // interrupt handler

#endif
