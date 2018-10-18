#ifndef IR_H
#define	IR_H

#include <xc.h> // include processor files - each processor file is guarded.
#include <stdint.h>
#include "libpic30.h"

// codes for Samsung protocol (reversed, due to implementatoin in ISR)
static uint32_t kPowerToggleBits = 0xe0e040bf;
static uint32_t kChannelUpBits = 0xe0e048b7;
static uint32_t kChannelDownBits = 0xe0e008f7;
static uint32_t kVolumeUpBits = 0xe0e0e01f;
static uint32_t kVolumeDownBits = 0xe0e0d02f;

void xmit_samsung_signal(uint32_t message); // message is one of the above unless you're an anarchist
void delay_us_t1(uint16_t us); // temporary hack TODO

#endif	/* IR_H */
