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
#if HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef SUBSYS_SG
#include "u3_scsi.h"
#include "u3_error.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <linux/cdrom.h>
#include <scsi/sg.h>

#include "sg_err.h"

#define SG_TIMEOUT 2000	//2000 millisecs == 2 seconds 

const char *u3_subsystem_name = "sg";
const char *u3_subsystem_help = "'/dev/sda0', '/dev/sg3'";

int u3_open(u3_handle_t *device, const char *which) 
{
	int k;
	int sg_fd;

	u3_set_error(device, "");
	device->dev = NULL;

	if ((sg_fd = open(which, O_RDWR)) < 0) {
		// Nautilus sends a eject ioctl to the device when unmounting.
		// Just have to close the tray ;-)
		if (errno == ENOMEDIUM) {
			if ((sg_fd = open(which, O_RDONLY | O_NONBLOCK)) < 0) {
				u3_set_error(device, "%s", strerror(errno));
				return U3_FAILURE;
			}
			ioctl(sg_fd, CDROMCLOSETRAY);
			close(sg_fd);
			if ((sg_fd = open(which, O_RDWR)) < 0) {
				u3_set_error(device, "%s", strerror(errno));
				return U3_FAILURE;
			}
		} else {
			u3_set_error(device, "%s", strerror(errno));
			return U3_FAILURE;
		}
	}

	// It is prudent to check we have a sg device by trying an ioctl
	if (ioctl(sg_fd, SG_GET_VERSION_NUM, &k) < 0) {
		close(sg_fd);
		if (k < 30000) {
			u3_set_error(device, "device is not an SG device");
		} else {
			u3_set_error(device, "SG driver to old");
		}
		return U3_FAILURE;
	}


	if ((device->dev = malloc(sizeof(int))) == NULL) {
		close(sg_fd);
		u3_set_error(device, "Failed allocating memory for file descriptor");
		return U3_FAILURE;
	}
		
	*((int *)device->dev) = sg_fd;
	return U3_SUCCESS;
}

void u3_close(u3_handle_t *device) 
{
	int *sg_fd = (int *)device->dev;
	close(*sg_fd);
	free(sg_fd);
}

int u3_send_cmd(u3_handle_t *device, uint8_t cmd[U3_CMD_LEN],
		int dxfer_direction, int dxfer_length, uint8_t *dxfer_data,
		uint8_t *status)
{
	sg_io_hdr_t io_hdr;
	unsigned char sense_buf[32];
	int *sg_fd = (int *)device->dev;

	// translate dxfer_direction
	switch (dxfer_direction) {
		case U3_DATA_NONE:
			dxfer_direction = SG_DXFER_NONE;
			break;
		case U3_DATA_TO_DEV:
			dxfer_direction = SG_DXFER_TO_DEV;
			break;
		case U3_DATA_FROM_DEV:
			dxfer_direction = SG_DXFER_FROM_DEV;
			break;
	}

	// Prepare command
	memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
	io_hdr.interface_id = 'S';			// fixed
	io_hdr.dxfer_direction = dxfer_direction;	// Select data direction
	io_hdr.cmd_len = U3_CMD_LEN;			// length of command in bytes
	io_hdr.mx_sb_len = 0;		// sense buffer size. do we use this???
	io_hdr.iovec_count = 0;   			// don't use iovector stuff
	io_hdr.dxfer_len = dxfer_length;		// Size of data transfered
	io_hdr.dxferp = dxfer_data;			// Data buffer to transfer
	io_hdr.cmdp = cmd;				// Command buffer to execute
	io_hdr.sbp = sense_buf;				// Sense buffer
	io_hdr.timeout = SG_TIMEOUT;			// timeout
	io_hdr.flags = 0;	 			// take defaults: indirect IO, etc 
	// io_hdr.pack_id = 0;				// internal packet. used by te program to recognize packets
	// io_hdr.usr_ptr = NULL;			// user data

	// preform ioctl on device
	if (ioctl(*sg_fd, SG_IO, &io_hdr) < 0) {
		u3_set_error(device, "Failed executing scsi command: "
				"SG_IO ioctl failed with %s", strerror(errno));
		return U3_FAILURE;
	}

	// evaluate result
	if ((io_hdr.info & SG_INFO_OK_MASK) != SG_INFO_OK) {
		if (io_hdr.host_status == SG_ERR_DID_OK &&
		    (io_hdr.driver_status & SG_ERR_DRIVER_SENSE))
		{
			// 
			// The usb-storage driver automatically request sense
			// data if a command fails. So this state isn't really
			// a error but only indicates that the sense data in
			// the buffer is fresh. Currently we aren't don't use
			// this sense data, we only wanna know if a command
			// failed or not. so ignore this...
			//

		} else {
			u3_set_error(device, "Failed executing scsi command: "
				"Status (S:0x%x,H:0x%x,D:0x%x)", io_hdr.status,
				io_hdr.host_status, io_hdr.driver_status);
			return U3_FAILURE;
		}
	}

	*status = io_hdr.masked_status;

	return U3_SUCCESS;
}

#endif //SUBSYS_SG
