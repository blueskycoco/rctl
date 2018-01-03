#ifndef _DBG_H
#define _DBG_H
void uart_write_string(char *buf);
void uart_init(void);
void uprintf(const char *fmt, ...);
#endif
