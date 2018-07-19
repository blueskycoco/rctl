#include <msp430g2553.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc1101_def.h"
#include "cc1101.h"
#include "spi.h"
#include "task.h"
/* process
  * 1 key pressed , send register code
  * 2 tamper removed , send alarm
  * 3 in rcv status, GOD2 int trigger cc1101 rcv 
  * 4 loop get battery status
  */
#define DEVICE_MODE		0xD111
//#define DEVICE_TIME		0x180622

#define ID_CODE_LEN		4
#define STM32_CODE_LEN	6
#define CMD_REG_CODE		0x0000
#define CMD_REG_CODE_ACK	0x0001
#define CMD_CONFIRM_CODE	0x0014
#define CMD_CONFIRM_CODE_ACK	0x0015
#define CMD_ALARM		0x0006
#define CMD_ALARM_ACK	0x0007
#define CMD_LOW_POWER	0x000c
#define CMD_LOW_POWER_ACK	0x000d
#define CMD_CUR_STATUS	0x0010
#define CMD_CUR_STATUS_ACK	0x0011

#define DEVICE_TYPE		0x42
#define MSG_HEAD0		0x6c
#define MSG_HEAD1		0xaa
#ifdef SW_SPI
#define LED_SEL         P1SEL
#define LED_OUT         P1OUT
#define LED_DIR         P1DIR
#define LED_N_PIN       BIT5
#else
#define LED_SEL         P1SEL
#define LED_OUT         P1OUT
#define LED_DIR         P1DIR
#define LED_N_PIN       BIT3
#endif
#define S1_KEY_SEL      P1SEL
#define S1_KEY_IN       P1IN
#define S1_KEY_DIR      P1DIR
#define S1_KEY_REN      P1REN
#define S1_KEY_IE      	P1IE
#define S1_KEY_IES      P1IES
#define S1_KEY_IFG      P1IFG
#define S1_KEY_N_PIN    BIT2

#define LIGHT_SEL		P1SEL
#define LIGHT_IN		P1IN
#define LIGHT_DIR		P1DIR
#define LIGHT_N_PIN		BIT4
#define LIGHT_REN     	P1REN
#ifdef SW_SPI
#define INFRAR_KEY_SEL      P1SEL
#define INFRAR_KEY_IN       P1IN
#define INFRAR_KEY_DIR      P1DIR
#define INFRAR_KEY_REN     	P1REN
#define INFRAR_KEY_IE  		P1IE
#define INFRAR_KEY_IES     	P1IES
#define INFRAR_KEY_IFG     	P1IFG
#define INFRAR_KEY_N_PIN    BIT3
#define INFRAR_POWER_SEL    P1SEL
#define INFRAR_POWER_OUT    P1OUT
#define INFRAR_POWER_DIR    P1DIR
#define INFRAR_POWER_N_PIN  BIT6
#else
#define INFRAR_KEY_SEL      P2SEL
#define INFRAR_KEY_IN       P2IN
#define INFRAR_KEY_DIR      P2DIR
#define INFRAR_KEY_REN     	P2REN
#define INFRAR_KEY_IE  		P2IE
#define INFRAR_KEY_IES     	P2IES
#define INFRAR_KEY_IFG     	P2IFG
#define INFRAR_KEY_N_PIN    BIT4
#define INFRAR_POWER_SEL    P2SEL
#define INFRAR_POWER_OUT    P2OUT
#define INFRAR_POWER_DIR    P2DIR
#define INFRAR_POWER_N_PIN  BIT3
#endif
#define CODE_KEY_SEL      	P1SEL
#define CODE_KEY_IN       	P1IN
#define CODE_KEY_DIR      	P1DIR
#define CODE_KEY_REN      	P1REN
#define CODE_KEY_IE      	P1IE
#define CODE_KEY_IES      	P1IES
#define CODE_KEY_IFG      	P1IFG
#define CODE_KEY_N_PIN    	BIT1


#define KEY_CODE	0x01
#define KEY_INFRAR	0x02
#define KEY_S1		0x04
#define KEY_TIMER 	0x08
#define KEY_WIRELESS	0x10
//#define KEY_LOWPOWER	0x20
#define HAVE_LIGHT	1

#define STATE_ASK_CC1101_ADDR		0
#define STATE_CONFIRM_CC1101_ADDR	1
#define STATE_PROTECT_ON			2
#define STATE_PROTECT_OFF			3
unsigned char g_state = STATE_ASK_CC1101_ADDR;
#define MIN_BAT		0x96
unsigned char b_protection_state = 1;	/*protection state*/
unsigned char last_sub_cmd = 0x00; /*0x01 s1_alarm, 0x02 infrar_alarm, 0x04 low_power_alarm, 0x08 cur_status*/
volatile unsigned char key = 0x0;
unsigned char stm32_id[STM32_CODE_LEN] = {0};
unsigned char zero_id[STM32_CODE_LEN] = {0};
unsigned char cc1101_addr = 0;
#define STM32_ADDR	0x01
#define USE_SMCLK 0
int test_cnt = 0;
int resend_cnt = 0;
int timer_5s = 0;
#define SECS_2	1
#define MINS_5	150
int g_cnt = SECS_2;
uint8_t g_trigger = 0;
void __attribute__ ((interrupt(TIMER0_A1_VECTOR))) Timer_A (void)
{  	
	switch( TA0IV )	
	{
		case  2:  break;
		case  4:  break;
		case 10:  
			{
					test_cnt++;
					timer_5s++;
					if (test_cnt >= g_cnt) {
						test_cnt = 0;
						key |= KEY_TIMER;
						#if USE_SMCLK
						__bic_SR_register_on_exit(LPM0_bits);
						#else
						__bic_SR_register_on_exit(LPM3_bits);					
						#endif
					}
			}
		break;
	}
}
void __attribute__ ((interrupt(PORT1_VECTOR))) Port_1 (void)
{  
	if (CODE_KEY_IFG & CODE_KEY_N_PIN )
	{
		key |= KEY_CODE;
		CODE_KEY_IE  &= ~CODE_KEY_N_PIN;
	}
	
	if (S1_KEY_IFG & S1_KEY_N_PIN )
	{
		key |= KEY_S1;
		S1_KEY_IE  &= ~S1_KEY_N_PIN;
	}
#ifdef SW_SPI
	if (INFRAR_KEY_IFG & INFRAR_KEY_N_PIN )
	{
		key |= KEY_INFRAR;
		//INFRAR_KEY_IE  &= ~INFRAR_KEY_N_PIN;
		INFRAR_KEY_IFG &= ~INFRAR_KEY_N_PIN;
	}
#endif
#ifdef SW_SPI
	if ((key & KEY_CODE) || (key & KEY_S1) || (key & KEY_INFRAR))
#else
	if ((key & KEY_CODE) || (key & KEY_S1))
#endif
		#if USE_SMCLK
		__bic_SR_register_on_exit(LPM0_bits);
		#else
		__bic_SR_register_on_exit(LPM3_bits);
		#endif
}	
void __attribute__ ((interrupt(PORT2_VECTOR))) Port_2 (void)
{ 
#ifndef SW_SPI
	if (INFRAR_KEY_IFG & INFRAR_KEY_N_PIN )
	{
		key |= KEY_INFRAR;
		//INFRAR_KEY_IE  &= ~INFRAR_KEY_N_PIN;
		//LED_OUT |= LED_N_PIN;		
		//if (last_sub_cmd) {
		//	key |= KEY_TIMER;	
		//}
		INFRAR_KEY_IFG &= ~INFRAR_KEY_N_PIN;
	}
#endif
	if (P2IFG & BIT0 )
	{
		key |= KEY_WIRELESS;
		//P2IE  &= ~BIT0;
		P2IFG &= ~BIT0;
	}
#ifndef SW_SPI
	if ((key & KEY_WIRELESS) || (key & KEY_INFRAR))
#else
	if (key & KEY_WIRELESS)
#endif
	{
		#if USE_SMCLK
		__bic_SR_register_on_exit(LPM0_bits);
		#else
		__bic_SR_register_on_exit(LPM3_bits);
		#endif
	}
}
/*
  	msp430 -> stm32 
	0x01 cc1101_addr 0x6c 0xaa data_len stm32_id msp430_id cmd_type sub_cmd_type device_type battery crc
	0x01 0x00 MSG_HEAD0 MSG_HEAD1 LEN STM32_ID SUB_ID CMD TYPE MODEL TIME BAT CRC
*/
void handle_cc1101_addr(uint8_t *id, uint8_t res) 
{	
	unsigned char cmd[32] = {0x00};
	unsigned char ofs = 0;
	cmd[0] = STM32_ADDR;cmd[1] = cc1101_addr;
	cmd[2] = MSG_HEAD0;cmd[3] = MSG_HEAD1;
	ofs = 5;
	if (id == NULL) 
		memset(cmd+ofs, 0, STM32_CODE_LEN);
	else
		memcpy(cmd+ofs, id, STM32_CODE_LEN);
	ofs += STM32_CODE_LEN;	
	read_info(ADDR_SN, cmd+ofs, 4);
	ofs += 4;	
	//cmd[ofs++] = ((long)ID_CODE >> 24) & 0xff;
	//cmd[ofs++] = ((long)ID_CODE >> 16) & 0xff;
	//cmd[ofs++] = ((long)ID_CODE >> 8) & 0xff;
	//cmd[ofs++] = ((long)ID_CODE >> 0) & 0xff;
	if (id == NULL) {
		cmd[ofs++] = (CMD_REG_CODE >> 8) & 0xff;
		cmd[ofs++] = CMD_REG_CODE & 0xff;
		cmd[ofs++] = DEVICE_TYPE;
	} else {		
		cmd[ofs++] = (CMD_CONFIRM_CODE >> 8) & 0xff;
		cmd[ofs++] = CMD_CONFIRM_CODE & 0xff;
		cmd[ofs++] = res;
		cmd[ofs++] = cc1101_addr;
		cmd[ofs++] = DEVICE_TYPE;
		cmd[ofs++] = (DEVICE_MODE>>8)&0xff;
		cmd[ofs++] = DEVICE_MODE&0xff;
		read_info(ADDR_DATE, cmd+ofs, 3);
		ofs += 3;	
		//cmd[ofs++] = (DEVICE_TIME>>16)&0xff;
		//cmd[ofs++] = (DEVICE_TIME>>8)&0xff;
		//cmd[ofs++] = DEVICE_TIME&0xff;
	}
	unsigned short bat = read_adc();
	cmd[ofs++] = (bat >> 8) & 0xff;
	cmd[ofs++] = (bat) & 0xff;
	cmd[4] = ofs-3; 
	unsigned short crc = CRC(cmd, ofs);
	cmd[ofs++] = (crc >> 8) & 0xff;
	cmd[ofs++] = (crc) & 0xff;
	radio_send(cmd, ofs);
	if (id == NULL) {
		P2REN &= ~BIT0;
		//P2IES &= ~BIT0;
		P2IFG &= ~BIT0;
		P2IE  |= BIT0;
	}
}
/*
  	msp430 -> stm32 
	0x01 cc1101_addr 0x6c 0xaa data_len stm32_id msp430_id cmd_type sub_cmd_type device_type battery crc
*/
void handle_cc1101_cmd(uint16_t main_cmd, uint8_t sub_cmd) 
{	
	unsigned char cmd[32] = {0x00};
	unsigned char ofs = 0;
	P2IE  &= ~BIT0;
	cmd[0] = STM32_ADDR;cmd[1] = cc1101_addr;
	cmd[2] = MSG_HEAD0;cmd[3] = MSG_HEAD1;
	ofs = 5;
	memcpy(cmd+ofs, stm32_id, STM32_CODE_LEN);	
	ofs += STM32_CODE_LEN;
	read_info(ADDR_SN, cmd+ofs, 4);
	ofs += 4;	
	//cmd[ofs++] = ((long)ID_CODE >> 24) & 0xff;
	//cmd[ofs++] = ((long)ID_CODE >> 16) & 0xff;
	//cmd[ofs++] = ((long)ID_CODE >> 8) & 0xff;
	//cmd[ofs++] = ((long)ID_CODE >> 0) & 0xff;
	cmd[ofs++] = (main_cmd >> 8) & 0xff;
	cmd[ofs++] = main_cmd & 0xff;
	cmd[ofs++] = sub_cmd;

	if (main_cmd == CMD_ALARM) {
		if (sub_cmd == 0x01)
			last_sub_cmd |= 0x02;
		else if(sub_cmd == 0x02)
			last_sub_cmd |= 0x01;
	} else if (main_cmd == CMD_LOW_POWER) {
		last_sub_cmd |= 0x04;
	} 
	
	cmd[ofs++] = DEVICE_TYPE;
	unsigned short bat = read_adc();
	cmd[ofs++] = (bat >> 8) & 0xff;
	cmd[ofs++] = (bat) & 0xff;	
	cmd[4] = ofs-3; 
	unsigned short crc = CRC(cmd, ofs);
	cmd[ofs++] = (crc >> 8) & 0xff;
	cmd[ofs++] = (crc) & 0xff;
	LED_OUT |= LED_N_PIN;
	__delay_cycles(100000);
	LED_OUT &= ~LED_N_PIN;
	__delay_cycles(100000);
	LED_OUT |= LED_N_PIN;
	__delay_cycles(100000);
	LED_OUT &= ~LED_N_PIN;
	radio_send(cmd, ofs);
	g_cnt = SECS_2;
	P2IE  |= BIT0;
}
void switch_protect(int status)
{
	test_cnt = 0;
	if (status) /*protect on*/
	{
		g_cnt = SECS_2;
	}
	else
	{		
		g_cnt = MINS_5;
		g_trigger = 0;
	}
}
/*	
	stm32 -> msp430
	cc1101_addr 0x01 0x6c 0xaa data_len stm32_id msp430_id cmd_type sub_cmd_type result/protect_status crc
*/
void handle_cc1101_resp()
{
	unsigned char resp[32] = {0};
	unsigned char len = 32;
	unsigned short cmd_type = 0;
//	unsigned char sub_cmd_type = 0;
	int result = radio_read(resp, &len);
	if (result !=0 && len > 0) {
		if (resp[2] != MSG_HEAD0 || resp[3] != MSG_HEAD1)
			return ;		
		if (resp[4] != len -5)
			return ;
		/*check subdevice id = local device id*/
		uint8_t id[4];
		read_info(ADDR_SN, id, 4);
		//id = (id << 8) + resp[12];
		//id = (id << 8) + resp[13];
		//id = (id << 8) + resp[14];
		if (memcmp(id , resp+11, 4)!=0)
			return ;
		/*check stm32 id = saved stm32 id*/
		if (memcmp(stm32_id , zero_id, STM32_CODE_LEN) !=0) {
			if (memcmp(stm32_id, resp+5, STM32_CODE_LEN) !=0 && g_state !=STATE_ASK_CC1101_ADDR)
				return ;
		}
		len = resp[4];
		unsigned short crc = CRC(resp, len+3);
		/*check crc*/
		if (crc != (resp[len+3] << 8 | resp[len+4]))
			return ;
		
	cmd_type = resp[15]<<8 | resp[16];
	switch (cmd_type) {
		case CMD_REG_CODE_ACK:	
			if (resp[len+2] != 0x00 && cc1101_addr == 0) {/*if get vaild addr && curr addr ==0*/
				memcpy(stm32_id, resp+5, STM32_CODE_LEN);
				cc1101_addr = resp[18];
				unsigned char pkt = 0x05;
				trx8BitRegAccess(RADIO_WRITE_ACCESS, ADDR, &cc1101_addr, 1);
				trx8BitRegAccess(RADIO_WRITE_ACCESS, PKTCTRL1, &pkt, 1);
				handle_cc1101_addr(stm32_id, 1);
			} else {
				handle_cc1101_addr(resp+8, 0);
			}
			g_state = STATE_CONFIRM_CC1101_ADDR;			
			break;
		
		case CMD_CONFIRM_CODE_ACK:
				g_state = STATE_PROTECT_ON;
			break;
		case CMD_ALARM_ACK:
			if (b_protection_state != resp[len+2]) {
				b_protection_state= resp[len+2];
				switch_protect(b_protection_state);
			}
			switch (resp[len+1]) {
				case 0x01:
					last_sub_cmd &= ~0x02;
					break;
				case 0x02:
					last_sub_cmd &= ~0x01;
					break;
				default:
					break;		
			}
			//LED_OUT &= ~LED_N_PIN;
			break;
		case CMD_LOW_POWER_ACK:
			if (b_protection_state != resp[len+2]) {
				b_protection_state= resp[len+2];
				switch_protect(b_protection_state);
			}
				last_sub_cmd &= ~0x04;
			break;
		default:
			break;
	}

	if (last_sub_cmd == 0 && g_state == STATE_PROTECT_ON) {
		resend_cnt = 0;
		if (!b_protection_state)
			g_cnt = MINS_5;
		radio_sleep();
		//LED_OUT &= ~LED_N_PIN;
		}
	}

//	if (last_sub_cmd == 0 && b_protection_state)
//	{
//		TACTL = MC_0;
//	}
}
void handle_timer()
{
	if (g_state == STATE_ASK_CC1101_ADDR ||
		g_state == STATE_CONFIRM_CC1101_ADDR ||
		last_sub_cmd != 0) {
		resend_cnt++;
		if (resend_cnt == 10)
		{
			last_sub_cmd = 0;
			if (!b_protection_state)
				g_cnt = MINS_5;
			radio_sleep();
			//LED_OUT &= ~LED_N_PIN;
			return ;
		}
	}

	if (resend_cnt < 10 || resend_cnt > 50) {
	if (g_state == STATE_ASK_CC1101_ADDR)
		handle_cc1101_addr(NULL, 0);
	else if (g_state == STATE_CONFIRM_CC1101_ADDR)
		handle_cc1101_addr(stm32_id, 1);
	else {
		if (last_sub_cmd & 0x01)
			handle_cc1101_cmd(CMD_ALARM,0x02);
		
		if (last_sub_cmd & 0x02)
			handle_cc1101_cmd(CMD_ALARM,0x01);

		if (last_sub_cmd & 0x04)
			handle_cc1101_cmd(CMD_LOW_POWER,0x00);
	}
	if (resend_cnt > 50)
		resend_cnt = 0;
	}

	if (g_trigger) {
		g_trigger = 0;
		handle_cc1101_cmd(CMD_ALARM,0x01);
	}
	//unsigned short bat = read_adc();
	//if (bat < MIN_BAT)
	//	handle_cc1101_cmd(CMD_LOW_POWER,0x00);
}
void task()
{
	LED_SEL &= ~LED_N_PIN;
	LED_DIR |= LED_N_PIN;
	LED_OUT |= LED_N_PIN;
	__delay_cycles(500000);
	 LED_OUT &= ~LED_N_PIN;
	__delay_cycles(500000);
	LED_OUT |= LED_N_PIN;
	__delay_cycles(500000);
	LED_OUT &= ~LED_N_PIN;

	LIGHT_SEL &= ~LIGHT_N_PIN;
	LIGHT_DIR &= ~LIGHT_N_PIN;
	LIGHT_REN &= ~LIGHT_N_PIN;

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
	INFRAR_POWER_OUT &= ~INFRAR_POWER_N_PIN;
	#if USE_SMCLK
	CCR0 = 32768;
	TACTL = TASSEL_2 + MC_1 + TAIE;
	#else
	TACTL = TASSEL_1 + MC_2 + TAIE;// + ID0;
	#endif
	radio_init();
	handle_cc1101_addr(NULL, 0);
	while (1) {
		//__bis_SR_register(LPM0_bits + GIE);
		#if USE_SMCLK
		__bis_SR_register(LPM0_bits + GIE);
		#else
		__bis_SR_register(LPM3_bits + GIE);
		#endif
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
			memset(stm32_id, 0, STM32_CODE_LEN);
			cc1101_addr = 0x0;			
			unsigned char pkt = 0x06;
			trx8BitRegAccess(RADIO_WRITE_ACCESS, PKTCTRL1, &pkt, 1);
			g_state = STATE_ASK_CC1101_ADDR;
			handle_cc1101_addr(NULL,0);
			CODE_KEY_IFG &= ~CODE_KEY_N_PIN;
			CODE_KEY_IE |= CODE_KEY_N_PIN;
		}

		if (key & KEY_S1) {
			key &= ~KEY_S1;
			/*send s1 alarm to stm32*/
			handle_cc1101_cmd(CMD_ALARM, 0x02);
			S1_KEY_IFG &= ~S1_KEY_N_PIN;
			S1_KEY_IE  |= S1_KEY_N_PIN;
		}

		//if (key & KEY_LOWPOWER) {
		//	key &= ~KEY_LOWPOWER;
		//	/*send s1 alarm to stm32*/
		//	handle_cc1101_cmd(CMD_LOW_POWER, 0x01);
		//}

		if (key & KEY_INFRAR) {
			key &= ~KEY_INFRAR;
			/*send infrar alarm to stm32*/
			//add int count then make decision
			if (g_state==STATE_PROTECT_ON) {
				if (b_protection_state || !(LIGHT_IN & LIGHT_N_PIN)) {
					//LED_OUT &= ~LED_N_PIN;
					if (timer_5s >= 3)
					{						
						timer_5s = 0;
						handle_cc1101_cmd(CMD_ALARM, 0x01);
					}
				} else if(!b_protection_state) {
					g_trigger = 1;
				}
			}
			//INFRAR_KEY_IFG &= ~INFRAR_KEY_N_PIN;
			//INFRAR_KEY_IE |= INFRAR_KEY_N_PIN;
		}

		if (key & KEY_WIRELESS) {
			key &= ~KEY_WIRELESS;
			/*new data come from stm32*/
			//P2IE  &= ~BIT0;
			handle_cc1101_resp();
			//P2IFG &= ~BIT0;
			//P2IE  |= BIT0;
		}
		NOP();
		_EINT();
	}
	return ;
}
