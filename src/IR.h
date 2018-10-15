#ifndef IR_H
#define	IR_H

#include <xc.h> // include processor files - each processor file is guarded.

enum CURRENT_MESSAGE {kPowerToggle, kChannelUp, kChannelDown,
                      kVolumeUp, kVolumeDown, kNothing};

void xmit_power_on(void);

void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void);

#endif	/* IR_H */
