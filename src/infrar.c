#include <msp430g2553.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc1101_def.h"
#include "cc1101.h"
#include "task.h"
/* process
  * 1 key pressed , send register code
  * 2 tamper removed , send alarm
  * 3 in rcv status, GOD2 int trigger cc1101 rcv 
  * 4 loop get battery status
  */
#define ID_CODE			0x00000001
#define CMD_REG_CODE		0x0000
#define CMD_REG_CODE_ACK	0x0014
#define CMD_LOW_POWER	0x0C
#define DEVICE_TYPE		0x02
#define	CMD_PROTECT_ON	0x02
#define	CMD_PROTECT_OFF	0x04
#define	CMD_ALARM		0x06
#define	CMD_MUTE		0x0e
#define MSG_HEAD0		0x6c
#define MSG_HEAD1		0xaa
#define PACKAGE_LEN		16
#define DATA_LEN		9
#define LED_SEL         P1SEL
#define LED_OUT         P1OUT
#define LED_DIR         P1DIR
#define LED_N_PIN       BIT3

#define S1_KEY_SEL      P1SEL
#define S1_KEY_IN       P1IN
#define S1_KEY_DIR      P1DIR
#define S1_KEY_REN      P1REN
#define S1_KEY_IE      	P1IE
#define S1_KEY_IES      P1IES
#define S1_KEY_IFG      P1IFG
#define S1_KEY_N_PIN    BIT2

#define INFRAR_KEY_SEL      P2SEL
#define INFRAR_KEY_IN       P2IN
#define INFRAR_KEY_DIR      P2DIR
#define INFRAR_KEY_REN     	P2REN
#define INFRAR_KEY_IE  		P2IE
#define INFRAR_KEY_IES     	P2IES
#define INFRAR_KEY_IFG     	P2IFG
#define INFRAR_KEY_N_PIN    BIT4

#define CODE_KEY_SEL      	P1SEL
#define CODE_KEY_IN       	P1IN
#define CODE_KEY_DIR      	P1DIR
#define CODE_KEY_REN      	P1REN
#define CODE_KEY_IE      	P1IE
#define CODE_KEY_IES      	P1IES
#define CODE_KEY_IFG      	P1IFG
#define CODE_KEY_N_PIN    	BIT1

#define INFRAR_POWER_SEL    P2SEL
#define INFRAR_POWER_OUT    P2OUT
#define INFRAR_POWER_DIR    P2DIR
#define INFRAR_POWER_N_PIN  BIT3

#define KEY_CODE	0x01
#define KEY_INFRAR	0x02
#define KEY_S1		0x04
#define KEY_TIMER 	0x08
#define KEY_WIRELESS	0x10

#define STATE_ASK_CC1101_ADDR		0
#define STATE_CONFIRM_CC1101_ADDR	1
#define STATE_PROTECT_ON			2
#define STATE_PROTECT_OFF			3
unsigned char g_state = STATE_ASK_CC1101_ADDR;
#define MIN_BAT		0x96
unsigned char b_protection_state = 0;	/*protection state*/
volatile unsigned char key = 0x0;
unsigned char stm32_id[6] = {0};
unsigned char cc1101_addr = 0;
#define STM32_ADDR	0x01
void __attribute__ ((interrupt(TIMER0_A1_VECTOR))) Timer_A (void)
{  	
	switch( TA0IV )	
	{
		case  2:  break;
		case  4:  break;
		case 10:  
			{
					key |= KEY_TIMER;
					__bic_SR_register_on_exit(LPM3_bits);
			}
		break;
	}
}
void __attribute__ ((interrupt(PORT1_VECTOR))) Port_1 (void)
{  
	if (CODE_KEY_IFG & CODE_KEY_N_PIN )
	{
		key |= KEY_CODE;
		//CODE_KEY_IFG &= ~CODE_KEY_N_PIN;
	}
	
	if (S1_KEY_IFG & S1_KEY_N_PIN )
	{
		key |= KEY_S1;
		//S1_KEY_IFG &= ~S1_KEY_N_PIN;
	}

	if ((key & KEY_CODE) || (key & KEY_S1))
		__bic_SR_register_on_exit(LPM3_bits);
}	
void __attribute__ ((interrupt(PORT2_VECTOR))) Port_2 (void)
{  
	if (INFRAR_KEY_IFG & INFRAR_KEY_N_PIN )
	{
		key |= KEY_INFRAR;
		//INFRAR_KEY_IFG &= ~INFRAR_KEY_N_PIN;
		__bic_SR_register_on_exit(LPM3_bits);
	}
	
	if (INFRAR_KEY_IFG & BIT0 )
	{
		key |= KEY_WIRELESS;
		//INFRAR_KEY_IFG &= ~BIT0;
		__bic_SR_register_on_exit(LPM3_bits);
	}
}

void request_cc1101_addr() 
{	
	unsigned char cmd[PACKAGE_LEN] = {0x00};
	unsigned short cmd_len = PACKAGE_LEN;
	cmd[0] = STM32_ADDR;cmd[1] = cc1101_addr;
	cmd[2] = MSG_HEAD0;cmd[3] = MSG_HEAD1;
	cmd[4] = DATA_LEN; cmd[5] = (CMD_REG_CODE >> 8) & 0xff;
	cmd[6] = CMD_REG_CODE & 0xff;
	cmd[7] = DEVICE_TYPE;
	cmd[8] = ((long)ID_CODE >> 24) & 0xff;
	cmd[9] = ((long)ID_CODE >> 16) & 0xff;
	cmd[10] = ((long)ID_CODE >> 8) & 0xff;
	cmd[11] = ((long)ID_CODE >> 0) & 0xff;
	unsigned short bat = read_adc();
	cmd[12] = (bat >> 8) & 0xff;
	cmd[13] = (bat) & 0xff;	
	unsigned short crc = CRC(cmd, PACKAGE_LEN - 2);
	cmd[PACKAGE_LEN - 2] = (crc >> 8) & 0xff;
	cmd[PACKAGE_LEN - 1] = (crc) & 0xff;
	radio_send(cmd, cmd_len);
	
	P2REN &= ~BIT0;
	P2IES &= ~BIT0;
	P2IFG &= ~BIT0;
	P2IE  |= BIT0;
}
void handle_cc1101()
{
	unsigned char resp[32] = {0};
	unsigned char len = 32;
	int result = radio_read(resp, &len);
	
	if (result !=0 && len > 0) {
		if (resp[2] != MSG_HEAD0 || resp[3] != MSG_HEAD1)
			return ;
		unsigned short crc = CRC(resp, resp[4]+5);
		/*check crc*/
		if (crc != (resp[resp[4]+4] << 8 | resp[resp[4]+5]))
			return ;
		/*check subdevice id = local device id*/
		if (memcmp(ID_CODE, resp+14, 4) !=0)
			return ;
	}

	switch (g_state) {
		case STATE_ASK_CC1101_ADDR:
			if ((resp[5]<<8|resp[6]) == CMD_REG_CODE_ACK) {
				if (resp[7] == 0x01 && cc1101_addr == 0) {
					memcpy(stm32_id, resp+8, 8);
					cc1101_addr = resp[20];
				}
			}
	}	
}
void task()
{		
	int i=0;
	unsigned char cmd[10] = {0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x37};
	unsigned char cmd1[20];
	unsigned short len = 10;
	unsigned short bat = 0;
	LED_SEL &= ~LED_N_PIN;
	LED_DIR |= LED_N_PIN;
	LED_OUT &= ~LED_N_PIN;

	S1_KEY_SEL &= ~S1_KEY_N_PIN;
	S1_KEY_DIR &= ~S1_KEY_N_PIN;
	S1_KEY_REN &= ~S1_KEY_N_PIN;
	S1_KEY_IE  |= S1_KEY_N_PIN;
	S1_KEY_IES &= ~S1_KEY_N_PIN;
	S1_KEY_IFG &= ~S1_KEY_N_PIN;

	CODE_KEY_SEL &= ~CODE_KEY_N_PIN;
	CODE_KEY_DIR &= ~CODE_KEY_N_PIN;
	CODE_KEY_REN &= ~CODE_KEY_N_PIN;
	CODE_KEY_IE  |= CODE_KEY_N_PIN;
	CODE_KEY_IES |= CODE_KEY_N_PIN;
	CODE_KEY_IFG &= ~CODE_KEY_N_PIN;

	INFRAR_KEY_SEL &= ~INFRAR_KEY_N_PIN;
	INFRAR_KEY_DIR &= ~INFRAR_KEY_N_PIN;
	INFRAR_KEY_REN &= ~INFRAR_KEY_N_PIN;
	INFRAR_KEY_IE  |= INFRAR_KEY_N_PIN;
	INFRAR_KEY_IES |= INFRAR_KEY_N_PIN;
	INFRAR_KEY_IFG &= ~INFRAR_KEY_N_PIN;
	
	INFRAR_POWER_SEL &= ~INFRAR_POWER_N_PIN;
	INFRAR_POWER_DIR |= INFRAR_POWER_N_PIN;
	INFRAR_POWER_OUT |= INFRAR_POWER_N_PIN;
	TACTL = TASSEL_1 + MC_2 + TAIE + ID0;
	radio_init();
	request_cc1101_addr();
	while (1) {
		__bis_SR_register(LPM3_bits + GIE);
		_DINT();
		NOP();
		if (key & KEY_TIMER) {
			key &= ~KEY_TIMER;
			handle_timer();
			//radio_send(cmd,len);
			//radio_sleep();
		}

		if (key & KEY_CODE) {
			key &= ~KEY_CODE;
			/*send machine code to stm32*/
			handle_code();
			CODE_KEY_IFG &= ~CODE_KEY_N_PIN;
		}

		if (key & KEY_S1) {
			key &= ~KEY_S1;
			/*send s1 alarm to stm32*/
			handle_s1();
			S1_KEY_IFG &= ~S1_KEY_N_PIN;
		}

		if (key & KEY_INFRAR) {
			key &= ~KEY_INFRAR;
			/*send infrar alarm to stm32*/
			handle_infrar();
			INFRAR_KEY_IFG &= ~INFRAR_KEY_N_PIN;
		}

		if (key & KEY_WIRELESS) {
			key &= ~KEY_WIRELESS;
			/*new data come from stm32*/
			handle_cc1101();
			INFRAR_KEY_IFG &= ~BIT0;
		}
		_EINT();
	}
	return ;
}
