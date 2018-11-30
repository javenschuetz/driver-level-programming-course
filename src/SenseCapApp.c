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

#define FCY 4000000UL;

//setup CTMU
void CTMUinit(float current){
        // function only supports these values
        if (current != 5.5 && current != 55) {
                return;
        }

        //Setting CTMU bits
        CTMUCONbits.TGEN = 0;           //disable edge time delay, also might cause current
        CTMUCONbits.IDISSEN = 0;        //opens the ground path
        CTMUCONbits.EDGEN = 0;          //block edges
        CTMUCONbits.EDGSEQEN = 0;       //no edge sequences
        CTMUCONbits.CTTRIG = 0;         //disable trigger output
        CTMUCONbits.EDG1STAT = 1;

        if (current == 55) {
                CTMUICONbits.IRNG = 0b11;   // 0b11 = set curren to 10x input current
        } else if (current == 5.5) {
                CTMUICONbits.IRNG = 0b01;   // 0b01 = set curren to 1x input current
        }

        CTMUICONbits.ITRIM = 0b001000;  //Adjusting levels 5.5ua - 0b001000

        //SETTING ADC BITS
        //    init_ADC();
        AD1CHSbits.CH0NA = 0;
        AD1CHSbits.CH0SA = 0b1011;      //sets ADC ch0 sample A to pin 16 - AN11/RB13
        AD1CHSbits.CH0NB = 0;
        AD1CHSbits.CH0SB = 0b1011;
        TRISBbits.TRISB13 = 1;          //sets RB13 to input
        AD1PCFGbits.PCFG11 = 0;         //sets AN11 to analog mode
        AD1CSSLbits.CSSL11 = 0;         //omit AN11 from scan
        AD1CON2bits.VCFG = 0b000;
        AD1CON1bits.FORM = 0b00;
        AD1CON2bits.CSCNA = 0;
        AD1CON2bits.SMPI = 0b0000;
        AD1CON2bits.BUFM = 0;
        AD1CON1bits.ASAM = 1;           //changed to 1 for test!!!!!!!!!!!!!!!!
        AD1CON1bits.SSRC = 0b000;       //changed to 1 for test!!!!!!!!!!!!!!!!
        AD1CON3bits.ADRC = 1;           //sets conversion clock to internal - prof said to 1
        AD1CON3bits.ADCS = 0b11111;     //A/D Conversion Clock Select bits
        AD1CON3bits.SAMC = 0b11111;
        AD1CON1bits.ADSIDL = 0;         //continues conversion when in idle mode

        CTMUCONbits.EDG2STAT = 1;       // current source is on
        CTMUCONbits.EDG1STAT = 0;       //    ^- as a combo ONLY
        CTMUCONbits.CTMUEN = 1;         //enable module
        AD1CON1bits.SAMP = 1;           //enable A/D conversion
        AD1CON1bits.ADON = 1;           //enable A/D module

        // Note: 353us is the dt for 2.2nF
        // delay_us_32bit(35,sampleCap);   //delays 35us (about dt/10)
}

/**
 * Calculates the capacitance.
 *
 *
 * Note: Voltage is calculated by multiplying dec representative by 3.22!!!!!!
 * Note 2: The charging time should be the same as delay_us!!!!!
 *          Now it is about 300us
 * Note 3: delay_us_32bit might be taking too much time.
 * Note 4: Done is not set, even tho it should, play with SSRC and ASAM
 */
void sampleCap(float current_uA, float delta_t){

        CTMUCONbits.CTMUEN = 0;         //disable module
        // alert - this 'holds' the capacitor value
        AD1CON1bits.SAMP = 0;           //stop sampling input

        //TODO: check when to turn off the CS
        //TODO: SC turns off when ADC convs

        // wait for done bit
        static const int kADCDone = 1;
        while (AD1CON1bits.DONE != kADCDone) {} //loop until done

        //
        int buffer_value = 0;
        buffer_value = ADC1BUF0;        //stores sample result
        buffer_value = buffer_value & 0x03ff;   //masking the last 10 bits

        // discharges the capacitor
        CTMUCONbits.IDISSEN = 1;

        //format and display the sampled values
        XmitUART2('\r', 1);

        int i;

        for (i = 0; i < buffer_value / 30; i++) {
                XmitUART2(220, 1);      //ASCII bar character
        }

        // print voltage
        static const float vref_plus = 3.25;
        static const float vref_neg = 0.0f;
        static const float max_step = 1023;
        float voltage = ((vref_plus - vref_neg) + vref_neg) *
                        (1000.0f * buffer_value / max_step);
        char *voltage_s[100];
        itoa(voltage, voltage_s, 10);

        Disp2String(" voltage: ");
        Disp2String(voltage_s);
        Disp2String("uV     ");

        // print capacitance
        float capacitance = (delta_t / voltage) * 1000.0f / current_uA;
        char *capacitance_s[100];
        sprintf(capacitance_s, "%g", capacitance);
        Disp2String("capacitance: ");
        Disp2String(capacitance_s);
        // Disp2String("... bottle caps? ilia help!");

        for (i = 0; i < 20; i++) {
                XmitUART2(' ', 1);      //empty spaces for neat output
        }

        CTMUCONbits.IDISSEN = 0;    //disconnects the cap from GND
        //    CTMUCONbits.CTMUEN = 1;     //re-enable the current source
}
