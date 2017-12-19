#include <msp430g2553.h>
#include <stdio.h>
#include <stdint.h>
#include "cc1101_def.h"
#include "cc1101.h"
#include "dbg.h"
#if 0
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
#endif
int main(void) {
	int i=0;
	unsigned char txBuffer[TX_BUF_SIZE];
	unsigned short rx_length;
	WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

	// configure clocks;
	BCSCTL1 = CALBC1_16MHZ; 		// Set oscillator to 16MHz
	DCOCTL = CALDCO_16MHZ;  		// Set oscillator to 16MHz

	uart_init();
	radio_init();
	while (1) {
		#if 0
		int wait_time = radio_wait_for_idle(1024);
		//printf("wait time %d, %d\r\n", wait_time,i);
		i++;
		if( wait_time < 1024)
		{
			uart_write_string("have data in\r\n");
			rx_length = TX_BUF_SIZE;
			radio_read(txBuffer, &rx_length);
			if (rx_length >0)
				uart_write_string("vaild data\r\n");
			radio_send(txBuffer,rx_length);
			radio_wait_for_idle(0);
			trxSpiCmdStrobe(RF_SFRX);	
		} else if (wait_time == 1024)
			uart_write_string("no data in\r\n");
		else
			uart_write_string("no resaon\r\n");
		#endif
		//sprintf(txBuffer,"test data %djsldkjflksdjflksdjf\n", i++);
		memset(txBuffer, 0x30+i, TX_BUF_SIZE);
		i++;
		if (i==10)
			i=0;
		rx_length = TX_BUF_SIZE;//strlen(txBuffer);		
		radio_send(txBuffer,rx_length);
		//uart_write_string("sending data ..\r\n");
		//radio_wait_for_idle(0);
		uart_write_string("sending data ...\r\n");
		__delay_cycles(48000000);
	}
	__bis_SR_register(GIE + LPM4_bits);
	return 0;
}
