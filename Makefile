APPNAME=rctl
MCU=msp430g2553

OBJECTS :=
OBJECTS += main.o spi.o dbg.o cc1101.o

SUPPORT_PATH=c:/ti/msp430_gcc

CC=msp430-elf-gcc
OBJCOPY=msp430-elf-objcopy
SIZE=msp430-elf-size

CFLAGS= -mmcu=${MCU} -g -O2 -Wall -Wunused
INCLUDES = -I${SUPPORT_PATH}/include
LDLIBS = -L${SUPPORT_PATH}/include

all: ${APPNAME}.hex ${APPNAME}.bin

%.o: %.c
	${CC} ${CFLAGS} ${INCLUDES} -c -o $@ $<

${APPNAME}.elf: ${OBJECTS}
	${CC} -mmcu=${MCU} ${LDLIBS} -o $@ $^

${APPNAME}.hex: ${APPNAME}.elf
	${OBJCOPY} -O ihex $< $@
${APPNAME}.bin: ${APPNAME}.elf
	${OBJCOPY} -S -O binary $< $@
	${SIZE} $<

clean:
	rm *.o ${APPNAME}.elf ${APPNAME}.hex ${APPNAME}.bin
