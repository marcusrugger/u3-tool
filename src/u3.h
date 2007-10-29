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
#ifndef __U3_H__
#define __U3_H__

#include <stdint.h>

#define U3_MAX_ERROR_LEN	100	// Max. length of error msg.
#define U3_SECTOR_SIZE		512	// size of one sector in the u3 system
#define U3_BLOCK_SIZE		2048	// size of one block in the u3 system

extern int debug;

/**
 * Handle for a U3 device
 */
struct u3_handle {
	void *dev;		/* Raw SCSI interface handle, This is a void *
				 * to be independent of the raw SCSI interface
				 * used(eg. sg, usb, etc.) */
	char err_msg[U3_MAX_ERROR_LEN];
};
typedef struct u3_handle u3_handle_t;

/**
 * Return values of U3 functions.
 */
enum {
	U3_SUCCESS = 0,
	U3_FAILURE = -1
};
#endif // __U3_H__
