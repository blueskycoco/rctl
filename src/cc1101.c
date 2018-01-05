#include <msp430g2553.h>
#include <stdint.h>
#include "spi.h"
#include "cc1101_def.h"
#include "cc1101.h"
#include "dbg.h"
#define RF_XTAL 26000
#define SCALING_FREQ     (float)((RF_XTAL)/65.536)
const registerSetting_t preferredSettings_1200bps[]=
{
	{IOCFG2,	0x06},
	{PKTCTRL0,	0x05},	
	{FSCTRL1,	0x06},
	{FREQ2,		0x10},
	{FREQ1,		0xa7},
	{FREQ0,		0x62},
	{MDMCFG4,	0xf5},
	{MDMCFG3,	0x83},
	{MDMCFG2,	0x13},
	{DEVIATN,	0x15}, 
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
#if 0
	for(i = 0; i < preferredSettings_length; i++) {
		uint8 readByte = 0;
		trx8BitRegAccess(RADIO_READ_ACCESS, preferredSettings[i].addr, &readByte, 1);
		if (readByte == preferredSettings[i].data)
			uart_write_string("rf reg set ok \r\n");
		else
			uart_write_string("rf reg set failed\r\n");
	}
	#endif
	return 0;
}
int radio_send(unsigned char *payload, unsigned short payload_len) {
	trx8BitRegAccess(RADIO_WRITE_ACCESS|RADIO_SINGLE_ACCESS, TXFIFO, (unsigned char *)&payload_len, 1);

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
	//if (freq_byte[2] != 0x62 || freq_byte[1] != 0xa7 || freq_byte[0] != 0x10)
	//	uart_write_string("set freq to 433 failed\r\n");
	//else
	//	uart_write_string("set freq to 433 ok\r\n");
	// upload the frequency word to the transciver using a 3 byte write
	trx8BitRegAccess(RADIO_WRITE_ACCESS, FREQ2, &(freq_byte[2]), 1);
	trx8BitRegAccess(RADIO_WRITE_ACCESS, FREQ1, &(freq_byte[1]), 1);
	trx8BitRegAccess(RADIO_WRITE_ACCESS, FREQ0, &(freq_byte[0]), 1);
	trxSpiCmdStrobe(RF_SCAL);

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

