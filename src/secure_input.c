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
#include "secure_input.h"
#include <stdio.h>
#include <unistd.h>
#ifndef WIN32
# include <termios.h>
# include <sys/ioctl.h>
#endif

void
secure_input(char *buf, size_t buf_size)
{
#ifndef WIN32
	struct termio ttymode;
#endif
	int pos;
	int ch;

#ifndef WIN32
	// hide input
	ioctl(STDIN_FILENO, TCGETA, &ttymode);
	ttymode.c_lflag &= ~( ECHO | ECHOE | ECHONL );
	ioctl(STDIN_FILENO, TCSETAF, &ttymode);
#endif

	pos=0;
	ch=fgetc(stdin);
	while (pos < buf_size-1 && ch != '\n' && ch != EOF) {
		buf[pos] = ch;
		pos++;
		ch=fgetc(stdin);
	} 
	buf[pos] = '\0';

	// flush the stdin buffer of remaining unused characters
	while (ch != '\n' && ch != EOF)
		ch = fgetc(stdin);

#ifndef WIN32
	// unhide input
	ttymode.c_lflag |= ECHO | ECHOE | ECHONL;
	ioctl(STDIN_FILENO, TCSETAF, &ttymode);
	fputc('\n', stdout);
#endif
}
