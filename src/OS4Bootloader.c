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

extern unsigned long __bss_start;
extern unsigned long _end;

#define __startup __attribute__((section(".aros.startup")))

static void clear_bss()
{
    unsigned long *ptr = &__bss_start;
    while(ptr < &_end)
            *ptr++ = 0;
}

int __startup bootstrap(context_t *ctx)
{
	clear_bss();
}