VPATH=..
CROSS=rx-elf-
CFLAGS=-g -O2 -Wall -fno-strict-overflow -funsigned-char -Wstrict-overflow=3 -Wno-unused-but-set-variable 
CC=$(CROSS)gcc
HOSTCC=gcc
AS=$(CROSS)as
CPP=$(CROSS)cpp
OBJCOPY=$(CROSS)objcopy
ECHO=echo

SUBMODULES=
INCLUDES += $(patsubst %,-I$(VPATH)/%,$(SUBMODULES)) -I$(VPATH)

.PHONY: depend clean all $(SUBMODULES)

SRCFILES= main.c editor.c hardware_setup.c interpreter.c sci0.c timer.c \
	vector_table.c console.c  events.c  hex.c interrupt_handlers.c  \
	threads.c tpos.c rxether.c phy.c base64.c sha1.c md5.c \
	wdta.c ethernet.c spi.c fat.c fatcmds.c diskio.c sdcard.c \
	tcp.c xy_string.c iram.c rx_crc.c strhash.c ccsbcs.c \
	ad537x.c mdio.c cdr.c shiftreg.c i2cmaster.c skb.c  xy_web.c \
	pvar_websock.c adc12.c pvar.c usbdev.c usbstorage.c \
	leds.c xo.c version.c bootloader.c dataflash.c swupdate.c \
	bert.c pat_trig.c

ASMFILES=reset_program.asm switch_context.asm bootloader_setup.asm


%.s: %.asm
	$(CPP) $< -o $@

.c.o:
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) -c $< -o $@

default: all

ifneq ($(MAKECMDGOALS),clean)
-include .depend
endif

include $(patsubst %,%/module.mk,$(SUBMODULES))

OBJS := $(SRCFILES:.c=.o) $(ASMFILES:.asm=.o)


version.o: $(SRCFILES) $(ASMFILES)
	$(CC) $(CFLAGS) -c $(VPATH)/version.c -o $@


FWNAME := CBEe$(shell date +"%d%m%y").fw

all: depend bert100.mot bert100.fw $(FWNAME)

bert100.bin: bert100.mot
	$(OBJCOPY) --gap-fill 0xff -I srec -O binary $< $@	

bert100.fw: bert100.bin crcsign 
	./crcsign $< $@

$(FWNAME): bert100.fw
	cp $< $@

bert100: $(OBJS) $(VPATH)/bert100.lds
	$(CC)  -nostartfiles -T $(VPATH)/bert100.lds $(OBJS) -o bert100  -lm  -Wl,-Map,bert100.map

bert100.mot: bert100
	$(OBJCOPY) $< -O srec  $@

crcsign: crcsign.c
	$(HOSTCC) -Wall $< -o $@

clean::
	rm -f *.bin bert100.fw $(OBJS) *.s bert100.mot bert100 .depend *.map

depend: .depend

#####################################################
# make depend does not work for shared libraries
# currently  (needs an .so dependecy not an .o)
#####################################################
.depend:
	rm -f .depend
	for file in $(SRCFILES); do \
		var=$$(dirname $$file); \
		$(ECHO) -n "$$var/" > .tmpdepend; \
		$(CC) -MM  $(INCLUDES)  $(VPATH)/$$file >> .tmpdepend && \
		cat .tmpdepend >> .depend; \
	done; \
	rm -f .tmpdepend
