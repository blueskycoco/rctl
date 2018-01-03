#include <msp430g2553.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc1101_def.h"
#include "cc1101.h"
#include "task.h"
/* process
  * 1 read short or long pressed
  * 2 short pressed, send bat adc, key name to stm32
  * 3 long pressed, send code to stm32
  * 4 power off
  */
  
#define     LED_SEL            P2SEL
#define     LED_OUT            P2OUT
#define     LED_DIR            P2DIR
#define     LED_N_PIN          BIT4
#define     KEY_SEL            P1SEL
#define     KEY_IN             P1IN
#define     KEY_DIR            P1DIR
#define     KEY_N_PIN0         BIT1
#define     KEY_N_PIN1         BIT2
#define     KEY_N_PIN2         BIT3
#define     POWER_SEL		   P1SEL
#define 	POWER_OUT		   P1OUT
#define   	POWER_DIR		   P1DIR
#define 	POWER_N_PIN		   BIT4
void task()
{	
	unsigned char cmd[20] = {0x33};
	unsigned short cmd_len = 0;
	int i=0,j=0,k=0;
	POWER_SEL &= ~POWER_N_PIN;
	POWER_DIR |= POWER_N_PIN;
	POWER_OUT |= POWER_N_PIN;
	LED_SEL &= ~LED_N_PIN;
	LED_DIR |= LED_N_PIN;
	LED_OUT |= LED_N_PIN;
	KEY_SEL &= ~KEY_N_PIN0;
	KEY_DIR &= ~KEY_N_PIN0;
	KEY_SEL &= ~KEY_N_PIN1;
	KEY_DIR &= ~KEY_N_PIN1;
	KEY_SEL &= ~KEY_N_PIN2;
	KEY_DIR &= ~KEY_N_PIN2;
	while (!(KEY_IN & KEY_N_PIN0) ||
		!(KEY_IN & KEY_N_PIN0) 	 ||
		!(KEY_IN & KEY_N_PIN0)) {
		if (!(KEY_IN & KEY_N_PIN0))
			i++;
		if (!(KEY_IN & KEY_N_PIN1))
			j++;
		if (!(KEY_IN & KEY_N_PIN2))
			k++;
		__delay_cycles(16000);
		if (i>2000 || j>2000 || k>2000)
			break;
	}
	LED_OUT &= ~LED_N_PIN;
	if ((i>0 && j>0) || (i>0 && k>0) || (j>0&&k>0) ||
		(i>0&&j>0&&k>0)) {
		POWER_OUT &= ~POWER_N_PIN;
		return;
	}

	if (i>2000 || j>2000 || k>2000) {
	/*code verification*/
	
	} else {
	/*normal process*/
	
	}
	LED_OUT |= LED_N_PIN;
	radio_init();	
	radio_send(cmd, cmd_len);
	LED_OUT &= ~LED_N_PIN;
	POWER_OUT &= ~POWER_N_PIN;
	return ;
}
