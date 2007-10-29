/**
 * u3_tool - U3 USB stick manager
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
#include "u3_error.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

char *u3_error_msg(u3_handle_t *device) {
	return device->err_msg;
}

void u3_set_error(u3_handle_t *device, const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(device->err_msg, U3_MAX_ERROR_LEN, fmt, ap);
	va_end(ap);
}
