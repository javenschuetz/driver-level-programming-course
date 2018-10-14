/*
 * File:   main.c
 * Original Author: rv
 *
 * Created on January 9, 2017, 5:26 PM
 */

// PIC specific libraries
#include "xc.h"
#include <p24fxxxx.h> // other lib for pic?
#include <p24F16KA101.h> // main lib for pic ?

// standard libraries
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <stdint.h>

// driver code
#include "UART2.h"
#include "ChangeClk.h"
#include "Timer.h"
#include "IO.h"
#include "IR.h"

// other includes


//// CONFIGURATION BITS ////

// Code protection
#pragma config BSS = OFF // Boot segment code protect disabled
#pragma config BWRP = OFF // Boot sengment flash write protection off
#pragma config GCP = OFF // general segment code protecion off
#pragma config GWRP = OFF

// CLOCK CONTROL
#pragma config IESO = OFF    // 2 Speed Startup disabled
#pragma config FNOSC = FRC  // Start up CLK = 8 MHz
#pragma config FCKSM = CSECMD // Clock switching is enabled, clock monitor disabled
#pragma config SOSCSEL = SOSCLP // Secondary oscillator for Low Power Operation
#pragma config POSCFREQ = MS  //Primary Oscillator/External clk freq betwn 100kHz and 8 MHz. Options: LS, MS, HS
#pragma config OSCIOFNC = ON  //CLKO output disabled on pin 8, use as IO.
#pragma config POSCMOD = NONE  // Primary oscillator mode is disabled

// WDT = watch dog timer
#pragma config FWDTEN = OFF // WDT is off
#pragma config WINDIS = OFF // STANDARD WDT/. Applicable if WDT is on
#pragma config FWPSA = PR32 // WDT is selected uses prescaler of 32
#pragma config WDTPS = PS1 // WDT postscler is 1 if WDT selected

//MCLR/RA5 CONTROL
#pragma config MCLRE = OFF // RA5 pin configured as input, MCLR reset on RA5 diabled
// b/c can be input or output. off = set to input at time of power-on

//BOR  - FPOR Register
#pragma config BORV = LPBOR // LPBOR value=2V is BOR enabled
#pragma config BOREN = BOR0 // BOR controlled using SBOREN bit
#pragma config PWRTEN = OFF // Powerup timer disabled
#pragma config I2C1SEL = PRI // Default location for SCL1/SDA1 pin
// brown out reset = failsafe for many processors (uncommon in laptops).
// If power supply gets too low (BOR Trigger?), PIC can turn off, so this lets you save critical registers before data is lost

//JTAG FICD Register
#pragma config BKBUG = OFF // Background Debugger functions disabled
#pragma config ICS = PGx2 // PGC2 (pin2) & PGD2 (pin3) are used to connect PICKIT3 debugger

// Deep Sleep RTCC WDT
#pragma config DSWDTEN = OFF // Deep Sleep WDT is disabled
#pragma config DSBOREN = OFF // Deep Sleep BOR is disabled
#pragma config RTCOSC = LPRC// RTCC uses LPRC 32kHz for clock
#pragma config DSWDTOSC = LPRC // DeepSleep WDT uses Lo Power RC clk
#pragma config DSWDTPS = DSWDTPS7 // DSWDT postscaler set to 32768

// MACROS
#define Nop() {__asm__ volatile ("nop");}
#define ClrWdt() {__asm__ volatile ("clrwdt");}
#define Sleep() {__asm__ volatile ("pwrsav #0");}   // set sleep mode
#define Idle() {__asm__ volatile ("pwrsav #1");}
#define dsen() {__asm__ volatile ("BSET DSCON, #15");} // dsen = deep sleep enable?

// GLOBAL VARIABLES
unsigned int temp;
unsigned int i;

// Magic Numbers
static const char PIN_OUTPUT_MODE = 0;
static const char ENABLE = 1;
static const char DISABLE = 0;
static const char ENABLE_NESTING = 0;

// ************************************************************ helper functions
static inline void init_clock(void) {
        //Clock output on REFO
        TRISBbits.TRISB15 = PIN_OUTPUT_MODE;  // Set RB15 as output for REFO
        REFOCONbits.ROEN = ENABLE; // Ref oscillator is enabled
        REFOCONbits.ROSSLP = DISABLE; // Ref oscillator is disabled in sleep
        REFOCONbits.ROSEL = 0; // Output base clk showing clock switching
        REFOCONbits.RODIV = 0b0000;

        // accuracy++
        OSCTUNbits.TUN = 0b011111; // improve clock accuracy slightly

        // 'create' the clock
        NewClk(32); // Switch clock: 32 for 32kHz, 500 for 500 kHz, 8 for 8MHz
}

// ************************************************************************ main
int main(void) { // runs at 1st power-up automatically
        init_clock();

        // enable LED on RB8
        TRISBbits.TRISB8 = PIN_OUTPUT_MODE; // set RB8 as output for LED
        // TRISBbits.TRISB9 = PIN_OUTPUT_MODE; // set RB9 as output for LED
        // delay_us(13);
        // delay_ms(1000);
        // delay_us_t1(13); // pairs with tris9 to power ir led

        INTCON1bits.NSTDIS = ENABLE_NESTING; // enable nesting interrupts

        // CN_init();
        delay_ms(1000);

        // xmit_power_on();
        while(1) {
                Idle();
        }
        return 0;
}
