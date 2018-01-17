#include <msp430g2553.h>
#include <stdint.h>
#include "spi.h"
#include "cc1101.h"

void trxRfSpiInterfaceInit(uint8 prescalerValue)
{
	#if 0
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
	#else
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
	#endif
	return;
}
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
    return RF_PORT_DIR & RF_MISO_PIN;
}
void _nop_()
{
    //volatile long i,j;
    //for(i=0;i<20;i++)
    //   j=0;
    NOP();
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
static void trxReadWriteBurstSingle(uint8 addr,uint8 *pData,uint16 len)
{
	uint16 i;
	if(addr&RADIO_READ_ACCESS)
	{
		if(addr&RADIO_BURST_ACCESS)
		{
			for (i = 0; i < len; i++)
			{
				*pData = spi_send_rcv(0);
				pData++;
			}
		}
		else
		{
			*pData = spi_send_rcv(0);
		}
	}
	else
	{
		if(addr&RADIO_BURST_ACCESS)
		{
			for (i = 0; i < len; i++)
			{
				spi_send_rcv(*pData);
				pData++;
			}
		}
		else
		{
			spi_send_rcv(*pData);
		}
	}
	return;
}
rfStatus_t trx8BitRegAccess(uint8 accessType, uint8 addrByte, uint8 *pData, uint16 len)
{
	uint8 readValue;
	RF_SPI_BEGIN();
	while(miso());
	readValue = spi_send_rcv(accessType|addrByte);
	trxReadWriteBurstSingle(accessType|addrByte,pData,len);
	RF_SPI_END();
	return(readValue);
}

rfStatus_t trxSpiCmdStrobe(uint8 cmd)
{
	uint8 rc;
	RF_SPI_BEGIN();
	while(miso());
	rc = spi_send_rcv(cmd);
	RF_SPI_END();
	return(rc);
}



