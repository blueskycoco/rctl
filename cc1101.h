#ifndef _CC1101_H
#define _CC1101_h
#include "macro.h"
#define     RADIO_BURST_ACCESS      0x40
#define     RADIO_SINGLE_ACCESS     0x00
#define     RADIO_READ_ACCESS       0x80
#define     RADIO_WRITE_ACCESS      0x00
typedef struct
{
    uint16  addr;
    uint8   data;
}registerSetting_t;
int radio_init(void);
#endif
