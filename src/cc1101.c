#include <msp430g2553.h>
#include <stdint.h>
#include "spi.h"
#include "cc1101_def.h"
#include "cc1101.h"
#include "dbg.h"
#define RF_XTAL 26000
#define SCALING_FREQ     (float)((RF_XTAL)/65.536)
#define VAL_MDMCFG3	0x3b
#define VAL_MDMCFG4 0x2d
#define MAX_PAYLOAD	32
const registerSetting_t preferredSettings_1200bps[]=
{
	{IOCFG2,	0x06},
	{IOCFG0,	0x1b},
	{PKTCTRL0,	0x05},	
	{FSCTRL1,	0x06},
	{ADDR, 		0x00},
	{PKTCTRL1,	0x06},
	#if 0
	{FREQ2,		0x10},
	{FREQ1,		0xa7},
	{FREQ0,		0x62},
	#else
	{FREQ2,		0x11},
	{FREQ1,		0xec},
	{FREQ0,		0x4e},
	#endif
	{MDMCFG4,	0xf5},
	{MDMCFG3,	0x83},
	{MDMCFG2,	0x13},
	{DEVIATN,	0x15},
	{AGCCTRL2,	0x07},
	{AGCCTRL1,	0x40},
	{MCSM1,		0x3f},
	{MCSM0,		0x18},
	{FOCCFG,	0x16},
	{WORCTRL,	0xFB},
	{FSCAL3,	0xE9},
	{FSCAL2,	0x2A},
	{FSCAL1,	0x00},
	{FSCAL0,	0x1F},
	{TEST0,		0x09},
	{PATABLE,	0x60}
};
#define MRFI_RSSI_VALID_DELAY_US    1300
void MRFI_RSSI_VALID_WAIT()                                                
{                                                                             
  int16_t delay = MRFI_RSSI_VALID_DELAY_US;                                   
  unsigned char status;															
  do                                                                          
  {	
  	trx8BitRegAccess(RADIO_READ_ACCESS|RADIO_BURST_ACCESS, PKTSTATUS, &status, 1); 
    if(status & (0x50))
    {
      break;                                                                  
    }
    __delay_cycles(64);
    delay -= 64;                                                              
  }while(delay > 0);
}        

void MRFI_STROBE_IDLE_AND_WAIT()                   
{													  
  trxSpiCmdStrobe( RF_SIDLE );						  
  while (trxSpiCmdStrobe( RF_SNOP ) & 0xF0) ; 		  
}

static uint8_t mrfiRndSeed=0;
static uint16_t sReplyDelayScalar=0;
static uint16_t sBackoffHelper=0;

#define   MRFI_RADIO_OSC_FREQ         	26000000
#define   PLATFORM_FACTOR_CONSTANT    	2
#define   PHY_PREAMBLE_SYNC_BYTES     	8
#define   MRFI_BACKOFF_PERIOD_USECS 	250
#define   MRFI_RSSI_OFFSET	74
#define MRFI_RANDOM_OFFSET                   67
#define MRFI_RANDOM_MULTIPLIER              109

int8_t Mrfi_CalculateRssi(uint8_t rawValue)
{
  int16_t rssi;

  if(rawValue >= 128)
  {
    rssi = (int16_t)(rawValue - 256)/2 - MRFI_RSSI_OFFSET;
  }
  else
  {
    rssi = (rawValue/2) - MRFI_RSSI_OFFSET;
  }

  if(rssi < -128)
  {
    rssi = -128;
  }

  return rssi;
}

void create_seed()
{
	unsigned char rssi;
	uint32_t dataRate, bits;
	uint16_t exponent, mantissa;
	trxSpiCmdStrobe(RF_SRX);
	MRFI_RSSI_VALID_WAIT();	
	for(uint8_t i=0; i<16; i++)
	{
		trx8BitRegAccess(RADIO_READ_ACCESS|RADIO_BURST_ACCESS, RSSI, &rssi, 1);
		mrfiRndSeed = (mrfiRndSeed << 1) | (rssi & 0x01);
		Mrfi_CalculateRssi(rssi);
	}

	mrfiRndSeed |= 0x0080;
	RF_GDO_PxIE  &= ~RF_GDO_PIN;
	MRFI_STROBE_IDLE_AND_WAIT();
	trxSpiCmdStrobe( RF_SFRX );
	RF_GDO_PxIFG &= ~RF_GDO_PIN;
	
	mantissa = 256 + VAL_MDMCFG3;

	exponent = 28 - (VAL_MDMCFG4 & 0x0F);

	dataRate = mantissa * (MRFI_RADIO_OSC_FREQ>>exponent);

	bits = ((uint32_t)((PHY_PREAMBLE_SYNC_BYTES + MAX_PAYLOAD)*8))*10000;

	sReplyDelayScalar = PLATFORM_FACTOR_CONSTANT + (((bits/dataRate)+5)/10);
	sBackoffHelper = MRFI_BACKOFF_PERIOD_USECS + (sReplyDelayScalar>>5)*1000;
	RF_GDO_PxIE  |= RF_GDO_PIN;
}
uint8_t MRFI_RandomByte(void)
{
  mrfiRndSeed = (mrfiRndSeed*MRFI_RANDOM_MULTIPLIER) + MRFI_RANDOM_OFFSET;

  return mrfiRndSeed;
}

static void Mrfi_RandomBackoffDelay(void)
{
  uint8_t backoffs;
  uint8_t i;

  backoffs = (MRFI_RandomByte() & 0x0F) + 1;
  for (i=0; i<backoffs*sBackoffHelper; i++)
  {
    __delay_cycles( 1 );
  }
}
void Mrfi_RxModeOff(void)
{
  RF_GDO_PxIE	&= ~RF_GDO_PIN;

  MRFI_STROBE_IDLE_AND_WAIT();

  trxSpiCmdStrobe( RF_SFRX );

  RF_GDO_PxIFG	&= ~RF_GDO_PIN;
}
static void Mrfi_RxModeOn(void)
{
  RF_GDO_PxIFG	&= ~RF_GDO_PIN;

  trxSpiCmdStrobe( RF_SRX );

 RF_GDO_PxIE	|= RF_GDO_PIN;
}

void cca()
{
	uint32_t ccaRetries = 400;
	uint8_t papd = 0x1b;
	uint8_t sync = 0x06;
	for (;;)
	{
		trxSpiCmdStrobe( RF_SRX );

		MRFI_RSSI_VALID_WAIT();
		
		RF_GDO0_PxIFG	&= ~RF_GDO0_PIN;
		trxSpiCmdStrobe( RF_STX );

		__delay_cycles(700);
		if (RF_GDO0_PxIFG & RF_GDO0_PIN)
		{
			RF_GDO0_PxIFG	&= ~RF_GDO0_PIN;
			while (!(RF_GDO0_IN & RF_GDO0_PIN));
			break;
		}
		else
		{
			MRFI_STROBE_IDLE_AND_WAIT();

			trxSpiCmdStrobe( RF_SFRX );

			if (ccaRetries != 0)
			{
				//Mrfi_RandomBackoffDelay();
				__delay_cycles(25);
				ccaRetries--;
			}
			else 
			{
				break;
			}
		} 
	} 
	trxSpiCmdStrobe( RF_SFTX );
	Mrfi_RxModeOn();
}
int radio_init(void)
{

	unsigned char i, writeByte, preferredSettings_length;
	registerSetting_t *preferredSettings;

	trxRfSpiInterfaceInit(0);
	trxSpiCmdStrobe(RF_SRES);

	__delay_cycles(1000);
	preferredSettings_length = sizeof(preferredSettings_1200bps)/sizeof(registerSetting_t);
	preferredSettings = (registerSetting_t *)preferredSettings_1200bps;
	for(i = 0; i < preferredSettings_length; i++) {
		writeByte = preferredSettings[i].data;
		trx8BitRegAccess(RADIO_WRITE_ACCESS, preferredSettings[i].addr, &writeByte, 1);
	}
	//create_seed();
	return 0;
}
int radio_send(unsigned char *payload, unsigned short payload_len) {

	#ifndef HAND
	Mrfi_RxModeOff();
	#endif
	trx8BitRegAccess(RADIO_WRITE_ACCESS|RADIO_SINGLE_ACCESS, TXFIFO, (unsigned char *)&payload_len, 1);
	trx8BitRegAccess(RADIO_WRITE_ACCESS|RADIO_BURST_ACCESS, TXFIFO, payload, payload_len);
	#ifndef HAND
	cca();
	#else
	trxSpiCmdStrobe(RF_STX);
	//while (!(RF_GDO0_IN & RF_GDO0_PIN));
	//while ((RF_GDO0_IN & RF_GDO0_PIN));
	#endif
	return(0);
}
int radio_read(unsigned char *buf, unsigned short *buf_len) {
	unsigned char status[2];
	unsigned char pktLen;
	//trxSpiCmdStrobe(RF_SRX);
	//while(!(RF_GDO_IN & RF_GDO_PIN));	
	while((RF_GDO_IN & RF_GDO_PIN));
	
	trx8BitRegAccess(RADIO_READ_ACCESS|RADIO_SINGLE_ACCESS, RXBYTES, &pktLen, 1);
	pktLen = pktLen  & NUM_RXBYTES;

	if (pktLen > 0)
		trx8BitRegAccess(RADIO_READ_ACCESS|RADIO_SINGLE_ACCESS, RXFIFO, &pktLen, 1);
	
	if ((pktLen > 0) && (pktLen <= *buf_len)) {
		trx8BitRegAccess(RADIO_READ_ACCESS|RADIO_BURST_ACCESS, RXFIFO, buf, pktLen);

		//*buf_len = pktLen;
		trx8BitRegAccess(RADIO_READ_ACCESS+RADIO_BURST_ACCESS, RXFIFO, status, 2);
		*buf_len = pktLen;
	} else {
		*buf_len = 0;
		status[1] = 0;
		trxSpiCmdStrobe(RF_SFRX);
	}

	return (status[1] & CRC_OK);
}
void radio_sleep() {
	trxSpiCmdStrobe(RF_SIDLE);
	trxSpiCmdStrobe(RF_SPWD);
}
int radio_set_freq(unsigned long freq) {

	unsigned long freq_word;
	unsigned char freq_byte[3];
	float freq_float;

	freq_float = freq*1000;
	freq_word = (unsigned long) (freq_float * 1/(float)SCALING_FREQ);
	freq_byte[2] = ((uint8*)&freq_word)[0];
	freq_byte[1] = ((uint8*)&freq_word)[1];
	freq_byte[0] = ((uint8*)&freq_word)[2];

	trx8BitRegAccess(RADIO_WRITE_ACCESS, FREQ2, &(freq_byte[2]), 1);
	trx8BitRegAccess(RADIO_WRITE_ACCESS, FREQ1, &(freq_byte[1]), 1);
	trx8BitRegAccess(RADIO_WRITE_ACCESS, FREQ0, &(freq_byte[0]), 1);
	//trxSpiCmdStrobe(RF_SCAL);

	return 0;
}
unsigned short CRC(unsigned char *Data,unsigned char Data_length)
{
	unsigned int mid=0;
	unsigned char times=0,Data_index=0;
	unsigned short CRC_data=0xFFFF;
	while(Data_length)
	{
		CRC_data=Data[Data_index]^CRC_data;
		for(times=0;times<8;times++)
		{
			mid=CRC_data;
			CRC_data=CRC_data>>1;
			if(mid & 0x0001)
			{
				CRC_data=CRC_data^0xA001;
			}
		}
		Data_index++;
		Data_length--;
	}
	return CRC_data;
}
void __attribute__ ((interrupt(ADC10_VECTOR))) ADC10_ISR (void)
{
  __bic_SR_register_on_exit(CPUOFF);        // Clear CPUOFF bit from 0(SR)
}
unsigned short read_adc()
{
	static volatile int adc[10] = {0};
	int total_adc = 0,i = 0;
	//ADC10CTL0 &= ~ENC;
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

