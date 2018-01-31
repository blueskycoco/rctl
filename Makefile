MCU=msp430g2553

SOURCES :=
CFLAGS= -mmcu=${MCU} -g -O2 -Wall -Wunused
ifeq ($(APPNAME), hand)
SOURCES += main.c spi.c cc1101.c hand.c
CFLAGS += -DHAND
endif
ifeq ($(APPNAME), door)
SOURCES += main.c spi.c cc1101.c door.c
endif
ifeq ($(APPNAME), infrar)
SOURCES += main.c spi.c cc1101.c infrar.c
endif
VPATH = ./src
BUILD_ROOT = ./obj
ALL_SOURCE_CODE_OBJS = $(addprefix $(BUILD_ROOT)/, $(patsubst %.c, %.o, $(SOURCES)))
SUPPORT_PATH=c:/ti/msp430_gcc

CC=msp430-elf-gcc
OBJCOPY=msp430-elf-objcopy
SIZE=msp430-elf-size

INCLUDES = -I${SUPPORT_PATH}/include -I./inc
LDLIBS = -L${SUPPORT_PATH}/include

all: ${APPNAME}.hex

$(ALL_SOURCE_CODE_OBJS): $(BUILD_ROOT)/%.o: %.c
	${CC} ${CFLAGS} ${INCLUDES} -c -o $@ $<

obj/${APPNAME}.elf: ${ALL_SOURCE_CODE_OBJS}
	${CC} -mmcu=${MCU} ${LDLIBS} -o $@ $^

${APPNAME}.hex: obj/${APPNAME}.elf
	${OBJCOPY} -O ihex $< bin/$@
	${SIZE} $<
${APPNAME}.bin: ${APPNAME}.elf
	${OBJCOPY} -S -O binary $< $@
	${SIZE} $<

clean:
	rm obj/*.o obj/${APPNAME}.elf bin/${APPNAME}.hex ${APPNAME}.bin
