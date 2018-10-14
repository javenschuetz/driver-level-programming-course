#ifndef TIMER_H
#define	TIMER_H

#include <xc.h> // include processor files - each processor file is guarded.
#include <stdint.h> // for uint16_t

// globals
#define MS_PER_S 1000
#define US_PER_S 1000000

// wraps delay_ms to enable LED flickering
void set_LED_toggles_on_t2interrupt(unsigned char perform_toggles);
void set_IR_toggles_on_t2interrupt(unsigned char perform_toggles);

// fire Interrupt after a few ms
void delay_ms(uint16_t ms);
void delay_us(uint16_t us);
void delay_us_32bit(uint32_t us);

void __attribute__ ((interrupt, no_auto_psv)) _T2Interrupt(void); // interrupt handler

#endif
