# makefile, written by guido socher
MCU=atmega168
#MCU=at90s4433
CC=avr-gcc
#CEXTRA=-Wa,-adhlns=$(<:.c=.lst)
#EXTERNAL_RAM = -Wl,--defsym=__heap_start=0x801100,--defsym=__heap_end=0x80ffff
#EXTERNAL_RAM = -Wl,-Tdata=0x801100,--defsym=__heap_end=0x80ffff
LDFLAGS  = -mmcu=${MCU} -Wl,-u, -lm
#LDFLAGS  = -mmcu=${MCU} -Wl,-u,vfprintf -lprintf_flt -lm
OBJCOPY=avr-objcopy
# optimize for size:
#CFLAGS=-g -mmcu=$(MCU) -Wall -Wstrict-prototypes -mcall-prologues ${CEXTRA}
CFLAGS=-mmcu=$(MCU) -Os -D'AVR_IS'
DEVICE = m168
AVRDUDE = avrdude -c usbasp -p $(DEVICE)
FUSEH = 0xdf
FUSEL = 0xf7


#-------------------
all: erdsir_wormversion.hex
#-------------------
help: 
	@echo "Usage: make all|load|load_pre|rdfuses|wrfuse1mhz|wrfuse4mhz|wrfusecrystal"
	@echo "Warning: you will not be able to undo wrfusecrystal unless you connect an"
	@echo "         external crystal! uC is dead after wrfusecrystal if you do not"
	@echo "         have an external crystal."
#-------------------
erdsir_wormversion.hex : erdsir_wormversion.out 
	$(OBJCOPY) -R .eeprom -O ihex erdsir_wormversion.out erdsir_wormversion.hex 
#	$(OBJCOPY) -O ihex erdsir_wormversion.out erdsir_wormversion.hex 
#erdsir_wormversion.out : erdsir_wormversion.o 
#	$(CC) $(CFLAGS) -o erdsir_wormversion.out -Wl,-Map,erdsir_wormversion.map erdsir_wormversion.o 
erdsir_wormversion.out : erdsir_wormversion.o 
	$(CC) ${LDFLAGS} $(CFLAGS) -o erdsir_wormversion.out  erdsir_wormversion.o 


erdsir_wormversion.o : erdsir_wormversion.c 
	$(CC) $(CFLAGS) -Os -c erdsir_wormversion.c

erdsir_wormversion.elf: erdsir_wormversion.o
	$(CC) ${LDFLAGS} $(CFLAGS) -o erdsir_wormversion.elf erdsir_wormversion.o

disasm:	erdsir_wormversion.elf
	avr-objdump -d noise.elf

fuse:
	$(AVRDUDE) -F -U hfuse:w:$(FUSEH):m -U lfuse:w:$(FUSEL):m

flash: all
	$(AVRDUDE) -F -U flash:w:erdsir_wormversion.hex:i



#-------------------
clean:
	rm -f *.o *.map *.out *t.hex
#-------------------
