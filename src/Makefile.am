EXTRA_DIST=ldscript.lds

bin_PROGRAMS=OS4Bootloader

OS4Bootloader_SOURCES=\
	context.h\
	uboot.h\
	device.h\
	menu.h\
	menu.c\
	support.h\
	support.c\
	tftp.h\
	tftp.c\
	OS4Bootloader.c

OS4Bootloader_LDFLAGS = -nostartfiles -nodefaultlibs -static -Wl,-s -Wl,-N -Wl,--entry=bootstrap -Wl,--script=ldscript.lds
