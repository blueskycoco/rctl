#include <msp430g2553.h>
#include <stdint.h>
#include "dbg.h"
// uart functions
void uart_write_char(char c) {
    while (!(IFG2 & UCA0TXIFG));
    UCA0TXBUF = c;
}

void uart_write_string(char *buf) {
    while (*buf) uart_write_char(*buf++);
}
void uart_init(void)
{
	// init uart
	P1SEL |= BIT1 | BIT2;
    P1SEL2 |= BIT1 | BIT2;
    UCA0CTL1 |= UCSSEL_2; // Use SMCLK = 16MHz
	UCA0BR1 = 0;
	UCA0BR0 = 138;
	UCA0MCTL = UCBRS_7 | UCBRF_0;
	UCA0CTL1 &= ~UCSWRST; // Initialize USCI state machine
    //IE2 |= UCA0RXIE; // enable RX interrupt
	uart_write_string((char *)"System <> Booting ...\r\n");
    //USCIAB0RX_VECTOR_HANDLER = USCI0RX_ISR;
    //__bis_SR_register(GIE);
    //__delay_cycles(16000000);					// Sleep 3 seconds and try to enter to bootloader
}
