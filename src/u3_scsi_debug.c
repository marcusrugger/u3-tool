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
#include <sys/ioctl.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <stdio.h>

#include "sg_err.h"

#define U3_TIMEOUT 2000	//2000 millisecs == 2 seconds 

int u3_open(u3_handle_t *device, const char *which) 
{
	FILE *fp;

	u3_set_error(device, "");
	device->dev = NULL;

	if (strcmp(which, "stdout") == 0) {
		fp = stdout;
	} else if (strcmp(which, "stderr") == 0) {
		fp = stderr;
	} else {
		u3_set_error(device, "unknown output '%s', 'stdout' and "
			"'stderr' only supported", which);
		return U3_FAILURE;
	}
		
	device->dev = fp;
	return U3_SUCCESS;
}

void u3_close(u3_handle_t *device) 
{
	FILE *fp = (FILE *)(device->dev);
//	if (fp != stdout && fp != stderr) {
//		fclose(fp);
//	}
}

int u3_send_cmd(u3_handle_t *device, uint8_t cmd[U3_CMD_LEN],
		int dxfer_direction, int dxfer_length, uint8_t *dxfer_data,
		uint8_t *status)
{
	FILE *fp = (FILE *)device->dev;
	int i;

	fprintf(fp, "---------------------------------------------------------"
			"-----------------------\n");

	fprintf(fp, "Command block:\n");
	for (i=0; i < U3_CMD_LEN; i++) {
		fprintf(fp, "%.2X ", cmd[i]);
	}
	fprintf(fp, "\n");

	fprintf(fp, "\n");
	switch (dxfer_direction) {
		case U3_DATA_NONE:
			fprintf(fp, "No data\n");
			break;
		case U3_DATA_TO_DEV:
			fprintf(fp, "Sending %d bytes of data to device\n", dxfer_length);
			
			fprintf(fp, "Data:");
			for (i=0; i < dxfer_length; i++) {
				if (i % 8 == 0) fprintf(fp, "\n%.4x\t", i);
				fprintf(fp, "%.2x ", dxfer_data[i]);
			}
			fprintf(fp, "\n");
			break;
		case U3_DATA_FROM_DEV:
			fprintf(fp, "Reading %d bytes of data from device\n", dxfer_length);
			memset(dxfer_data, 0, dxfer_length);
			break;
	}

	fprintf(fp, "---------------------------------------------------------"
			"-----------------------\n");

	*status = 0;

	return U3_SUCCESS;
}


