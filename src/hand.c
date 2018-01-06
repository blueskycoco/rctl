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

void __attribute__ ((interrupt(ADC10_VECTOR))) ADC10_ISR (void)
{
  __bic_SR_register_on_exit(CPUOFF);        // Clear CPUOFF bit from 0(SR)
}
unsigned short read_adc()
{
	static volatile int adc[10] = {0};
	int total_adc = 0,i = 0;
	ADC10CTL1 = CONSEQ_2 + INCH_0;						// Repeat single channel, A0
	ADC10CTL0 = ADC10SHT_2 +MSC + ADC10ON + ADC10IE;	// Sample & Hold Time + ADC10 ON + Interrupt Enable
	ADC10DTC1 = 0x0A;									// 10 conversions
	ADC10AE0 |= 0x01;									// P1.0 ADC option select
	ADC10CTL0 &= ~ENC;				// Disable Conversion
    while (ADC10CTL1 & BUSY);		// Wait if ADC10 busy
    ADC10SA = (int)adc;				// Transfers data to next array (DTC auto increments address)
    ADC10CTL0 |= ENC + ADC10SC;		// Enable Conversion and conversion start
    __bis_SR_register(CPUOFF + GIE);// Low Power Mode 0, ADC10_ISR
    for (i=0; i<10; i++)
		total_adc += adc[i];
	return (unsigned short)(total_adc/10);
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
		key |= 0x01;
	if (!(KEY_IN & KEY_N_PIN1))
		key |= 0x02;
	if (!(KEY_IN & KEY_N_PIN2))
		key |= 0x04;

	if (key != 0x01 && key != 0x02 && key != 0x04)
	{
		LED_OUT &= ~LED_N_PIN;
		POWER_OUT &= ~POWER_N_PIN;
		return ;
	}

	if (key == 0x02)
	{
		while (!(KEY_IN & KEY_N_PIN1)) {
			i++;
			__delay_cycles(16000);
			if (i >= 2000) {
				key = 0x08;
				break;
			}
		}
	}
	
	cmd[0] = MSG_HEAD0;cmd[1] = MSG_HEAD1;
	cmd[2] = DATA_LEN; cmd[4] = DEVICE_TYPE;
	cmd[5] = ((long)ID_CODE >> 16) & 0xff;
	cmd[6] = ((long)ID_CODE >> 8) & 0xff;
	cmd[7] = ((long)ID_CODE >> 0) & 0xff;
	unsigned short bat = read_adc();
	cmd[9] = (bat >> 8) & 0xff;
	cmd[10] = (bat) & 0xff;
	if (key == 0x01)
		cmd[3] = 0x01;
	else if (key == 0x02)
		cmd[3] = 0x02;
	else if (key == 0x04)
		cmd[3] = 0x03;
	else
		cmd[3] = 0x04;
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
