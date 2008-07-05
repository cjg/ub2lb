/* parthenope.c */

/* <project_name> -- <project_description>
 *
 * Copyright (C) 2006 - 2007
 *     Giuseppe Coviello <cjg@cruxppc.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "context.h"
#include "device.h"
#include "tftp.h"
#include "ext2.h"
#include "menu.h"
#include "elf.h"
#include "image.h"
#include "cdrom.h"

extern unsigned long __bss_start;
extern unsigned long _end;

#define __startup __attribute__((section(".aros.startup")))

static void clear_bss()
{
	unsigned long *ptr = &__bss_start;
	while (ptr < &_end)
		*ptr++ = 0;
}

static void flush_cache(char *start, char *end)
{
    start = (char*)((unsigned long)start & 0xffffffe0);
    end = (char*)((unsigned long)end & 0xffffffe0);
    char *ptr;
    
    for (ptr = start; ptr < end; ptr +=32)
    {
        asm volatile("dcbst 0,%0"::"r"(ptr));
    }
    asm volatile("sync");

    for (ptr = start; ptr < end; ptr +=32)
    {
        asm volatile("icbi 0,%0"::"r"(ptr));
    }
    
    asm volatile("sync; isync; ");
}

static boot_dev_t *get_booting_device(void)
{
	SCAN_HANDLE hnd;
	
	hnd = get_curr_device();

	switch(hnd->ush_device.type) {
	case DEV_TYPE_HARDDISK:
		return ext2_guess_booting(0);
		break;
	case DEV_TYPE_NETBOOT:
		return tftp_create();
		break;
	case DEV_TYPE_CDROM:
		return cdrom_create();
		break;
	}

	return NULL;
}

void testboot_linux(menu_t *entry, void *kernel, boot_dev_t *dev)
{
	image_header_t *header;
	char *argv[3];
	int argc;
	void *initrd;

	header = kernel;
	
	if(header->ih_magic != IH_MAGIC 
	   || header->ih_type != IH_TYPE_KERNEL
	   || header->ih_os != IH_OS_LINUX)
		return;

	setenv("stdout", "vga");
	printf("We should boot %s:\n\t%s %s\n\t%s\n", entry->title,
	       entry->kernel, entry->append, entry->initrd);
	
	if(entry->append != NULL)
		setenv("bootargs", entry->append);
	else
		setenv("bootargs", "");

	argc = 2;
	argv[0] = "bootm";
	argv[1] = malloc(32);
	sprintf(argv[1], "%p", kernel);
	argv[2] = NULL;

	if(entry->initrd != NULL) {
		initrd = malloc(18 * 1024 * 1024);
		dev->load_file(dev, entry->initrd, initrd);
		argc = 3;
		argv[2] = malloc(32);
		sprintf(argv[2], "%p", initrd);
	}

	bootm(NULL, 0, argc, argv);
}

void testboot_standalone(menu_t *entry, void *kernel, boot_dev_t *dev)
{
	image_header_t *header;
	char *argv[2];
	int argc;

	header = kernel;
	
	if(header->ih_magic != IH_MAGIC 
	   || header->ih_type != IH_TYPE_STANDALONE)
		return;

	setenv("autostart", "yes");
	setenv("stdout", "vga");

	argc = 2;
	argv[0] = "bootm";
	argv[1] = malloc(32);
	sprintf(argv[1], "%p", kernel);

	bootm(NULL, 0, argc, argv);
}

int max_entries;
static void set_progress(int progress)
{
	int p = progress * 66 / max_entries;
	video_repeat_char(7, 7, p, 219, 0);
	video_repeat_char(7+p, 7, 66-p, 177, 0);
}

void testboot_aros(menu_t *menu, void *kernel, boot_dev_t *boot)
{
	int i;
        char tmpbuf[100];
	void *file_buff = malloc(5*1024*1024);
	tagitem_t items[50];
	tagitem_t *tags = &items[0];

	if (!load_elf_file(kernel))
		return;

	max_entries = menu->modules_cnt + 1;
	sprintf(tmpbuf, "Booting %s", menu->title);
	video_clear();
        video_set_partial_scroll_limits(12, 25);
                    
	video_draw_box(1, 0, tmpbuf, 1, 5, 4, 70, 7);
	set_progress(1);
	video_draw_text(7, 9, 0, menu->kernel, 66);
                    
	
	for (i=0; i < menu->modules_cnt; i++) {
		printf("[BOOT] Loading file '%s'\n", menu->modules[i]);
		if (boot->load_file(boot, menu->modules[i], file_buff) < 0) {
			return;
		}
		if (!load_elf_file(file_buff)) {
			printf("[BOOT] Load ERRRO\n");
			return;
		}
		set_progress(i + 2);
		video_draw_text(7, 9, 0, menu->modules[i], 66);
	}
                        
	void (*entry)(void *);
	flush_cache(get_ptr_rw(), get_ptr_ro());
                            
	printf("[BOOT] Jumping into kernel\n");
	entry = (void *)KERNEL_PHYS_BASE;
                            
	tags->ti_tag = KRN_KernelBss;
	tags->ti_data = (unsigned long)&tracker[0];
	tags++;
                            
	tags->ti_tag = KRN_KernelLowest;
	tags->ti_data = (unsigned long)get_ptr_rw();
	tags++;
	
	tags->ti_tag = KRN_KernelHighest;
	tags->ti_data = (unsigned long)get_ptr_ro();
	tags++;
	
	tags->ti_tag = KRN_KernelBase;
	tags->ti_data = (unsigned long)KERNEL_PHYS_BASE;
	tags++;
	
	tags->ti_tag = KRN_ProtAreaStart;
	tags->ti_data = (unsigned long)KERNEL_VIRT_BASE;
	tags++;
	
	tags->ti_tag = KRN_ProtAreaEnd;
	tags->ti_data = (unsigned long)get_ptr_ro();
	tags++;
	
	tags->ti_tag = KRN_ARGC;
	tags->ti_data = (unsigned long)menu->argc;
	tags++;
	
	tags->ti_tag = KRN_ARGV;
	tags->ti_data = (unsigned long)&menu->argv[0];
	tags++;
	
	tags->ti_tag = 0;
	
	struct bss_tracker *bss = &tracker[0];
	while(bss->addr) {
		printf("[BOOT] Bss: %p-%p, %08x\n", 
		       bss->addr, (char*)bss->addr + bss->length - 1, 
		       bss->length);
		bss++;
	}
                            
	entry(&items[0]);
                            
	printf("[BOOT] Shouldn't be back...\n");
	while(1); 
}

int __startup bootstrap(context_t * ctx)
{
	boot_dev_t *boot;
	menu_t *menu, *entry;
	int i, selected;

	clear_bss();
	
	context_init(ctx);

	setenv("stdout", "serial");
	
	video_clear();
	video_draw_text(5, 4, 0, " Parthenope (ub2lb) version 0.01", 80);

	boot = get_booting_device();

	if (boot == NULL)
		goto exit;

	menu = menu_load(boot);
	if (menu == NULL) {
		setenv("stdout", "vga");
		printf("No menu.lst found!\n");
		goto exit;
	}

	entry = menu_display(menu);

	if(entry == NULL)
		goto exit;

	switch(entry->device_type) {
	case IDE_TYPE:
		boot = ext2_create(entry->device_num, 
				   entry->partition);
		break;
	case TFTP_TYPE:
		boot = tftp_create();
		break;
	case CD_TYPE:
		boot = cdrom_create();
		break;
	}

	if(boot == NULL)
		goto exit;

	void *kernel = malloc(3*1024*1024);
	if(boot->load_file(boot, entry->kernel, kernel) < 0)
		return 1;
	
	testboot_standalone(entry, kernel, boot);
	testboot_linux(entry, kernel, boot);
	testboot_aros(entry, kernel, boot);

	free(kernel);

	boot->destroy(boot);
  exit:
	setenv("stdout", "vga");
	printf("Press a key to open U-Boot prompt!\n");
	video_get_key();
	return 0;
}
