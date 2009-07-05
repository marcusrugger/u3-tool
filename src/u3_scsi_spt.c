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
#include "u3_scsi.h"
#include "u3_error.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#include <windows.h>
#include <ddk/ntddscsi.h>

#define U3_TIMEOUT 2	// 2 seconds 

int u3_open(u3_handle_t *device, const char *which) 
{
	HANDLE hDevice;
    CHAR lpszDeviceName[7];
    DWORD dwBytesReturned;
    DWORD dwError;

    u3_set_error(device, "");
    device->dev = NULL;

    // check parameter
    if (strlen(which) != 1 || ! isalpha(which[0])) {
        u3_set_error(device, "Unknown drive name '%s', Expecting a "
            "drive letter", which);
        return U3_FAILURE;
    }

    //take the drive letter and put it in the format used in CreateFile
    memcpy(lpszDeviceName, (char *) "\\\\.\\*:", 7);
    lpszDeviceName[4] = which[0];

    //get a handle to the device, the parameters used here must be used in order for this to work
    hDevice=CreateFile(lpszDeviceName,
						GENERIC_READ|GENERIC_WRITE,
						FILE_SHARE_READ|FILE_SHARE_WRITE,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);

    //if for some reason we couldn't get a handle to the device we will try again using slightly different parameters for CreateFile
    if (hDevice==INVALID_HANDLE_VALUE)
    {
		u3_set_error(device, "Failed openning handle for %s: Error %d\n", which, GetLastError());
		return U3_FAILURE;
	}

	
	device->dev = hDevice;
	return U3_SUCCESS;
}

void u3_close(u3_handle_t *device) 
{
	HANDLE hDevice = (HANDLE)device->dev;
	CloseHandle(hDevice);
}

int u3_send_cmd(u3_handle_t *device, uint8_t cmd[U3_CMD_LEN],
		int dxfer_direction, int dxfer_length, uint8_t *dxfer_data,
		uint8_t *status)
{
	HANDLE hDevice = (HANDLE)device->dev;
	SCSI_PASS_THROUGH_DIRECT sptd;
	DWORD returned;
	BOOL err;

	// translate dxfer_direction
	switch (dxfer_direction) {
		case U3_DATA_NONE:
			dxfer_direction = SCSI_IOCTL_DATA_UNSPECIFIED;
			break;
		case U3_DATA_TO_DEV:
			dxfer_direction = SCSI_IOCTL_DATA_OUT;
			break;
		case U3_DATA_FROM_DEV:
			dxfer_direction = SCSI_IOCTL_DATA_IN;
			break;
	}

	// Prepare command
    memset(&sptd, 0, sizeof(SCSI_PASS_THROUGH_DIRECT));
    sptd.Length             = sizeof(SCSI_PASS_THROUGH_DIRECT);// fixed
    sptd.CdbLength          = U3_CMD_LEN;						// length of command in bytes
    sptd.SenseInfoLength    = 0;								// don't use this currently...
    sptd.DataIn             = dxfer_direction;					// data direction
    sptd.DataTransferLength = dxfer_length;					// Size of data transfered
    sptd.TimeOutValue       = U3_TIMEOUT;						// timeout in seconds
    sptd.DataBuffer         = dxfer_data;						// data buffer
    //sptd.SenseInfoOffset    = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, sbuf);
    memcpy(sptd.Cdb, cmd, U3_CMD_LEN);

	// preform ioctl on device
    err = DeviceIoControl(hDevice, IOCTL_SCSI_PASS_THROUGH_DIRECT, &sptd,
						sizeof(sptd), &sptd, sizeof(sptd), &returned, NULL);

	// evaluate result
	if (!err) {
		u3_set_error(device, "Failed executing scsi command: "
			"Error %d", GetLastError());
		return U3_FAILURE;
	}

	*status = sptd.ScsiStatus;

	return U3_SUCCESS;
}


