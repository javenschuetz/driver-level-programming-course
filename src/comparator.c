// libraries and header
#include "Comparator.h"
#include <p24F16KA101.h>

// project files
#include "UART2.h"
#include "button_state.h"

static volatile unsigned int count = 0;
int volatile countTarget = 0;



// *********************************************************************** cvref
void CVREFinit (float vref) {
        // clamp Vref between 0 and 2.37
        if (vref < 0 ) {
            vref = 0;
        }

        if (vref > 2.37) {
            vref = 2.37;
        }

        // CVRR has to be 1 in this case, and the output uses a different formula based on CVRR
        // CVR and CVRR combined determines ref voltage
        if (vref <= 0.83) {
                CVRCONbits.CVRR = 1; // supports 0 to 0.83
                CVRCONbits.CVR = vref * 24.0f / 3.2f; // 3.2 is because 3.2 V is what our 3.5V setting outputs
        } else {
                CVRCONbits.CVRR = 0; // supports 0.83 to 2.37
                CVRCONbits.CVR = (vref - (3.2f / 4.0f)) * 32.0f / 3.2f;
        }

        CVRCONbits.CVRSS = 0; // CVRSS 0 means "use AVdd - AVss" for reference source

        CVRCONbits.CVREN = 1; // reference enable
        CVRCONbits.CVROE = 1; // vref output enable (pin 17)

        // Disp2Hex(CVRCONbits.CVR);   // debug message
        // Disp2Hex(CVRCONbits.CVRR);  // debug message
}



// ****************************************************************** Comparator
void ComparatorInit(void){
    CM2CONbits.COE = 1;     //enables output on comparator
    CM2CONbits.CREF = 1;    //sets Vref to non-inverting input
    CM2CONbits.CCH = 0b01;  //external input to inverting input
    IPC4bits.CMIP = 0x2;    //sets comparator priority

    CM2CONbits.CEVT = 0;    //clears interrupt flag if was not previously
    CM2CONbits.CON = 1;     //enables comparator
    IEC1bits.CMIE = 1;      //enables comparator interrupt
    IFS1bits.CMIF = 0;      // clear IF flag

    CM2CONbits.CPOL = 0;     //output polarity is set
    CM2CONbits.EVPOL = 0b11; //generate interrupt on any change
}

void __attribute__((interrupt, no_auto_psv)) _CompInterrupt(void) {
        // left here for further implementation
        //    if(CMSTATbits.C1OUT == 1){ // If interrupt due to Comparator 1
        //        //Do nothing
        //    }
        // increase the count every interrupt
        count++;
        // if count equal to the desired division value
        if (count >= countTarget){
                LATBbits.LATB9 = !LATBbits.LATB9;   // toggle LED
                count = 0;                          // resets the counter
        }
        IFS1bits.CMIF = 0;              // clear IF flag
        CM2CONbits.CEVT = 0; // Interrupts disabled till this bit is cleared
        Nop();
}
