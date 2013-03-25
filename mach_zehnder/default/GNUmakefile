###############################################################################
# Makefile for the project Mach-Zehnder 
###############################################################################

## General Flags
PROJECT = MachZehnder 
MCU = atxmega32a4
TARGET = MachZehnder.elf
CC = avr-gcc
CPR = avr-cpp

CPP = avr-g++

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2 -std=gnu99 -DF_CPU=32000000UL -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
#CFLAGS += -Wall -std=gnu99 -DF_CPU=32000000UL -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -MD -MP -MT $(*F).o -MF dep2/$(@F).d 

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS +=  -Wl,-T,MachZehnder.lds  -Wl,-Map=MachZehnder.map
#LDFLAGS +=  -Wl,-Map=poly.map -Wl,-verbose


## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Objects that must be built in order to link
OBJECTS = main.o events.o timer.o console.o editor.o eeprom.o i2cmaster.o interpreter.o twislave.o uartc0.o hex.o adc.o bitops.o calib.o pwm.o tx.o wdt.o control.o switch_context.o ad537x.o threads.o tpos.o sweep.o

## Objects explicitly added by the user
LINKONLYOBJECTS = 

## Build
all: $(TARGET) MachZehnder.hex MachZehnder.eep MachZehnder.lss size

%.s: %.asm
	$(CPR) $< -o $@

%.o: %.s
	$(CC) -c $< -o $@

## Compile
main.o: ../main.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

events.o: ../events.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

timer.o: ../timer.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

console.o: ../console.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

editor.o: ../editor.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

eeprom.o: ../eeprom.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

i2cmaster.o: ../i2cmaster.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

interpreter.o: ../interpreter.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

twislave.o: ../twislave.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

uartc0.o: ../uartc0.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

hex.o: ../hex.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

tpos.o: ../tpos.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

adc.o: ../adc.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

bitops.o: ../bitops.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

calib.o: ../calib.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

pwm.o: ../pwm.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

wdt.o: ../wdt.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

tx.o: ../tx.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

threads.o: ../threads.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

control.o: ../control.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

ad537x.o: ../ad537x.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

sweep.o: ../sweep.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

switch_context.o: ./switch_context.s
	$(CC) $(ASMFLAGS) -c $< -o $@

switch_context.s: ../switch_context.asm
	$(CPR) $< -o $@

##Link
$(TARGET): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(TARGET)
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

size: ${TARGET}
	@echo
	@avr-size -C --mcu=${MCU} ${TARGET}

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) MachZehnder.elf dep2/* MachZehnder.hex MachZehnder.eep MachZehnder.lss MachZehnder.map


## Other dep2endencies
-include $(shell mkdir dep2 2>/dev/null) $(wildcard dep2/*)

