#include <msp430g2553.h>
#include <stdint.h>
#include "cc1101.h"
#include "dbg.h"
// Vector table handlers
typedef void vector_handler(void);
vector_handler *APP = (vector_handler *)0xC000;
vector_handler *PORT1_VECTOR_HANDLER = (vector_handler *)0x0202;
vector_handler *PORT2_VECTOR_HANDLER = (vector_handler *)0x0203;
vector_handler *ADC10_VECTOR_HANDLER = (vector_handler *)0x0205;
vector_handler *USCIAB0TX_VECTOR_HANDLER = (vector_handler *)0x0206;
vector_handler *USCIAB0RX_VECTOR_HANDLER = (vector_handler *)0x0207;
vector_handler *TIMER0_A1_VECTOR_HANDLER = (vector_handler *)0x0208;
vector_handler *TIMER0_A0_VECTOR_HANDLER = (vector_handler *)0x0209;
vector_handler *WDT_VECTOR_HANDLER = (vector_handler *)0x020A;
vector_handler *COMPARATORA_VECTOR_HANDLER = (vector_handler *)0x020B;
vector_handler *TIMER1_A1_VECTOR_HANDLER = (vector_handler *)0x020C;
vector_handler *TIMER1_A0_VECTOR_HANDLER = (vector_handler *)0x020D;
vector_handler *NMI_VECTOR_HANDLER = (vector_handler *)0x020E;

__attribute__((interrupt(PORT1_VECTOR))) void PORT1_ISR(void);
__attribute__((interrupt(PORT2_VECTOR))) void PORT2_ISR(void);
__attribute__((interrupt(ADC10_VECTOR))) void ADC10_ISR(void);
__attribute__((interrupt(USCIAB0TX_VECTOR))) void USCIAB0TX_ISR(void);
__attribute__((interrupt(USCIAB0RX_VECTOR))) void USCIAB0RX_ISR(void);
__attribute__((interrupt(TIMER0_A1_VECTOR))) void TIMER0_A1_ISR(void);
__attribute__((interrupt(TIMER0_A0_VECTOR))) void TIMER0_A0_ISR(void);
__attribute__((interrupt(WDT_VECTOR))) void WDT_ISR(void);
__attribute__((interrupt(COMPARATORA_VECTOR))) void COMPARATORA_ISR(void);
__attribute__((interrupt(TIMER1_A1_VECTOR))) void TIMER1_A1_ISR(void);
__attribute__((interrupt(TIMER1_A0_VECTOR))) void TIMER1_A0_ISR(void);
__attribute__((interrupt(NMI_VECTOR))) void NMI_ISR(void);

void PORT1_ISR(void) { PORT1_VECTOR_HANDLER(); }
void PORT2_ISR(void) { PORT2_VECTOR_HANDLER(); }
void ADC10_ISR(void) { ADC10_VECTOR_HANDLER(); }
void USCIAB0TX_ISR(void) { USCIAB0TX_VECTOR_HANDLER(); }
void USCIAB0RX_ISR(void) { USCIAB0RX_VECTOR_HANDLER(); }
void TIMER0_A1_ISR(void) { TIMER0_A1_VECTOR_HANDLER(); }
void TIMER0_A0_ISR(void) { TIMER0_A0_VECTOR_HANDLER(); }
void WDT_ISR(void) { WDT_VECTOR_HANDLER(); }
void COMPARATORA_ISR(void) { COMPARATORA_VECTOR_HANDLER(); }
void TIMER1_A1_ISR(void) { TIMER1_A1_VECTOR_HANDLER(); }
void TIMER1_A0_ISR(void) { TIMER1_A0_VECTOR_HANDLER(); }
void NMI_ISR(void) { NMI_VECTOR_HANDLER(); }

int main(void) {
	WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

	// configure clocks;
	BCSCTL1 = CALBC1_16MHZ; 		// Set oscillator to 16MHz
	DCOCTL = CALDCO_16MHZ;  		// Set oscillator to 16MHz

    // init flash memory control
    FCTL2 = FWKEY + FSSEL_2 + 0x30;

    // configure pins
    P1DIR = 0xFF;									// Set P1 to output direction
	P2DIR = 0xFF;									// Set P2 to output direction
	P3DIR = 0xFF;									// Set P3 to output direction

	P1OUT = 0x00;									// Set P1  outputs to logic 0
	P2OUT = 0x00;									// Set P2  outputs to logic 0
	P3OUT = 0x00;									// Set P3  outputs to logic 0

	uart_init();
	radio_init();
	while (1) {
		//uart_write_string((char *)"1 count ...\r\n");
		__delay_cycles(16000000);
	}
	__bis_SR_register(GIE + LPM4_bits);
	return 0;
}
