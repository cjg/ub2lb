/* tftp.c */

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

#include "tftp.h"

static int load_file(tftp_boot_dev_t * this, char *filename, void *buffer)
{
	return this->ctx->c_my_netloop(filename, buffer);
}

static int destroy(tftp_boot_dev_t * this)
{
	free_mem(this->ctx, this);
}

tftp_boot_dev_t *tftp_create(context_t * ctx)
{
	tftp_boot_dev_t *boot = alloc_mem(ctx, sizeof(boot_dev_t));

	if (boot == NULL)
		return NULL;

	boot->ctx = ctx;
	boot->load_file = (int (*)(void *, char *, void *))load_file;
	boot->destroy = (void (*)(void *))destroy;

	return boot;
}
