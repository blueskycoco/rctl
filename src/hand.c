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
#define ID_CODE			0x000001
#define DEVICE_TYPE		0x01
#define	CMD_PROTECT_ON	0x01
#define	CMD_PROTECT_OFF	0x02
#define	CMD_ALARM		0x03
#define	CMD_MUTE		0x04
#define MSG_HEAD0		0x6c
#define MSG_HEAD1		0xaa
#define PACKAGE_LEN		12
#define DATA_LEN		7
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
void task()
{	
	unsigned char cmd[PACKAGE_LEN] = {0x00};
	unsigned short cmd_len = PACKAGE_LEN;
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
	cmd[0] = MSG_HEAD0;cmd[1] = MSG_HEAD1;
	cmd[2] = DATA_LEN; cmd[4] = DEVICE_TYPE;
	cmd[5] = (ID_CODE >> 16) & 0xff;
	cmd[6] = (ID_CODE >> 8) & 0xff;
	cmd[7] = (ID_CODE >> 0) & 0xff;
	/*get battery val*/
	
	if (i>2000 || j>2000 || k>2000) {
	/*code verification*/
	
	} else {
	/*normal process*/
	
	}
	unsigned short crc = CRC(cmd, PACKAGE_LEN - 2);
	cmd[PACKAGE_LEN - 2] = (crc >> 8) & 0xff;
	cmd[PACKAGE_LEN - 1] = (crc) & 0xff;
	LED_OUT |= LED_N_PIN;
	radio_init();	
	radio_send(cmd, cmd_len);
	LED_OUT &= ~LED_N_PIN;
	POWER_OUT &= ~POWER_N_PIN;
	return ;
}
