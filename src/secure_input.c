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
#ifdef WIN32
# include <windows.h>
#else
# include <termios.h>
# include <sys/ioctl.h>
#endif

void
secure_input(char *buf, size_t buf_size)
{
#ifdef WIN32
	DWORD mode;
	HANDLE ih;
#else
	struct termios ttymode;
#endif
	int pos;
	int ch;

	// input checking
	if (buf_size < 1) return;
	buf[0] = '\n';

	// hide input
#ifdef WIN32
	ih = GetStdHandle(STD_INPUT_HANDLE);
	if (!GetConsoleMode(ih, &mode)) {
		fprintf(stderr, "Failed to obtain handle to console\n");
		return;
	}
	SetConsoleMode(ih, mode & ~(ENABLE_ECHO_INPUT ));
#else
	tcgetattr(STDIN_FILENO, &ttymode);
	ttymode.c_lflag &= ~( ECHO | ECHOE | ECHONL );
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &ttymode);
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

	// unhide input
#ifdef WIN32
	SetConsoleMode(ih, mode);
	fputc('\n', stdout);
#else
	ttymode.c_lflag |= ECHO | ECHOE | ECHONL;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &ttymode);
	fputc('\n', stdout);
#endif
}
