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
#include "string.h"

// project files
#include "SenseCapApp.h"
#include "misc.h"
#include "comparator.h"
#include "ADC.h"
#include "Timer.h"

#define FCY 4000000UL;
#define MILI 0.001
#define MICRO 0.000001
#define NANO 0.000000001
#define PICO 0.000000000001

//setup CTMU and ADC
void CTMUinit(){
        //Setting CTMU bits
        CTMUCONbits.TGEN = 0;           // disable edge time delay, also might cause current
        CTMUCONbits.IDISSEN = 0;        // opens the ground path
        CTMUCONbits.EDGEN = 0;          // block edges
        CTMUCONbits.EDGSEQEN = 0;       // no edge sequences
        CTMUCONbits.CTTRIG = 0;         // disable trigger output
        CTMUCONbits.EDG1STAT = 0;       // EG1/2 turn current on if equal to one another
        CTMUCONbits.EDG2STAT = 0;       // see above ^

        //SETTING ADC BITS
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
}
/**
 * Turns on the CTMU and the current source. As well, turns on the ADC module
 * @param current
 */
void start_current_source(float current) {
        /*
        * Controls current
        *
        * IRNG = current multiplier
        * 0b00 = current off
        * 0b01 = current 1x (0.55uA nominal)
        * 0b10 = set current to 10x input current
        * 0b11 = set current to 100x input current
        *
        * ITRIM = current accuracy knob
        **/
        if (current == 0.55) {
                CTMUICONbits.IRNG = 0b01;
        } else if (current == 5.5) {
                CTMUICONbits.IRNG = 0b10;
//                CTMUICONbits.ITRIM = 0b000001; // for Nathan's laptop - reduce current a bit
                 CTMUICONbits.ITRIM = 0b001000; // note - Ilia's laptop needed 0b001000 for 5.5uA
        } else if (current == 55) {
                CTMUICONbits.IRNG = 0b11;
                CTMUICONbits.ITRIM = 0b000111; // increase current a bit
        } else {
                Disp2String("Invalid Current!");
                return;
        }
        
        CTMUCONbits.IDISSEN = 0;        // break the GND path
        CTMUCONbits.CTMUEN = 1;         // enable current source
        CTMUCONbits.EDG2STAT = 1;       // current source is on
        CTMUCONbits.EDG1STAT = 0;       //    ^- as a combo ONLY
        AD1CON1bits.SAMP = 1;           // start sampling & don't hold pin value
        AD1CON1bits.ADON = 1;           // enable A/D module
}

/**
 * Calculates voltage, using ADC values measured previously
 * @param step_value - ADC value
 * @return voltage value in volts
 */
static double calc_voltage(float step_value) {
        static const float vref_plus = 3;   // Nathan's PC - 3.25;
        static const float vref_neg = 0.1f; // Noise measurements from breadboard
        static const float max_step = 1023; // float to prevent truncation
        static const float real_ground = 0.05f; // measured ground

        return vref_neg + ((vref_plus - vref_neg) *
                        ( (float) step_value / max_step)) - real_ground;
}

/**
 * calculates capacitance with dt, dV, and current. Using the following 
 * function: I = C*(dV/dt)
 * @param time_us - dt
 * @param current_uA - current
 * @param voltage - voltage calculated previously
 * @return - capacitance value in F
 */
double calc_capacitance(float time_us, float current_uA, double voltage) {
        float time_s = time_us / 1000000.0f;
        float current_A = current_uA / 1000000.0f;
        return (time_s / voltage) * current_A;
}

static void print_results(double voltage, double capacitance) {
        XmitUART2('\r', 1);
        XmitUART2('\n', 1);

        // print voltage
        char *voltage_s[100];
        sprintf(voltage_s, "%g", voltage);

        Disp2String(" voltage: ");
        Disp2String(voltage_s);
        Disp2String("V     ");

        // print capacitance
        char *unit[2];
        
        if(capacitance < NANO){
            //pico
            capacitance /= PICO;
            strcpy(unit, "pF"); //unit = "pF";
        }
        else if (capacitance < MICRO){
            //nano
            capacitance /= NANO;
            strcpy(unit, "nF");
        }
        else if (capacitance < MILI){
            //micro
            capacitance /= MICRO;
            strcpy(unit, "uF");
        }
        else{
            //print error
            strcpy(unit, "??");
        }
        
        char *capacitance_s[100];
        strcat(capacitance_s, "capacitance: ");
        strcat(capacitance_s, sprintf(capacitance_s, "%g", capacitance));
        strcat(capacitance_s, unit);
        Disp2String(capacitance_s);

        int i;
        for (i = 0; i < 20; i++) {
                XmitUART2(' ', 1);      // empty spaces for neat output
        }
}

/**
 * converts the wanted amount of microseconds to cycles, assuming 8MHz clock
 * @param time_us - desired time in uS
 * @return number of cycles
 */
static inline long int us_to_cycles(float time_us) {
        double clock_freq = 8000000; // recall ints are 16 bit on this chip
        double prescaling_factor = 1.0;
        const float kMagicNumber = 1.0/2.0;

        return (clock_freq / US_PER_S) * prescaling_factor *
                        kMagicNumber * time_us;
}

/**
 * Converts sampled value of ADC and discharges the capacitor
 * @param time_us - for capacitor discharging time
 * @return the ADC value (step_value) as integer
 */
static int sampleVoltage(float time_us){
        // turn off current
        CTMUCONbits.EDG2STAT = 0;       // 00 = current source is off
        CTMUCONbits.EDG1STAT = 0;       //    ^- as a combo ONLY

        /*
        * this does a lot:
        *       - stop sampling input
        *       - begin conversion
        *       - 'hold' the capacitor value until set to 1 again
        */
        AD1CON1bits.SAMP = 0;

        // wait for done bit
        static const int kADCDone = 1;
        while (AD1CON1bits.DONE != kADCDone) {} // busy wait until conversion is complete

        int step_value = ADC1BUF0 & 0x03ff; // stores last 10 bits of sample result
 
        AD1CON1bits.SAMP = 1; // 1 = stop holding cap value
        CTMUCONbits.IDISSEN = 1; // discharge the capacitor during printing
        __delay32(time_us*8);   // recalculate the cycles, assuming 8MHz clock
        return step_value;
}

/**
 * Runs a test voltage measurement to decide which current and time to use,
 * Adopts accordingly by figuring out the amount of voltage generated. For 
 * big capacitors, the voltage would be low, so increase the time. For low
 * capacitors the voltage will hit the rail, so decrease the current and time. 
 * For low capacitors, if the voltage is below 1V, increment time and measure
 * again. For mid range capacitors, just calculate the capacitance. 
 */
void sample_capacitance_adaptive(){
        float time_us = 5000; // test value to decide which current to use
        long int cycles = us_to_cycles(time_us);
        float current_uA = 55; // initial test current
        // calculated for 5ms, 55uA, and 1uF, used as Vref
        double expectedVoltageValue = 0.3f; // limiting value for lower caps
        double capacitance = 0;
        
        // adapt the current and time
        // first measure one time
        start_current_source(current_uA);
        __delay32(cycles);
        int step_value = sampleVoltage(time_us);
        double voltage = calc_voltage(step_value);

        // change the current and measure again if needed
        if (voltage < 0.10){    // if the voltage is too low, the cap is big
            time_us = 1500000;  // testing time for big caps
            cycles = us_to_cycles(time_us);
            start_current_source(current_uA);
            __delay32(cycles);
            int step_value = sampleVoltage(time_us);
            double voltage2 = calc_voltage(step_value);
            // calculate the capacitance with 2 different voltage values
            capacitance = calc_capacitance(time_us, current_uA, (voltage2 - voltage));
        }
        else if (voltage > expectedVoltageValue){
            current_uA = 5.5;      // lower the current for smaller caps
            time_us = 37;          // calculated for 100pF - 37us
            cycles = us_to_cycles(time_us);
            start_current_source(current_uA);
            __delay32(cycles);
            int step_value = sampleVoltage(time_us);
            voltage = calc_voltage(step_value);
            // increase time until the voltage reaches 1V to avoid noise floor
            while (voltage < 1){
                time_us += time_us;
                cycles = us_to_cycles(time_us);
                start_current_source(current_uA);
                __delay32(cycles);
                int step_value = sampleVoltage(time_us);
                voltage = calc_voltage(step_value);
            }
            capacitance = calc_capacitance(time_us, current_uA, voltage);
        }
        else{ // when the cap is around 1uF
            capacitance = calc_capacitance(time_us, current_uA, voltage);
        }
        
        print_results(voltage, capacitance);
}
