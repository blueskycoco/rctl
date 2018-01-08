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
#define CMD_REG_CODE	0x00
#define CMD_LOW_POWER	0x0C
#define DEVICE_TYPE		0x02
#define	CMD_PROTECT_ON	0x02
#define	CMD_PROTECT_OFF	0x04
#define	CMD_ALARM		0x06
#define	CMD_MUTE		0x0e
#define MSG_HEAD0		0x6c
#define MSG_HEAD1		0xaa
#define PACKAGE_LEN		14
#define DATA_LEN		9
#define LED_SEL         P1SEL
#define LED_OUT         P1OUT
#define LED_DIR         P1DIR
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
unsigned char b_protection_state = 0;	/*protection state*/
volatile unsigned int cnt = 0;
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
void __attribute__ ((interrupt(TIMER0_A1_VECTOR))) Timer_A (void)
{  	
	switch( TA0IV )	
	{
		case  2:  break;
		case  4:  break;
		case 10:  
			{
				cnt++;
				LED_OUT |= LED_N_PIN;
				__delay_cycles(8000000);
				LED_OUT &= ~LED_N_PIN;
				if (cnt == 2700) /*almost 12 hours*/
				{
					cnt = 0;
					__bic_SR_register_on_exit(LPM3_bits);
				}
			}
		break;
	}
}
/*void __attribute__ ((interrupt(PORT1_VECTOR))) Port_1 (void)
{  
	P1OUT ^= BIT0;                            // P1.0 = toggle  
	P1IFG &= ~BIT3;                           // P1.3 IFG cleared
}*/	

void task()
{		
	LED_SEL &= ~LED_N_PIN;
	LED_DIR |= LED_N_PIN;
	LED_OUT &= ~LED_N_PIN;
	radio_init();

	TACTL = TASSEL_1 + MC_2 + TAIE + ID0 + ID1;
	while (1) {		
		/*ask which state should enter */
		//radio_send();
		__bis_SR_register(LPM3_bits + GIE);
		//radio_read();
	}
	return ;
}
