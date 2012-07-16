PROJECT=uip_demo.elf
PROJECT_MAP=$(PROJECT:.elf=.map)
PROJECT_LST=$(PROJECT:.elf=.lst)
PROJECT_HEX=$(PROJECT:.elf=.hex)

CC=rx-elf-gcc
AS=rx-elf-gcc
LD=rx-elf-gcc
SIZE=rx-elf-size
OBJDUMP=rx-elf-objdump
OBJCOPY=rx-elf-objcopy
FLASH_TOOL=rxusb

#	-Wredundant-decls \
#	-Wshadow \

CFLAGS=\
	-I. \
	-Ibsp \
	-Idriver \
	-Iuip \
	-Iuip/uip \
	-Iuip/lib \
	-Iuip/yrdkrx62n \
	-Iuip/apps/dhcpc \
	-Iuip/apps/webserver \
	-Iuser-app \
	-D__LIT=1 \
	-O2 \
	-g2 \
	-Wall \
	-Wextra \
	-Wnested-externs \
	-Wpointer-arith \
	-Wswitch \
	-Wreturn-type \
	-Wstrict-prototypes \
	-Wunused \
	-Wno-main \
	\
	-Wno-strict-aliasing \
	-Wno-unused-but-set-variable \
	-Wno-unused-parameter \
	-Wno-missing-field-initializers \
	-Wno-sign-compare \
	\
	-MMD \
	-mlittle-endian-data \
	-mint-register=0 \
	-ffunction-sections \
	-fdata-sections \
	-std=gnu99 \
	$(END)

ASFLAGS=\
	$(END)

LDFLAGS=\
	-nostartfiles \
	-Wl,--gc-sections \
	-Wl,-Map=$(PROJECT_MAP) \
	-T bsp/RX62N8.ld \
	$(END)

SRC=\
	driver/phy.c \
	driver/r_ether.c \
	uip/apps/dhcpc/dhcpc.c \
	uip/apps/webserver/http-strings.c \
	uip/apps/webserver/httpd-cgi.c \
	uip/apps/webserver/httpd-fs.c \
	uip/apps/webserver/httpd.c \
	uip/uip/psock.c \
	uip/uip/timer.c \
	uip/uip/uip-fw.c \
	uip/uip/uip-neighbor.c \
	uip/uip/uip-split.c \
	uip/uip/uip.c \
	uip/uip/uip_arp.c \
	uip/uip/uiplib.c \
	uip/yrdkrx62n/clock-arch.c \
	uip/yrdkrx62n/main.c \
	user-app/user-app.c \
	bsp/isr_vectors.c \
	bsp/hwsetup.c \
	bsp/font_x5x7.c \
	bsp/lcd.c \
	$(END)

OBJ=$(SRC:.c=.o) bsp/crt0.o
DEP=$(OBJ:.o=.d)

all: $(PROJECT)

$(PROJECT): $(OBJ)
	@echo -e "\tLD\t"$@
	@$(LD) $(LDFLAGS) -o $@ $^
	@echo -e "\tSIZE\t"$@
	@$(SIZE) $@

%.o: %.c
	@echo -e "\tCC\t"$@
	@$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S
	@echo -e "\tAS\t"$@
	@$(AS) $(ASFLAGS) -c -o $@ $<

%.lst: %.elf
	@echo -e "\tOBJDUMP\t"$@
	@$(OBJDUMP) -DS $^ > $@

%.hex: %.elf
	@echo -e "\tOBJCOPY\t"$@
	@$(OBJCOPY) -Oihex $^ $@

flash: $(PROJECT)
	@$(FLASH_TOOL) -v $<

clean:
	@rm -f $(OBJ) $(DEP) $(PROJECT) $(PROJECT_MAP) $(PROJECT_LST) $(PROJECT_HEX)

-include $(DEP)
