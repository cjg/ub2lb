EXTRA_DIST=ldscript.lds

SOURCES=\
	cdrom.c\
	cdrom.h\
	context.c\
	context.h\
	uboot.h\
	device.h\
	elf.c\
	elf.h\
	ext2.c\
	ext2.h\
	image.h\
	menu.c\
	menu.h\
	rdb.c\
	rdb.h\
	sfs.c\
	sfs.h\
	support.c\
	support.h\
	tftp.c\
	tftp.h\
	parthenope.c

LDFLAGS = -nostartfiles -nodefaultlibs -static -Wl,-s -Wl,-N -Wl,--entry=bootstrap -Wl,--script=ldscript.lds
CFLAGS = -Wall -W -Werror -fno-builtin -nostdinc -nostdlib -DIN_PARTHENOPE -Os -DVERSION=\"91\" -DDATE=\"`date +%-d.%-m.%Y`\"
CFLAGS += -ffixed-r2 -ffixed-r14 -ffixed-r29 -DUSE_UBOOT

OBJECTS := $(patsubst %.c,%.o,$(filter %.c, $(SOURCES)))

ifdef CROSS_COMPILE
	CC = $(CROSS_COMPILE)gcc
endif

all: ub2lb

ub2lb: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJECTS) -o ub2lb
	
%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c $<
	
clean:
	-rm -rf ub2lb $(OBJECTS)

