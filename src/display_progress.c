/**
 * u3-tool - U3 USB stick manager
 * Copyright (C) 2007 Daviedev, daviedev@users.sourceforge.net
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */ 
#include "display_progress.h"
#include <stdio.h>

void display_progress(unsigned int cur, unsigned int total) {
        unsigned int procent;
	unsigned int bar_len, i;

	if (total == 0) return;

	procent = (cur * 100) / total;

	putchar('|');
	bar_len = (cur * PROGRESS_BAR_WIDTH) / total;
	for (i =0; i < bar_len; i++) {
		putchar('*');
	}
	for (i = bar_len; i < PROGRESS_BAR_WIDTH; i++) {
		putchar(' ');
	}
	putchar('|');

	printf(" %d%%\r", procent);
	fflush(stdout);
}
