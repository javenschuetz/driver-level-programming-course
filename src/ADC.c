// libraries and header
#include "xc.h"
#include "ADC.h"

// drivers
#include "UART2.h" // for testing / debugging only
#include "ChangeClk.h"

// other dependencies

// Magic Numbers
static const char kEnable = 1;
static const char kDisable = 0;

// statics

// callback
static void (* todo_callback)(void); // my callback!


void init_ADC(void) {
        // echona & chosea

        // note - an5 is RA3 / pin 8
        static const int kChannel1NegIsVrNeg = 0;
        AD1CHSbits.CH0NB = kChannel1NegIsVrNeg;
        AD1CHSbits.CH0SB = 0b0101; // channel 0 pos input is AN5
        AD1CHSbits.CH0NA = kChannel1NegIsVrNeg;
        AD1CHSbits.CH0SA = 0b0101; // channel 0 pos input is AN5

        // config an5 as input

        // me
        AD1CON1bits.SSRC = 0b111; // todo - check this, its for software triggered adc

        // pcfgs
        static const int kPinInputMode = 1;
        TRISAbits.TRISA3 = kPinInputMode;
        // todo - use if block?
        AD1PCFGbits.PCFG5 = 0; // warning - do not set before setting pin to input

        // config vref+ and - (vcfg)
        static const int kUseAvddAvss = 0b000;
        AD1CON2bits.VCFG = kUseAvddAvss;

        // form ??
        static const int kInteger = 0b00;
        AD1CON1bits.FORM = kInteger;

        // ASAM
        static const int kSamplingBeginsWhenSampSet = 0;
        AD1CON1bits.ASAM = kSamplingBeginsWhenSampSet;

        // scan bits
        static const int kDontScanInputs = 0;
        AD1CON2bits.CSCNA = kDontScanInputs;

        //smpi
        static const int kInterruptAfterEveryConversion = 0b0000;
        AD1CON2bits.SMPI = kInterruptAfterEveryConversion;

        //bufm
        static const int k16BitBuffer = 0;
        AD1CON2bits.BUFM = k16BitBuffer;

        //alts
        static const int kAlwaysUseMuxA;
        AD1CON2bits.ALTS = kAlwaysUseMuxA;

        //adc timing
        static const int kUseInternalRC = 1;
        static const int kUseSysDerivedClock = 0;

        AD1CON3bits.ADRC = kUseSysDerivedClock; // might need to toggle this sometimes
        AD1CON3bits.SAMC = 0b11111; // determines sample rate

        // ADON
        AD1CON1bits.ADON = 1; // turns ADC on
}

// todo - rename
void do_ADC(void) {
        AD1CON1bits.SAMP = 1;

        // check for done bit
        static const int kADCDone = 1;
        while (AD1CON1bits.DONE != kADCDone) {
                // Nop();
        }
        AD1CON1bits.SAMP = 0;

        int buffer_value = 0;
        buffer_value = ADC1BUF0;
        XmitUART2('\r', 1);

        int i;

        // XmitUART2('\r', 1);

        for (i = 0; i < buffer_value / 30; i++) {
                XmitUART2(220, 1);
        }

        Disp2Hex(buffer_value);

        for (i = 0; i < 20; i++) {
                XmitUART2(' ', 1);
        }

}
