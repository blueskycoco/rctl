#include <msp430g2553.h>
#include <stdint.h>
#include "spi.h"
#include "cc1101_def.h"
#include "cc1101.h"
#include "dbg.h"
#define RF_XTAL 26000
#define SCALING_FREQ     (float)((RF_XTAL)/65.536)
#if 0
const registerSetting_t preferredSettings_1200bps[]=
{
	{IOCFG2,0x06},
	{PKTCTRL1,0x04},
	{PKTCTRL0,0x05},
	{FSCTRL1,0x06},
	{FSCTRL0,0x00},
	{FREQ2,0x10},
	{FREQ1,0xa7},
	{FREQ0,0x62},
	{MDMCFG4,0xf5},
	{MDMCFG3,0x83},
	{MDMCFG2,0x13},
	{MDMCFG1,0x22},
	{MDMCFG0,0xf8},
	{CHANNR,0x00},
	{DEVIATN,0x15},
	{MCSM0,0x18},
	{FOCCFG,0x16},
	{BSCFG,0x6C},
	{AGCCTRL2,0x03},
	{AGCCTRL1,0x40},
	{AGCCTRL0,0x91},
	{WORCTRL,0xFB},
	{FREND1,0x56},
	{FREND0,0x10},
	{FSCAL3,0xE9},
	{FSCAL2,0x2A},
	{FSCAL1,0x00},
	{FSCAL0,0x1F},
	{TEST2,0x81},
	{TEST1,0x35},
	{TEST0,0x09},
	{FSTEST,0x59},
	{FIFOTHR,0x47},
	{ADDR,0x00},
	{PKTLEN,0x3d},
	{PATABLE,0x60} 
};
#else
const registerSetting_t preferredSettings_1200bps[]=
{
	{IOCFG2,0x06},
	{IOCFG1,0x2e},
	{IOCFG0,0x06},
	{FIFOTHR,0x47},
	{SYNC1,0xf3},
	{SYNC0,0x9f},
	{PKTLEN,0x3D},
	{PKTCTRL1,0x0E},
	{PKTCTRL0,0x45},
	{ADDR,0x01},
	{CHANNR,0x00},
	{FSCTRL1,0x0C},
	{FSCTRL0,0x00},
	{FREQ2,0x10},
	{FREQ1,0xA7},
	{FREQ0,0x62},
	{MDMCFG4,0x5B},
	{MDMCFG3,0xF8},
	{MDMCFG2,0x93},
	{MDMCFG1,0x23},
	{MDMCFG0,0xF8},
	{DEVIATN,0x47},
	{MCSM2,0x07},
	{MCSM1,0x30},
	{MCSM0,0x08},
	{FOCCFG,0x1D},
	{BSCFG,0x1C},
	{AGCCTRL2,0xC7},
	{AGCCTRL1,0x00},
	{AGCCTRL0,0xB2},
	{WOREVT1,0x87},
	{WOREVT0,0x6B},
	{WORCTRL,0xFB},
	{FREND1,0xB6},
	{FREND0,0x10},
	{FSCAL3,0xEA},
	{FSCAL2,0x2A},
	{FSCAL1,0x00},
	{FSCAL0,0x1F},
	{RCCTRL1,0x41},
	{RCCTRL0,0x00},
	{FSTEST,0x59},
	{PTEST,0x7F},
	{AGCTEST,0x3F},
	{TEST2,0x81},
	{TEST1,0x35},
	{TEST0,0x09}	
};
#endif
int radio_init(void)
{

	unsigned char i, writeByte, preferredSettings_length;
	registerSetting_t *preferredSettings;

	//hal_timer_init(32768);
	trxRfSpiInterfaceInit(2);

	trxSpiCmdStrobe(RF_SRES);

	__delay_cycles(16000);
	preferredSettings_length = sizeof(preferredSettings_1200bps)/sizeof(registerSetting_t);
	preferredSettings = (registerSetting_t *)preferredSettings_1200bps;
	for(i = 0; i < preferredSettings_length; i++) {
		writeByte = preferredSettings[i].data;
		trx8BitRegAccess(RADIO_WRITE_ACCESS, preferredSettings[i].addr, &writeByte, 1);
	}
#if 1
	for(i = 0; i < preferredSettings_length; i++) {
		uint8 readByte = 0;
		trx8BitRegAccess(RADIO_READ_ACCESS, preferredSettings[i].addr, &readByte, 1);
		if (readByte == preferredSettings[i].data)
			uart_write_string("rf reg set ok \r\n");
		else
			uart_write_string("rf reg set failed\r\n");
	}
	#endif
	//radio_set_freq(433000);
	return 0;
}
int radio_send(unsigned char *payload, unsigned short payload_len) {
	trx8BitRegAccess(RADIO_WRITE_ACCESS|RADIO_SINGLE_ACCESS, TXFIFO, &payload_len, 1);

	trx8BitRegAccess(RADIO_WRITE_ACCESS|RADIO_BURST_ACCESS, TXFIFO, payload, payload_len);

	trxSpiCmdStrobe(RF_STX);               // Change state to TX, initiating	
	
	while(!(RF_GDO_IN & RF_GDO_PIN));	
	while((RF_GDO_IN & RF_GDO_PIN));
	return(0);
}
int radio_read(unsigned char *buf, unsigned short *buf_len) {
	unsigned char status[2];
	unsigned char pktLen;
	trxSpiCmdStrobe(RF_SRX);
	while(!(RF_GDO_IN & RF_GDO_PIN));	
	while((RF_GDO_IN & RF_GDO_PIN));
	/* Read number of bytes in RX FIFO */
	trx8BitRegAccess(RADIO_READ_ACCESS|RADIO_SINGLE_ACCESS, RXBYTES, &pktLen, 1);
	pktLen = pktLen  & NUM_RXBYTES;

	/* make sure the packet size is appropriate, that is 1 -> buffer_size */
	if ((pktLen > 0) && (pktLen <= *buf_len)) {
		trx8BitRegAccess(RADIO_READ_ACCESS|RADIO_SINGLE_ACCESS, RXFIFO, &pktLen, 1);
			//uart_write_string("len is %d\r\n",pktLen);
		/* retrieve the FIFO content */
		trx8BitRegAccess(RADIO_READ_ACCESS|RADIO_BURST_ACCESS, RXFIFO, buf, pktLen);

		/* return the actual length of the FIFO */
		*buf_len = pktLen;

		/* retrieve the CRC status information */
		trx8BitRegAccess(RADIO_READ_ACCESS+RADIO_BURST_ACCESS, RXFIFO, status, 2);
	//	printf("status %x %x\r\n", status[0],status[1]);

	} else {

		/* if the length returned by the transciever does not make sense, flush it */
		*buf_len = 0;                                // 0
		status[1] = 0;
		trxSpiCmdStrobe(RF_SFRX);	                 // Flush RXFIFO
	}

	/* return status information, CRC OK or NOT OK */
	return (status[1] & CRC_OK);
}
int radio_set_freq(unsigned long freq) {

	unsigned long freq_word;
	unsigned char freq_byte[3];
	float freq_float;

	// calculate the frequency word

	freq_float = freq*1000;
	freq_word = (unsigned long) (freq_float * 1/(float)SCALING_FREQ);

	/* return the frequency word */
	freq_byte[2] = ((uint8*)&freq_word)[0];
	freq_byte[1] = ((uint8*)&freq_word)[1];
	freq_byte[0] = ((uint8*)&freq_word)[2];
	if (freq_byte[2] != 0x62 || freq_byte[1] != 0xa7 || freq_byte[0] != 0x10)
		uart_write_string("set freq to 433 failed\r\n");
	else
		uart_write_string("set freq to 433 ok\r\n");
	// upload the frequency word to the transciver using a 3 byte write
	trx8BitRegAccess(RADIO_WRITE_ACCESS, FREQ2, &(freq_byte[2]), 1);
	trx8BitRegAccess(RADIO_WRITE_ACCESS, FREQ1, &(freq_byte[1]), 1);
	trx8BitRegAccess(RADIO_WRITE_ACCESS, FREQ0, &(freq_byte[0]), 1);

	return 0;
}

