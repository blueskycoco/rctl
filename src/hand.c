#include <msp430g2553.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc1101_def.h"
#include "cc1101.h"
#include "task.h"
/* process
  * 1 check which key pressed
  * 2 if mid key pressed , check shor press or long press
  * 3 get battery val
  * 4 send out
  */
#define ID_CODE			0x0000000a
#define DEVICE_MODEL	0xD001
#define DEVICE_TIME		0x180622
#define DEVICE_TYPE		0x01

#define	CMD_PROTECT_ON	0x02
#define	CMD_PROTECT_OFF	0x04
#define	CMD_ALARM		0x06
#define	CMD_MUTE		0x0e
#define MSG_HEAD0		0x6c
#define MSG_HEAD1		0xaa
#define PACKAGE_LEN		27
#define DATA_LEN		22
#define LED_SEL         P2SEL
#define LED_OUT         P2OUT
#define LED_DIR         P2DIR
#define LED_N_PIN       BIT4
#define KEY_SEL         P1SEL
#define KEY_IN          P1IN
#define KEY_DIR         P1DIR
#define KEY_N_PIN0      BIT1
#define KEY_N_PIN1      BIT2
#define KEY_N_PIN2      BIT3
#define POWER_SEL		P1SEL
#define POWER_OUT		P1OUT
#define POWER_DIR		P1DIR
#define POWER_N_PIN		BIT4
volatile unsigned int cnt = 0;
volatile unsigned int flag = 1;
volatile unsigned int g_i =0;
void __attribute__ ((interrupt(TIMER0_A1_VECTOR))) Timer_A (void)
{  	
	switch( TA0IV )	
	{
		case  2:  break;
		case  4:  break;
		case 10:  
			{
				g_i++;
				if (g_i> 10) {					
				__bic_SR_register_on_exit(LPM0_bits);
				}
			}
		break;
	}
}
void __attribute__ ((interrupt(PORT1_VECTOR))) Port_1 (void)
{  
	if (P1IFG & KEY_N_PIN1 )
	{
		P1IFG &= ~KEY_N_PIN1;
		__bic_SR_register_on_exit(LPM0_bits);
	}	
}	

void task()
{	
	unsigned char key = 0x00;
	unsigned char cmd[PACKAGE_LEN] = {0x00};
	unsigned short cmd_len = PACKAGE_LEN;
	int i=0;
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
	
	if (!(KEY_IN & KEY_N_PIN0))
		key = 0x01;
	if (!(KEY_IN & KEY_N_PIN1))
		key = 0x02;
	if (!(KEY_IN & KEY_N_PIN2))
		key = 0x04;

	if (key != 0x01 && key != 0x02 && key != 0x04)
	{
		LED_OUT &= ~LED_N_PIN;
		POWER_OUT &= ~POWER_N_PIN;
		return ;
	}
	radio_init();

	if (!(KEY_IN & KEY_N_PIN1))
	{
		P1REN &= ~KEY_N_PIN1;
		P1IES &= ~KEY_N_PIN1;
		P1IFG &= ~KEY_N_PIN1;
		P1IE  |= KEY_N_PIN1;
		CCR0 = 16384;
		TACTL = TASSEL_2 + MC_1 + TAIE;
		__bis_SR_register(LPM0_bits + GIE);
		if (!(KEY_IN & KEY_N_PIN1))
			key = 0x08;
		TACTL = MC_0;
	}
/* msp430 -> stm32
  * 0x01 0x00 MSG_HEAD0 MSG_HEAD1 LEN STM32_ID SUB_ID CMD TYPE MODEL TIME BAT CRC
  * 1       1       1                  1                  1     6              4           2      1      2          3      2     2
  */
	cmd[0] = 0x01; cmd[1] = 0x00;
	cmd[2] = MSG_HEAD0;cmd[3] = MSG_HEAD1;
	cmd[4] = DATA_LEN; cmd[5] = 0x00;cmd[6] = 0x00;cmd[7] = 0x00;cmd[8] = 0x00;cmd[9] = 0x00;cmd[10] = 0x00;
	cmd[11] = ((long)ID_CODE >> 24) & 0xff;
	cmd[12] = ((long)ID_CODE >> 16) & 0xff;
	cmd[13] = ((long)ID_CODE >> 8) & 0xff;
	cmd[14] = ((long)ID_CODE >> 0) & 0xff;
	cmd[15] = 0;
	cmd[17] = DEVICE_TYPE;
	cmd[18] = (DEVICE_MODEL>>8)&0xff;
	cmd[19] = DEVICE_MODEL&0xff;
	cmd[20] = (DEVICE_TIME>>16)&0xff;
	cmd[21] = (DEVICE_TIME>>8)&0xff;
	cmd[22] = DEVICE_TIME&0xff;
	unsigned short bat = read_adc();
	cmd[23] = (bat >> 8) & 0xff;
	cmd[24] = (bat) & 0xff;
	if (key == 0x01)
		cmd[16] = CMD_PROTECT_ON;
	else if (key == 0x08)
		cmd[16] = CMD_ALARM;
	else if (key == 0x04)
		cmd[16] = CMD_PROTECT_OFF;
	else
		cmd[16] = CMD_MUTE;
	unsigned short crc = CRC(cmd, PACKAGE_LEN - 2);
	cmd[25] = (crc >> 8) & 0xff;
	cmd[26] = (crc) & 0xff;
	LED_OUT |= LED_N_PIN;
	//radio_init();
	for (i=0;i<1;i++) {
	//memset(cmd, 0x36, cmd_len);
	radio_send(cmd, cmd_len);
	__delay_cycles(300000);
	}
	LED_OUT &= ~LED_N_PIN;
	POWER_OUT &= ~POWER_N_PIN;
	return ;
}
