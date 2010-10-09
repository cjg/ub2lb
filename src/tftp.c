/*
 * ub2lb -- UBoot second level bootloader.
 *
 * Copyright (C) 2006 - 2010  Giuseppe Coviello <cjg@cruxppc.org>.
 *
 * This file is part of ub2lb.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * Written by: Giuseppe Coviello <cjg@cruxppc.org>
 */

#include "tftp.h"

static int load_file(boot_dev_t * this, char *filename, void *buffer) {
    /* FIXME: find a better way to avoid unused warning */
    this = this;

    return netloop(filename, buffer);
}

static int destroy(boot_dev_t * this) {
    free(this);
    return 0;
}

boot_dev_t *tftp_create(void) {
    boot_dev_t *boot = malloc(sizeof (boot_dev_t));

    if (boot == NULL)
        return NULL;

    boot->load_file = (int (*)(void *, char *, void *))load_file;
    boot->destroy = (void (*)(void *))destroy;

    return boot;
}
