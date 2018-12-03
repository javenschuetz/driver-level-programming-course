/*
 * File:   SenseCapApp.c
 * Author: Ilia, Nathan
 *
 * Created on November 19, 2018, 7:02 PM
 */

// libraries
#include <libpic30.h>
#include "xc.h"
#include "UART2.h"

// project files
#include "SenseCapApp.h"
#include "misc.h"
#include "comparator.h"
#include "ADC.h"
#include "Timer.h"

//setup CTMU
void CTMUinit(float current){
        //Setting CTMU bits
        CTMUCONbits.TGEN = 0;           // disable edge time delay, also might cause current
        CTMUCONbits.IDISSEN = 0;        // opens the ground path
        CTMUCONbits.EDGEN = 0;          // block edges
        CTMUCONbits.EDGSEQEN = 0;       // no edge sequences
        CTMUCONbits.CTTRIG = 0;         // disable trigger output
        CTMUCONbits.EDG1STAT = 0;       // EG1/2 turn current on if equal to one another
        CTMUCONbits.EDG2STAT = 0;       // see above ^

        /*
        * Controls current
        *
        * 0b00 = current off
        * 0b01 = current 1x (0.55uA nominal)
        * 0b10 = set curren to 10x input current
        * 0b11 = set curren to 100x input current
        **/
        if (current == 0.55) {
                CTMUICONbits.IRNG = 0b01;
        } else if (current == 5.5) {
                CTMUICONbits.IRNG = 0b10;
        } else if (current == 55) {
                CTMUICONbits.IRNG = 0b11;
        } else {
                Disp2String("Invalid Current!");
                return;
        }
        /*
        * Adjust current knob
        * Nathan's laptop: 0b101011 means I seem to get within 20% of target i think*/
        CTMUICONbits.ITRIM = 0b101011;  // Adjusting levels 5.5ua - 0b001000

        //SETTING ADC BITS
        //    init_ADC();
        AD1CHSbits.CH0NA = 0;
        AD1CHSbits.CH0SA = 0b1011;      // sets ADC ch0 sample A to pin 16 - AN11/RB13
        AD1CHSbits.CH0NB = 0;
        AD1CHSbits.CH0SB = 0b1011;
        TRISBbits.TRISB13 = 1;          // sets RB13 to input
        AD1PCFGbits.PCFG11 = 0;         // sets AN11 to analog mode
        AD1CSSLbits.CSSL11 = 0;         // omit AN11 from scan
        AD1CON2bits.VCFG = 0b000;       // 0 = use Avdd Avss
        AD1CON1bits.FORM = 0b00;        // 0 = integer
        AD1CON2bits.CSCNA = 0;          // 0 = don't scan inputs
        AD1CON2bits.SMPI = 0b0000;      // 0 = interrupt after conversions??
        AD1CON2bits.BUFM = 0;           // 0 = 16 bit buffer
        AD1CON1bits.ASAM = 1;           // 1 = sampling starts asap
        AD1CON1bits.SSRC = 0b000;       // 0 = clearing samp starts conversion
        AD1CON3bits.ADRC = 1;           // sets conversion clock to internal - prof said to 1
        AD1CON3bits.ADCS = 0b11111;     // A/D Conversion Clock Select bits
        AD1CON3bits.SAMC = 0b11111;     // determines sample rate
        AD1CON1bits.ADSIDL = 0;         // continue conversion when in idle mode

        CTMUCONbits.CTMUEN = 1;         // enable current source
        CTMUCONbits.EDG2STAT = 1;       // current source is on
        CTMUCONbits.EDG1STAT = 0;       //    ^- as a combo ONLY
        AD1CON1bits.SAMP = 1;           // start sampling & don't hold pin value
        AD1CON1bits.ADON = 1;           // enable A/D module

        // Note: 353us is the dt for 2.2nF
        // delay_us_32bit(35,sampleCapacitance);   //delays 35us (about dt/10)
}

static void print_results(float current_uA, float time_us, int step_value) {
        /*
        * Note: for 5.5uA and 50k resistor, with no discharge, should see 2.75V
        */
        XmitUART2('\r', 1);
        XmitUART2('\n', 1);

        int i;
        for (i = 0; i < step_value / 30; i++) {
                XmitUART2(220, 1); //ASCII bar character
        }

        // print voltage
        static const float vref_plus = 3.25;
        static const float vref_neg = 0.1f;
        static const float max_step = 1023; // float to prevent truncation
        double voltage = vref_neg +
                        ((vref_plus - vref_neg) * ( (float) step_value / max_step));
        Disp2String("debug msg:");
        Disp2Hex(step_value);
        char *voltage_s[100];
        sprintf(voltage_s, "%g", voltage);

        Disp2String(" voltage: ");
        Disp2String(voltage_s);
        Disp2String("V     ");

        // print capacitance
        float time_s = time_us / 1000000.0f;
        float current_A = current_uA / 1000000.0f;

        double capacitance = (time_s / voltage) * current_A;
        char *capacitance_s[100];
        sprintf(capacitance_s, "%g", capacitance);
        Disp2String("capacitance: ");
        Disp2String(capacitance_s);
        // Disp2String("... bottle caps? ilia help!");

        for (i = 0; i < 20; i++) {
                XmitUART2(' ', 1);      // empty spaces for neat output
        }
}

/**
 * Calculates the capacitance.
 */
void sampleCapacitance(float current_uA, float time_us){
        CTMUCONbits.CTMUEN = 0; // disable current source (comment out to test voltages)

        /*
        * this does a lot:
        *       - stop sampling input
        *       - begin conversion
        *       - 'hold' the capacitor value until set to 1 again
        */
        AD1CON1bits.SAMP = 0;

        // wait for done bit
        static const int kADCDone = 1;
        while (AD1CON1bits.DONE != kADCDone) {} // loop until done

        AD1CON1bits.SAMP = 1; // 1 = stop holding cap value
        CTMUCONbits.IDISSEN = 1; // discharge the capacitor during printing

        int step_value = ADC1BUF0 & 0x03ff; // stores last 10 bits of sample result
        print_results(current_uA, time_us, step_value);

        CTMUCONbits.IDISSEN = 0; // un-ground the capacitor
}
