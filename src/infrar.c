#include <msp430g2553.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc1101_def.h"
#include "cc1101.h"
#include "task.h"
#define DEST_ADDR	0x01
#define SRC_ADDR	0x02
#define LED_SEL         P1SEL
#define LED_OUT         P1OUT
#define LED_DIR         P1DIR
#define LED_N_PIN       BIT3
volatile int i=0;
volatile uint8 softDog = 0;
void __attribute__ ((interrupt(TIMER0_A1_VECTOR))) Timer_A (void)
{  	
	switch( TA0IV )	
	{
		case  2:  break;
		case  4:  break;
		case 10:  
			{
				if (++softDog < 20)
					WDTCTL = WDT_ARST_1000;				
				i++;
				if (i == 10) {
				i=0;
				__bic_SR_register_on_exit(LPM3_bits);			
				}
			}
		break;
	}
}
void task()
{		
	int i=0;
	unsigned char cmd[12] = {0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x37};
	unsigned char cmd1[20];
	unsigned short len = 10;
	LED_SEL &= ~LED_N_PIN;
	LED_DIR |= LED_N_PIN;
	LED_OUT |= LED_N_PIN;
	__delay_cycles(500000);
	LED_OUT &= ~LED_N_PIN;
	__delay_cycles(500000);
	LED_OUT |= LED_N_PIN;

	radio_init();
	CCR0 = 8192;
	TACTL = TASSEL_1 + MC_1 + TAIE + ID0;
	while (1) {
		__bis_SR_register(LPM3_bits + GIE);
		LED_OUT ^= LED_N_PIN;
		if (i==10)
		i=0;
		len = 10;
		/*memset(cmd+2,0x30+i,len);
		cmd[0] = DEST_ADDR;
		cmd[1] = SRC_ADDR;
		len += 2;*/
		memset(cmd,0x30+i,len);
		radio_send(cmd,len);
		//radio_read(cmd1,&len);
		//if (memcmp(cmd,cmd1,10) !=0 || len != 10)
		//	LED_OUT |= LED_N_PIN;		
		radio_sleep();
		softDog = 0;
		i=i+1;
	}
	return ;
}
