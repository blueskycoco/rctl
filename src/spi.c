#include <msp430g2553.h>
#include <stdint.h>
#include "spi.h"
#include "cc1101.h"

void trxRfSpiInterfaceInit(uint8 prescalerValue)
{
#ifdef SW_SPI
	RF_CS_N_PORT_SEL &= ~RF_CS_N_PIN;
	RF_CS_N_PORT_DIR |= RF_CS_N_PIN;
	RF_CS_N_PORT_OUT |= RF_CS_N_PIN;


	RF_PORT_SEL &= ~RF_MOSI_PIN;
	RF_PORT_DIR |= RF_MOSI_PIN;
	RF_PORT_OUT |= RF_MOSI_PIN;

	RF_PORT_SEL &= ~RF_MISO_PIN;
	RF_PORT_DIR &= ~RF_MISO_PIN;

	RF_PORT_SEL &= ~RF_SCLK_PIN;
	RF_PORT_DIR |= RF_SCLK_PIN;
	RF_PORT_OUT |= RF_SCLK_PIN;

	RF_GDO_SEL &= ~RF_GDO_PIN;
	RF_GDO_DIR &= ~RF_GDO_PIN;
	RF_GDO_PxIES	|= RF_GDO_PIN;	

	RF_GDO0_SEL &= ~RF_GDO0_PIN;
	RF_GDO0_DIR &= ~RF_GDO0_PIN;
	RF_GDO0_PxIES	|= RF_GDO0_PIN;

	RF_POWER_N_PORT_SEL &= ~RF_POWER_N_PIN;
	RF_POWER_N_PORT_DIR |= RF_POWER_N_PIN;
	RF_POWER_N_PORT_OUT |= RF_POWER_N_PIN;
	__delay_cycles(1000);
	RF_POWER_N_PORT_OUT &= ~RF_POWER_N_PIN;
	__delay_cycles(1000);
#else
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
	RF_GDO_PxIES	|= RF_GDO_PIN;
	RF_GDO0_SEL &= ~RF_GDO0_PIN;
	RF_GDO0_DIR &= ~RF_GDO0_PIN;
	RF_GDO0_PxIES	|= RF_GDO0_PIN;	
	RF_POWER_N_PORT_SEL &= ~RF_POWER_N_PIN;
	RF_POWER_N_PORT_DIR |= RF_POWER_N_PIN;
	RF_POWER_N_PORT_OUT |= RF_POWER_N_PIN;
	__delay_cycles(1000);
	RF_POWER_N_PORT_OUT &= ~RF_POWER_N_PIN;
	__delay_cycles(1000);

	UCB0CTL1 &= ~UCSWRST;
#endif
	return;
}
#ifdef SW_SPI
void mosi(int type)
{
	if(type)
	{
		RF_PORT_OUT |= RF_MOSI_PIN;
	}
	else
	{
		RF_PORT_OUT &= ~RF_MOSI_PIN;		
	}
}
void clk(int type)
{
	if(type)
	{
		RF_PORT_OUT |= RF_SCLK_PIN;
	}
	else
	{
		RF_PORT_OUT &= ~RF_SCLK_PIN;
	}
}
int miso()
{
	return RF_PORT_IN & RF_MISO_PIN;
}
void _nop_()
{ 
	__delay_cycles(10);
}
uint8_t spi_send_rcv(uint8_t data)
{
	uint8_t i,temp;
	temp = 0;

	clk(0);
	for(i=0; i<8; i++)
	{
		if(data & 0x80)
		{
			mosi(1);
		}
		else mosi(0);
		data <<= 1;

		clk(1); 
		_nop_();
		_nop_();

		temp <<= 1;
		if(miso())temp++; 
		clk(0);
		_nop_();
		_nop_();	
	}
	return temp;
}
#endif
static void trxReadWriteBurstSingle(uint8 addr,uint8 *pData,uint16 len)
{
	uint16 i;
	if(addr&RADIO_READ_ACCESS)
	{
		if(addr&RADIO_BURST_ACCESS)
		{
			for (i = 0; i < len; i++)
			{
#ifdef SW_SPI
				*pData = spi_send_rcv(0);
#else
				RF_SPI_TX(0);   
				RF_SPI_WAIT_DONE();
				*pData = RF_SPI_RX();   
#endif
				pData++;
			}
		}
		else
		{
#ifdef SW_SPI
			*pData = spi_send_rcv(0);
#else
			RF_SPI_TX(0);
			RF_SPI_WAIT_DONE();
			*pData = RF_SPI_RX();
#endif
		}
	}
	else
	{
		if(addr&RADIO_BURST_ACCESS)
		{
			for (i = 0; i < len; i++)
			{
#ifdef SW_SPI
				spi_send_rcv(*pData);
#else
				RF_SPI_TX(*pData);
				RF_SPI_WAIT_DONE();
#endif
				pData++;
			}
		}
		else
		{
#ifdef SW_SPI
			spi_send_rcv(*pData);
#else
			RF_SPI_TX(*pData);
			RF_SPI_WAIT_DONE();
#endif
		}
	}
	return;
}
rfStatus_t trx8BitRegAccess(uint8 accessType, uint8 addrByte, uint8 *pData, uint16 len)
{
	uint8 readValue;

	RF_SPI_BEGIN();
#ifdef SW_SPI
	while(miso());
	readValue = spi_send_rcv(accessType|addrByte);
#else
	while(RF_PORT_IN & RF_MISO_PIN);
	RF_SPI_TX(accessType|addrByte);
	RF_SPI_WAIT_DONE();
	readValue = RF_SPI_RX();
#endif
	trxReadWriteBurstSingle(accessType|addrByte,pData,len);
	RF_SPI_END();
	return(readValue);
}

rfStatus_t trxSpiCmdStrobe(uint8 cmd)
{
	uint8 rc;
	RF_SPI_BEGIN();
#ifdef SW_SPI
	while(miso());
	rc = spi_send_rcv(cmd);
#else
	while(RF_PORT_IN & RF_MISO_PIN);
	RF_SPI_TX(cmd);
	RF_SPI_WAIT_DONE();
	rc = RF_SPI_RX();
#endif
	RF_SPI_END();
	return(rc);
}


