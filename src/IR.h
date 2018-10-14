#ifndef IR_H
#define	IR_H

#include <xc.h> // include processor files - each processor file is guarded.

enum CURRENT_MESSAGE {kPowerToggle, kChannelUp, kChannelDown,
                      kVolumeUp, kVolumeDown, kNothing};

void xmit_power_on(void);

#endif	/* IR_H */
