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

static boot_dev_t *get_booting_device(context_t * ctx)
{
	boot_dev_t *dev = NULL;
	SCAN_HANDLE hnd;
	uint32_t blocksize;

	for (hnd = ctx->c_start_unit_scan(ctx->c_scan_list, &blocksize);
	     hnd != NULL; hnd = ctx->c_next_unit_scan(hnd, &blocksize)) {
		if (hnd->ush_device.type == DEV_TYPE_NETBOOT) {
			dev = tftp_create(ctx);
			ctx->c_video_draw_text(17, 4, 0, "TFTP", 80);
		}

		if (dev == NULL)
			continue;
	}

	ctx->c_end_unit_scan(hnd);
	ctx->c_end_global_scan();

	if (dev == NULL) {
		hnd = ctx->c_curr_device;
		if (hnd->ush_device.type == DEV_TYPE_NETBOOT) {
			dev = tftp_create(ctx);
			ctx->c_video_draw_text(17, 4, 0, "TFTP", 80);
		}
	}

	return dev;
}

int __startup bootstrap(context_t * ctx)
{
	boot_dev_t *boot;
	menu_t *menu;
	clear_bss();
	ctx->c_setenv("stdout", "serial");
	ctx->c_video_clear();
	ctx->c_video_draw_text(0, 3, 0, "I'm ub2lb (Parthenope)", 80);
	ctx->c_video_draw_text(0, 4, 0, "Booting from ...", 80);
	boot = get_booting_device(ctx);

	if (boot != NULL) {
		menu = menu_load(ctx, boot);
		if (menu != NULL) {
			menu_display(ctx, menu);
			menu_free(ctx, menu);
		} else
			ctx->c_video_draw_text(0, 5, 0, "We haven't a menu",
					       80);

		boot->destroy(boot);
	}

	return 0;
}
