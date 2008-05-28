/* menu.h */

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

#ifndef _MENU_H
#define _MENU_H

#include "context.h"
#include "device.h"

#define MENU_FILE "menu.lst"

typedef struct menu {
	char *title;
	char *kernel;
	char *append;
	char *initrd;
	struct menu *next;
} menu_t;

menu_t *menu_load(context_t * ctx, boot_dev_t * boot);
int menu_display(context_t * ctx, menu_t * self);
void menu_free(context_t * ctx, menu_t * self);

#endif /* _MENU_H */
