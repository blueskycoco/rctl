#include <msp430g2553.h>
#include <stdio.h>
#include <stdint.h>
#include "cc1101_def.h"
#include "cc1101.h"
#include "dbg.h"
int main(void) {
	int i=0;
	unsigned char txBuffer[TX_BUF_SIZE];
	unsigned short rx_length;
	WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

	BCSCTL1 = CALBC1_16MHZ; 		// Set oscillator to 16MHz
	DCOCTL = CALDCO_16MHZ;  		// Set oscillator to 16MHz

	//uart_init();
	radio_init();
	while (1) {
		rx_length = TX_BUF_SIZE;
		if (CRC_OK == radio_read(txBuffer,&rx_length)) {
			P1OUT |= BIT4;
			radio_send(txBuffer,rx_length);
			P1OUT &= ~BIT4;
			}
	}
	__bis_SR_register(GIE + LPM4_bits);
	return 0;
}
