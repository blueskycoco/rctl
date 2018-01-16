#include <msp430g2553.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc1101_def.h"
#include "cc1101.h"
#include "task.h"
#define LED_SEL         P1SEL
#define LED_OUT         P1OUT
#define LED_DIR         P1DIR
#define LED_N_PIN       BIT0
void __attribute__ ((interrupt(TIMER0_A1_VECTOR))) Timer_A (void)
{  	
	switch( TA0IV )	
	{
		case  2:  break;
		case  4:  break;
		case 10:  
			{
				__bic_SR_register_on_exit(LPM0_bits);			
			}
		break;
	}
}
void task()
{		
	int i=0;
	unsigned char cmd[10] = {0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x37};
	unsigned char cmd1[20];
	unsigned short len = 10;
	LED_SEL &= ~LED_N_PIN;
	LED_DIR |= LED_N_PIN;
	LED_OUT &= ~LED_N_PIN;

	radio_init();
	TACTL = TASSEL_2 + MC_2 + TAIE + ID0;
	while (1) {
		__bis_SR_register(LPM0_bits + GIE);
		LED_OUT |= LED_N_PIN;	
		if (i==10)
			i=0;
		memset(cmd,0x30+i,len);
		radio_send(cmd,len);
		radio_sleep();
		LED_OUT &= ~LED_N_PIN;	
		i=i+2;
	}
	return ;
}
