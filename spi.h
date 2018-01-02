#ifndef _SPI_H
#define _SPI_H
#include "macro.h"
#define     RF_PORT_SEL            P1SEL
#define     RF_PORT_OUT            P1OUT
#define     RF_PORT_DIR            P1DIR
#define     RF_PORT_IN             P1IN

#define     RF_MOSI_PIN            BIT7
#define     RF_MISO_PIN            BIT6
#define     RF_SCLK_PIN            BIT5

#define     RF_CS_N_PORT_SEL       P2SEL
#define     RF_CS_N_PORT_DIR       P2DIR
#define     RF_CS_N_PORT_OUT       P2OUT
#define     RF_CS_N_PIN            BIT2

#define 	RF_POWER_N_PORT_SEL 	P2SEL
#define     RF_POWER_N_PORT_DIR     P2DIR
#define     RF_POWER_N_PORT_OUT     P2OUT
#define     RF_POWER_N_PIN          BIT5

#define     RF_LED_SEL            P1SEL
#define     RF_LED_OUT            P1OUT
#define     RF_LED_DIR            P1DIR
#define     RF_LED_N_PIN           BIT4


/*use GDO2 as int */
#define     RF_PORT_VECTOR         PORT2_VECTOR
#define     RF_GDO_OUT             P2OUT
#define     RF_GDO_DIR             P2DIR
#define     RF_GDO_IN              P2IN
#define     RF_GDO_SEL             P2SEL
#define     RF_GDO_PxIES           P2IES
#define     RF_GDO_PxIFG           P2IFG
#define     RF_GDO_PxIE            P2IE
#define     RF_GDO_PIN             BIT0


#define RF_SPI_BEGIN()              st( RF_CS_N_PORT_OUT &= ~RF_CS_N_PIN; NOP(); )
#define RF_SPI_TX(x)                st( IFG2 &= ~UCB0RXIFG; UCB0TXBUF= (x); )
#define RF_SPI_WAIT_DONE()          st( while(!(IFG2 & UCB0RXIFG)); )
#define RF_SPI_WAIT_TX_DONE()       st( while(!(IFG2 & UCB0TXIFG)); )
#define RF_SPI_RX()                 UCB0RXBUF
#define RF_SPI_END()                st( NOP(); RF_CS_N_PORT_OUT |= RF_CS_N_PIN; )
void trxRfSpiInterfaceInit(uint8 prescalerValue);
rfStatus_t trx8BitRegAccess(uint8 accessType, uint8 addrByte, uint8 *pData, uint16 len);
rfStatus_t trxSpiCmdStrobe(uint8 cmd);
#endif
