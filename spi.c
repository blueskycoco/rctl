#include <msp430g2553.h>
#include <stdint.h>
#include "spi.h"
#include "cc1101.h"

void trxRfSpiInterfaceInit(uint8 prescalerValue)
{
	UCB0CTL1 |= UCSWRST;
	UCB0CTL0  =  0x00+UCMST + UCSYNC + UCMODE_0 + UCMSB + UCCKPH;
	UCB0CTL1 |=  UCSSEL_2;
	UCB0BR1   =  0x00;

	UCB0BR0 = prescalerValue;
	P1SEL2 |= RF_MISO_PIN + RF_MOSI_PIN + RF_SCLK_PIN;
	RF_PORT_SEL |= RF_MISO_PIN + RF_MOSI_PIN + RF_SCLK_PIN;
	RF_PORT_DIR |= RF_MOSI_PIN + RF_SCLK_PIN;
	RF_PORT_DIR &= ~RF_MISO_PIN;
	RF_CS_N_PORT_SEL &= ~RF_CS_N_PIN;
	RF_CS_N_PORT_DIR |= RF_CS_N_PIN;
	RF_CS_N_PORT_OUT |= RF_CS_N_PIN;
	RF_GDO_SEL &= ~RF_GDO_PIN;
	RF_GDO_DIR &= ~RF_GDO_PIN;

	UCB0CTL1 &= ~UCSWRST;
	return;
}

static void trxReadWriteBurstSingle(uint8 addr,uint8 *pData,uint16 len)
{
	uint16 i;
	if(addr&RADIO_READ_ACCESS)
	{
		if(addr&RADIO_BURST_ACCESS)
		{
			for (i = 0; i < len; i++)
			{
				RF_SPI_TX(0);   
				RF_SPI_WAIT_DONE();
				*pData = RF_SPI_RX();   
				pData++;
			}
		}
		else
		{
			RF_SPI_TX(0);
			RF_SPI_WAIT_DONE();
			*pData = RF_SPI_RX();
		}
	}
	else
	{
		if(addr&RADIO_BURST_ACCESS)
		{
			for (i = 0; i < len; i++)
			{
				RF_SPI_TX(*pData);
				RF_SPI_WAIT_DONE();
				pData++;
			}
		}
		else
		{
			RF_SPI_TX(*pData);
			RF_SPI_WAIT_DONE();
		}
	}
	return;
}
rfStatus_t trx8BitRegAccess(uint8 accessType, uint8 addrByte, uint8 *pData, uint16 len)
{
	uint8 readValue;

	RF_SPI_BEGIN();
	while(RF_PORT_IN & RF_MISO_PIN);
	RF_SPI_TX(accessType|addrByte);
	RF_SPI_WAIT_DONE();
	readValue = RF_SPI_RX();
	trxReadWriteBurstSingle(accessType|addrByte,pData,len);
	RF_SPI_END();
	return(readValue);
}

rfStatus_t trxSpiCmdStrobe(uint8 cmd)
{
	uint8 rc;
	RF_SPI_BEGIN();
	while(RF_PORT_IN & RF_MISO_PIN);
	RF_SPI_TX(cmd);
	RF_SPI_WAIT_DONE();
	rc = RF_SPI_RX();
	RF_SPI_END();
	return(rc);
}


