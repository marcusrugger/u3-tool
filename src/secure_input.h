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
#ifndef __SECURE_INPUT_H__
#define __SECURE_INPUT_H__

#include <sys/types.h>

/**
 * Read string from standard input while hiding the input. The read
 * input is returned in 'buf' as a \0 terminated string. The maximum string
 * length will be 'buf_size' - 1.
 *
 * @param buf		target buffer
 * @param buf_size	Size of the target buffer
 */
void secure_input(char *buf, size_t buf_size);

#endif // __SECURE_INPUT_H__
