EXTRA_DIST=ldscript.lds

bin_PROGRAMS=Parthenope

Parthenope_SOURCES=\
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
	support.c\
	support.h\
	tftp.c\
	tftp.h\
	parthenope.c

Parthenope_LDFLAGS = -nostartfiles -nodefaultlibs -static -Wl,-s -Wl,-N -Wl,--entry=bootstrap -Wl,--script=ldscript.lds
CFLAGS = -O0 -fno-builtin
