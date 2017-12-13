#include <msp430g2553.h>
#include <stdint.h>
#include "spi.h"
#include "cc1101_def.h"
#include "cc1101.h"
#include "dbg.h"
#define SCALING_FREQ     (float)((RF_XTAL)/65.536)
#define SCALING_FREQEST  (unsigned long)((RF_XTAL)/16.384)

unsigned char paTable[1];           
unsigned char rf_end_packet = 0;

const registerSetting_t preferredSettings_1200bps[]=
{
	{IOCFG2,      0x06},
	{IOCFG1,      0x2E},
	{IOCFG0,      0x06},
	{FIFOTHR,     0x47},
	{SYNC1,       0xD3},
	{SYNC0,       0x91},
	{PKTLEN,      0x18},
	{PKTCTRL1,    0x00},
	{PKTCTRL0,    0x04},
	{FSCTRL1,     0x06},
	{FREQ2,       0x23},
	{FREQ1,       0xB8},
	{FREQ0,       0x9D},
	{MDMCFG4,     0xF5},
	{MDMCFG3,     0x83},
	{MDMCFG2,     0x13},
	{MDMCFG1,     0x22},
	{MDMCFG0,     0xF7},
	{DEVIATN,     0x14},
	{MCSM0,       0x18},
	{FOCCFG,      0x14},
	{WORCTRL,     0xFB},
	{FSCAL3,      0xE9},
	{FSCAL2,      0x2A},
	{FSCAL1,      0x00},
	{FSCAL0,      0x1F},
	{TEST2,       0x81},
	{TEST1,       0x35},
	{TEST0,       0x09},
};

int radio_init(void)
{

	unsigned char i, writeByte, preferredSettings_length;
	registerSetting_t *preferredSettings;


	trxRfSpiInterfaceInit(2);

	trxSpiCmdStrobe(RF_SRES);

	__delay_cycles(16000);
	preferredSettings_length = sizeof(preferredSettings_1200bps)/sizeof(registerSetting_t);
	preferredSettings = (registerSetting_t *)preferredSettings_1200bps;
	for(i = 0; i < preferredSettings_length; i++) {
		writeByte = preferredSettings[i].data;
		trx8BitRegAccess(RADIO_WRITE_ACCESS, preferredSettings[i].addr, &writeByte, 1);
	}
	paTable[0] = 0xC5;	
	trx8BitRegAccess(RADIO_WRITE_ACCESS|RADIO_BURST_ACCESS, PATABLE, paTable, 1);

	for(i = 0; i < preferredSettings_length; i++) {
		uint8 readByte = 0;
		trx8BitRegAccess(RADIO_READ_ACCESS, preferredSettings[i].addr, &readByte, 1);
		if (readByte == preferredSettings[i].data)
			uart_write_string("rf reg set ok\r\n");
		else
			uart_write_string("rf reg set failed\r\n");
	}
	return 0;
}

