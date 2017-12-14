#include <msp430g2553.h>
#include <stdint.h>
#include "spi.h"
#include "cc1101_def.h"
#include "cc1101.h"
#include "dbg.h"
#define RF_XTAL 26000
#define SCALING_FREQ     (float)((RF_XTAL)/65.536)
#define SCALING_FREQEST  (unsigned long)((RF_XTAL)/16.384)
unsigned char volatile timer_event;
unsigned long volatile time_counter = 0;

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
void hal_timer_init(unsigned int master_count) {

  // Start Timer 0 using the ACLK as a source (this enables the use of
  // various low power modes). Timer 0 will be used to keep RF burst time
  TA0CCR0  = master_count - 1;              // Seting for MASTER SCHEDULE
  TA0CCR1  = 0;                             // will be used for burst alignnment
  TA0CCR2  = 0;                             // will be used for expiration counter
  TA0CTL   = TASSEL_1 + MC_1 + TACLR + ID_0;// ACLK, Up to CCR0, clear TB0R, div/1

  return;
}
unsigned int hal_timer_wait(unsigned int time) {
  unsigned int wait_count, TBR_init;

  TBR_init = TA0R;   // store the current value of the timer register
  wait_count = TBR_init + time;

  // if the requested wait time exceeds the TBCCR0 (max value) then make a loop
  while(wait_count > TA0CCR0) {

	  // configure the timeout for 1 less than the master clock
	  TA0CCR2  = TA0CCR0-1;

	  // calculate the remaining wait time remaining
	  wait_count = wait_count - (TA0CCR2 - TBR_init);

	  // do not count the initial timer values more that once, zero it out
	  timer_event = 0;
	  TBR_init = 0;

		while (!(TA0CCTL1 & CCIFG) && !(TA0CCTL2 & CCIFG) && !(RF_GDO_PxIFG & RF_GDO_PIN));
		if (TA0CCTL1 & CCIFG) timer_event = TA0IV_TACCR1;
		else if (TA0CCTL2 & CCIFG) timer_event = TA0IV_TACCR2;
		else if (RF_GDO_PxIFG & RF_GDO_PIN) timer_event = 0;
		TA0CCTL1 &= ~(CCIFG);
		TA0CCTL2 &= ~(CCIFG);
		RF_GDO_PxIFG &= ~RF_GDO_PIN;

      // check to see if the timer woke us up or not
	  if (timer_event == 0)
		  // it did not, return imidiately and note time actual delay
		  return (time - (wait_count - TA0R));
  }

  // in the case of loop, this executes the remaining wait, in the case of no
  // loop this is the only wait that gets executed

  /* define maximum timeout by using timer counter 2 */
  TA0CCR2  = wait_count;

  


	while (!(TA0CCTL1 & CCIFG) && !(TA0CCTL2 & CCIFG) && !(RF_GDO_PxIFG & RF_GDO_PIN));
	if (TA0CCTL1 & CCIFG) timer_event = TA0IV_TACCR1;
	else if (TA0CCTL2 & CCIFG) timer_event = TA0IV_TACCR2;
	else if (RF_GDO_PxIFG & RF_GDO_PIN) timer_event = 0;
	TA0CCTL1 &= ~(CCIFG);
	TA0CCTL2 &= ~(CCIFG);
	RF_GDO_PxIFG &= ~RF_GDO_PIN;

  /* return the time spend in sleep */
  	if (timer_event == 0) return 0;
	else return (time - (wait_count-TA0R));
}
unsigned char set_rf_packet_length(unsigned char length) {
  unsigned char reg_value;

  /* make sure we are in fixed packet mode */
  reg_value = 0x04;
  trx8BitRegAccess(RADIO_WRITE_ACCESS, PKTCTRL0, &reg_value, 1);

  /* set the fixed packet length */
  trx8BitRegAccess(RADIO_WRITE_ACCESS, PKTLEN, &length, 1);

  return (0);
}
int radio_get_rssi(void) {
  int rssi;
  unsigned char cc_rssi;
  
  trx8BitRegAccess(RADIO_READ_ACCESS | RADIO_BURST_ACCESS, RSSI, &cc_rssi, 1);
  
  if (cc_rssi >= 128) {
    rssi = ((cc_rssi-256)>>1) - 72;
  } else {
    rssi = (cc_rssi>>1) - 72;
  }
  return rssi; 
}

char get_device_id(void) {
  unsigned char ret_partnum;
  unsigned char ret_version;
  
  trx8BitRegAccess(RADIO_READ_ACCESS+RADIO_BURST_ACCESS, VERSION, &ret_version, 1);
  trx8BitRegAccess(RADIO_READ_ACCESS+RADIO_BURST_ACCESS, PARTNUM, &ret_partnum, 1);

  switch (ret_partnum) {
  case 0:
    if(ret_version == 0x04) {
      return DEV_CC1101;
    } 
    if(ret_version == 0x07) {
      return DEV_CC1101;
    } 
    if(ret_version == 0x06) {
      return DEV_CC430x;
    }    
    if(ret_version == 0x00) {
      return DEV_CC1100;
    }
    break;
  case 128:
    if(ret_version == 0x03) {
      return DEV_CC2500;
    }
    break;
  default:
    break;
  }
  
  return DEV_UNKNOWN;
}

int radio_init(void)
{

	unsigned char i, writeByte, preferredSettings_length;
	registerSetting_t *preferredSettings;

	hal_timer_init(32768);
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
	radio_set_freq(902750);
	set_rf_packet_length(TX_BUF_SIZE);
	radio_receive_on();
	return 0;
}
int radio_receive_on(void) {

	/* Range extender in RX mode */
#ifdef ENABLE_RANGE_EXTENDER
	range_extender_rxon();
#endif

	trxSpiCmdStrobe(RF_SRX);                 // Change state to TX, initiating

	return(0);
}
int radio_send(unsigned char *payload, unsigned short payload_len) {

	trx8BitRegAccess(RADIO_WRITE_ACCESS|RADIO_BURST_ACCESS, TXFIFO, payload, payload_len);

	/* Range extender in TX mode */
#ifdef ENABLE_RANGE_EXTENDER
	range_extender_txon();
#endif

	trxSpiCmdStrobe(RF_STX);               // Change state to TX, initiating
	return(0);
}
int radio_read(unsigned char *buf, unsigned short *buf_len) {
	unsigned char status;
	unsigned char pktLen;

	/* Read number of bytes in RX FIFO */
	trx8BitRegAccess(RADIO_READ_ACCESS|RADIO_BURST_ACCESS, RXBYTES, &pktLen, 1);
	pktLen = pktLen  & NUM_RXBYTES;

	/* make sure the packet size is appropriate, that is 1 -> buffer_size */
	if ((pktLen > 0) && (pktLen <= *buf_len)) {

		/* retrieve the FIFO content */
		trx8BitRegAccess(RADIO_READ_ACCESS|RADIO_BURST_ACCESS, RXFIFO, buf, pktLen);

		/* return the actual length of the FIFO */
		*buf_len = pktLen;

		/* retrieve the CRC status information */
		trx8BitRegAccess(RADIO_READ_ACCESS+RADIO_BURST_ACCESS, PKTSTATUS, &status, 1);

	} else {

		/* if the length returned by the transciever does not make sense, flush it */
		*buf_len = 0;                                // 0
		status = 0;
		trxSpiCmdStrobe(RF_SFRX);	                 // Flush RXFIFO
	}

	/* return status information, CRC OK or NOT OK */
	return (status & CRC_OK);
}
int radio_wait_for_idle(unsigned short max_hold) {

	unsigned int status = 0;
	unsigned char reg_status;

	/* check that we are still in RX mode before entering wait for RX end */
	trx8BitRegAccess(RADIO_READ_ACCESS+RADIO_BURST_ACCESS, MARCSTATE, &reg_status, 1);

	/* filter out only the status section of the register values */
	reg_status  = (reg_status & 0x1F);

	/* check for not idle mode */
	if(!(reg_status == MARCSTATE_IDLE)) {

		rf_end_packet = 0;  // initialize global variable for use in this function

		RF_GDO_PxIES |= RF_GDO_PIN;       // Int on falling edge (end of pkt)
		RF_GDO_PxIFG &= ~RF_GDO_PIN;      // Clear flag


		/* wait for idle */
		if(max_hold > 0) {
			status = hal_timer_wait(max_hold);    // this will timeout either with GDO or timer
			if (status == 0) rf_end_packet = 1;
		} else {
			// wait for radio to interupt us to continue processing
			status = 0;

			while(!(RF_GDO_PxIFG & RF_GDO_PIN));
			// indicate that end of packet has been found
			rf_end_packet = 1;
			// clear the interrupt flag
			RF_GDO_PxIFG &= ~RF_GDO_PIN;
		}


	}
	/* Get timer values, however if we did not get a packet in time use 0      */
	if(rf_end_packet == 0) {
		status = max_hold;
	}

#ifdef ENABLE_RANGE_EXTENDER
	range_extender_idle();
#endif

	return status;
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

	// upload the frequency word to the transciver using a 3 byte write
	trx8BitRegAccess(RADIO_WRITE_ACCESS | RADIO_BURST_ACCESS , FREQ2, freq_byte, 3);

	return 0;
}
int radio_idle(void) {

	/* Idle range extender */
#ifdef ENABLE_RANGE_EXTENDER
	range_extender_idle();
#endif

	/* Idle range extender */
	trxSpiCmdStrobe(RF_SIDLE);

	/* Flush the FIFO's */
	trxSpiCmdStrobe(RF_SFRX);
	trxSpiCmdStrobe(RF_SFTX);
	return(0);
}

