#ifndef _MACRO_H
#define _MACRO_H
typedef signed   char   int8;
typedef unsigned char   uint8;

typedef signed   short  int16;
typedef unsigned short  uint16;

typedef signed   long   int32;
typedef unsigned long   uint32;
typedef uint8 rfStatus_t;
#define NOP()  asm(" nop")
#define st(x)      do { x } while (__LINE__ == -1)
#endif
