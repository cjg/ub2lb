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
	rdb.h\
	sfs.c\
	sfs.h\
	support.c\
	support.h\
	tftp.c\
	tftp.h\
	parthenope.c

Parthenope_LDFLAGS = -nostartfiles -nodefaultlibs -static -Wl,-s -Wl,-N -Wl,--entry=bootstrap -Wl,--script=ldscript.lds
CFLAGS = -Wall -fno-builtin -nostdinc -nostdlib -Os -ffixed-r2 -ffixed-r14 -ffixed-r29 -DVERSION=\"`svn info --xml |grep revision=\" | head -n 1 | sed -e 's|[^"]*"||' -e 's|"[^"]*||'`\"

