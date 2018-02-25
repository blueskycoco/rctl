#ifndef _CC1101_H
#define _CC1101_h
#include "macro.h"
#define     RADIO_BURST_ACCESS      0x40
#define     RADIO_SINGLE_ACCESS     0x00
#define     RADIO_READ_ACCESS       0x80
#define     RADIO_WRITE_ACCESS      0x00
#define DEV_UNKNOWN       10
#define DEV_CC1100        11
#define DEV_CC1101        12
#define DEV_CC2500        13
#define DEV_CC430x        14
#define DEV_CC1120        15
#define DEV_CC1121        16
#define DEV_CC1125        17
#define DEV_CC1200        18
#define DEV_CC1201        19
#define DEV_CC1175        20

#define RADIO_GENERAL_ERROR     0x00
#define RADIO_CRC_OK            0x80
#define RADIO_IDLE              0x81
#define RADIO_RX_MODE           0x82
#define RADIO_TX_MODE           0x83
#define RADIO_RX_ACTIVE         0x84
#define RADIO_TX_ACTIVE         0x85
#define RADIO_SLEEP             0x86
#define RADIO_TX_PACKET_RDY     0x87
#define RADIO_CHANNEL_NOT_CLR   0x88
#define RADIO_CHANNEL_IS_CLR    0x89
#define TX_BUF_SIZE 128


#define ID_CODE			0x00000112
#define RAND_DELAY		85
typedef struct
{
    uint16  addr;
    uint8   data;
}registerSetting_t;
int radio_init(void);
int radio_wait_for_idle(unsigned short max_hold);
int radio_send(unsigned char *payload, unsigned char payload_len);
int radio_read(unsigned char *buf, unsigned char *buf_len);
int radio_set_freq(unsigned long freq);
int radio_receive_on(void);
unsigned short CRC(unsigned char *Data,unsigned char Data_length);
unsigned short read_adc();
void radio_sleep();
#endif
