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

#ifndef _DEVICE_H
#define _DEVICE_H

typedef struct boot_dev {
    int (*load_file) (void *this, char *filename, void *buffer);
    void (*destroy) (void *this);
} boot_dev_t;

#endif /* _DEVICE_H */
