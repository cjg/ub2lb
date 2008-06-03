/***************************************************************************
 *   Copyright (C) 2008 by Giuseppe Coviello   *
 *   gicoviello@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "context.h"
#include "device.h"
#include "tftp.h"
#include "ext2.h"
#include "menu.h"

extern unsigned long __bss_start;
extern unsigned long _end;

#define __startup __attribute__((section(".aros.startup")))

static void clear_bss()
{
	unsigned long *ptr = &__bss_start;
	while (ptr < &_end)
		*ptr++ = 0;
}

static boot_dev_t *get_booting_device(void)
{
	boot_dev_t *dev = NULL;
	SCAN_HANDLE hnd;
	uint32_t blocksize;

	for (hnd = start_unit_scan(get_scan_list(), &blocksize);
	     hnd != NULL; hnd = next_unit_scan(hnd, &blocksize)) {
		if (hnd->ush_device.type == DEV_TYPE_NETBOOT) {
			dev = tftp_create();
			video_draw_text(17, 4, 0, "TFTP", 80);
		}

		if (dev == NULL)
			continue;
	}

	end_unit_scan(hnd);
	end_global_scan();

	if (dev == NULL) {
		hnd = get_curr_device();
		if (hnd->ush_device.type == DEV_TYPE_NETBOOT) {
			dev = tftp_create();
			video_draw_text(17, 4, 0, "TFTP", 80);
		}
	}

	return dev;
}

int __startup bootstrap(context_t * ctx)
{
	boot_dev_t *boot;
	menu_t *menu, *entry;
	int i, selected;
	clear_bss();
	context_init(ctx);
/* /\* 	ctx->c_setenv("stdout", "serial"); *\/ */
	video_clear();
	video_draw_text(0, 3, 0, "I'm ub2lb (Parthenope)", 80);
	video_draw_text(0, 4, 0, "Booting from ...", 80);

	boot = get_booting_device();

	if (boot == NULL)
		return 0;
	menu = menu_load(boot);
	selected = menu_display(menu);
/* /\* 	for(i = 0, entry = menu; i < selected; entry = entry->next, i++); *\/ */
/* /\* 	/\\* code to boot linux *\\/ *\/ */
/* /\* 	ctx->c_setenv("stdout", "vga"); *\/ */
/* 	{ */
/* /\* 		char bootargs[100]; *\/ */
/* 		char outbuff[200]; */
/* 		char *args[3]; */
/* 		void *uImage = malloc(3*1024*1024); */
/* 		void *uRamdisk = malloc(4*1024*1024); */
/* 		boot = ext2_create(0, 6); */
/* /\* 		return; *\/ */
/* 		boot->load_file(boot, "uRamdisk", uRamdisk); */
/* 		boot->load_file(boot, "uImage", uImage); */
/* /\* 		boot->load_file(boot, "uRamdisk", uRamdisk); *\/ */
/* /\* 		ctx->c_sprintf(outbuff, "%lX", filelen); *\/ */
/* /\* 		ctx->c_setenv("filesize", outbuff); *\/ */
/* 		printf("%p\n", uImage); */
/* /\*  		ctx->c_do_bootm(NULL, 0, 0, NULL); *\/ */
/* 		args[0] = "bootm"; */
/* 		args[1] = malloc(32); */
/* 		args[2] = malloc(32); */
/* 		sprintf(args[1], "%p", uImage); */
/* 		sprintf(args[2], "%p", uRamdisk); */
/* /\* 		ctx->c_video_clear(); *\/ */
/* /\* 		ctx->c_video_draw_text(7, 7, 0, "Booting linux ...", 66); *\/ */
/* /\* 		ctx->c_sprintf(bootargs, "%s quiet", ctx->c_getenv("bootargs")); *\/ */
/* /\* 		ctx->c_sprintf(bootargs, "%s", entry->append); *\/ */
/* /\* 		ctx->c_sprintf(outbuff, "Setting bootargs %s", bootargs); *\/ */
/* /\* 		ctx->c_video_draw_text(7, 9, 0, outbuff, 66); *\/ */
/* /\* 		ctx->c_setenv("bootargs", bootargs); *\/ */
/* /\* 		ctx->c_sprintf(outbuff, "Loading %s at %s ...", entry->kernel, args[0]); *\/ */
/* /\* 		ctx->c_video_draw_text(7, 10, 0, outbuff, 66); *\/ */
/* /\* 		boot->load_file(boot, entry->kernel, uImage); *\/ */
/* /\* 		if(entry->initrd != NULL) { *\/ */
/* /\* 			ctx->c_sprintf(outbuff, "Loading %s at %s ...", *\/ */
/* /\* 				       entry->initrd, args[1]); *\/ */
/* /\* 			ctx->c_video_draw_text(7, 11, 0, outbuff, 66); *\/ */
/* /\* 			boot->load_file(boot, entry->initrd, uRamdisk); *\/ */
/* /\* 		} *\/ */
/* /\* 		ctx->c_video_draw_text(7, 13, 0, "Boot!", 66); *\/ */
/* 		bootm(NULL, 0, 3, args); */
/* 		return; */
/* 	} */

/*   exit1: */
/* 		boot->destroy(boot); */

	return 0;
}
