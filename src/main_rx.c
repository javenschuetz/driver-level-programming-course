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

// libraries
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>

// driver code
#include "ChangeClk.h"
#include "IR.h"
#include "samsung_rx.h"
#include "Timer.h"
#include "UART2.h"


//// CONFIGURATION BITS ////

// Code protection
#pragma config BSS = OFF // Boot segment code protect kDisabled
#pragma config BWRP = OFF // Boot sengment flash write protection off
#pragma config GCP = OFF // general segment code protecion off
#pragma config GWRP = OFF

// CLOCK CONTROL
#pragma config IESO = OFF    // 2 Speed Startup kDisabled
#pragma config FNOSC = FRC  // Start up CLK = 8 MHz
#pragma config FCKSM = CSECMD // Clock switching is kEnabled, clock monitor kDisabled
#pragma config SOSCSEL = SOSCLP // Secondary oscillator for Low Power Operation
#pragma config POSCFREQ = MS  //Primary Oscillator/External clk freq betwn 100kHz and 8 MHz. Options: LS, MS, HS
#pragma config OSCIOFNC = ON  //CLKO output kDisabled on pin 8, use as IO.
#pragma config POSCMOD = NONE  // Primary oscillator mode is kDisabled

// WDT = watch dog timer
#pragma config FWDTEN = OFF // WDT is off
#pragma config WINDIS = OFF // STANDARD WDT/. Applicable if WDT is on
#pragma config FWPSA = PR32 // WDT is selected uses prescaler of 32
#pragma config WDTPS = PS1 // WDT postscler is 1 if WDT selected

//MCLR/RA5 CONTROL
#pragma config MCLRE = OFF // RA5 pin configured as input, MCLR reset on RA5 diabled
// b/c can be input or output. off = set to input at time of power-on

//BOR  - FPOR Register
#pragma config BORV = LPBOR // LPBOR value=2V is BOR kEnabled
#pragma config BOREN = BOR0 // BOR controlled using SBOREN bit
#pragma config PWRTEN = OFF // Powerup timer kDisabled
#pragma config I2C1SEL = PRI // Default location for SCL1/SDA1 pin
// brown out reset = failsafe for many processors (uncommon in laptops).
// If power supply gets too low (BOR Trigger?), PIC can turn off, so this lets you save critical registers before data is lost

//JTAG FICD Register
#pragma config BKBUG = OFF // Background Debugger functions kDisabled
#pragma config ICS = PGx2 // PGC2 (pin2) & PGD2 (pin3) are used to connect PICKIT3 debugger

// Deep Sleep RTCC WDT
#pragma config DSWDTEN = OFF // Deep Sleep WDT is kDisabled
#pragma config DSBOREN = OFF // Deep Sleep BOR is kDisabled
#pragma config RTCOSC = LPRC// RTCC uses LPRC 32kHz for clock
#pragma config DSWDTOSC = LPRC // DeepSleep WDT uses Lo Power RC clk
#pragma config DSWDTPS = DSWDTPS7 // DSWDT postscaler set to 32768

// MACROS
#define Nop() {__asm__ volatile ("nop");}
#define ClrWdt() {__asm__ volatile ("clrwdt");}
#define Sleep() {__asm__ volatile ("pwrsav #0");}   // set sleep mode
#define Idle() {__asm__ volatile ("pwrsav #1");}
#define dsen() {__asm__ volatile ("BSET DSCON, #15");} // dsen = deep sleep kEnable?

// GLOBAL VARIABLES
#define FCY 4000000UL;


// Magic Numbers
static const char kOutputEnable = 0;
static const char kEnable = 1;
static const char kDisable = 0;
static const char kEnableNesting = 0;

// ************************************************************ helper functions
static inline void init_clock(unsigned int freq) { // shared code
        //Clock output on REFO
        TRISBbits.TRISB15 = kOutputEnable;  // Set RB15 as output for REFO
        REFOCONbits.ROEN = kEnable; // Ref oscillator is disabled
        REFOCONbits.ROSSLP = kDisable; // true = Ref oscillator is kDisabled in sleep
        REFOCONbits.ROSEL = 0; // Output base clk showing clock switching
        REFOCONbits.RODIV = 0b0000;

        // accuracy++
        OSCTUNbits.TUN = 0b011111; // improve clock accuracy slightly

        // 'create' the clock
        NewClk(freq); // Switch clock: 32 for 32kHz, 500 for 500 kHz, 8 for 8MHz
}

// ************************************************************************ main
int main(void) { // runs at 1st power-up automatically
        init_clock(8);
        CN_init();

        while(1) {
                Idle();
        }
        return 0;
}
