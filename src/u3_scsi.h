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
#ifndef __U3_SCSI_H__
#define __U3_SCSI_H__
/**
 * @file	u3_scsi.h
 *
 * @description	Declare some helper functions to provide a subsystem
 * 		independent way of sending SCSI commands to a U3 device.
 */

#include "u3.h"

#define U3_CMD_LEN		12

/**
 * Open U3 device
 *
 * This opens the device addressed by the variable 'which'.
 *
 * @param device	pointer to U3 handle that will be used to access the
 * 			newly openned device.
 *
 * @returns		U3_SUCCESS if successful, else U3_FAILURE and
 * 			an error string can be obtained using u3_error()
 */
int u3_open(u3_handle_t *device, const char *which);

/**
 * Close U3 device
 *
 * This closes the u3 device handle pointed to by 'device'
 *
 * @param device	U3 handle
 */
void u3_close(u3_handle_t *device);


/**
 * dxfer_direction values as used by u3_send_cmd()
 *
 * @see 'u3_send_cmd()'
 */
enum {
	U3_DATA_NONE = 0,		// dont transfer extra data
	U3_DATA_TO_DEV = 1,		// send data to device
	U3_DATA_FROM_DEV = 2,	// read data from device
};

/**
 * Execute a scsi command at device
 *
 * This send the given SCSI CDB in 'cmd' to the device given by 'device'.
 * Optional data is transfered from or to the device. The SCSI status as
 * returned by the device is placed in 'status'.
 *
 * @param device		U3 handle
 * @param cmd			SCSI CDB
 * @param dxfer_direction	Direction of extra data, given by on of the
 * 				U3_DATA_* enum values
 * @param dxfer_len		Length of extra data
 * @param dxfer_data		Buffer with extra data
 * @param status		Pointer to variable to return SCSI status in
 *
 * @returns		U3_SUCCESS if successful, else U3_FAILURE and
 * 			an error string can be obtained using u3_error()
 */
int u3_send_cmd(u3_handle_t *device, uint8_t cmd[U3_CMD_LEN],
		int dxfer_direction, int dxfer_length, uint8_t *dxfer_data,
		uint8_t *status);

#endif // __U3_SCSI_H__
