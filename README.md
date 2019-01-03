# driver-level-programming course

A course on embedded driver-level programming using the PIC24F microcontroller. Work by Ilia Gaissinski and Nathan Schuetz.

## Projects

#### Blinking LED 
The embedded hello world

#### Buttons
Drivers to handle buttons using change of notification interrupts. Mostly about handling debouncing using state machines

#### Samsung Remote Control
Drivers to use 2 PICs to both transmit and receive an signal from an IR LED. Used a carrier wave and an envelope to follow the Samsung spec, and tested the receiver using a real samsung remote.

#### CVREF
Driver to customize the low reference voltage seen by other parts of the PIC

#### Comparator
Driver to setup and handle interrupts caused by changing pin voltages. Used in conjunction with CVREF

#### ADC
Driver to measure 'instantaneous' DC voltages on a pin

#### CTMU
Driver to use the CTMU current source reliably

#### Capacitance Touch Sensors
Drivers to handle a capacitive touch sensor with the help of the CTMU & ADC.
