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
#ifndef __U3_ERROR_H__
#define __U3_ERROR_H__

#include "u3.h"

/**
 * Get string error message of last error
 *
 * This function returns a pointer to the internal error buffer of the U3
 * device handle.
 *
 * @param device	U3 device handle
 *
 * @return		error string of last error
 */
char* u3_error_msg(u3_handle_t *device);

/* set a new u3 error */
void u3_set_error(u3_handle_t *device, const char *fmt, ...);

#endif // __U3_ERROR_H__
