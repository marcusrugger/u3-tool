/**
 * u3-tool - U3 USB stick manager
 * Copyright (C) 2011 Daviedev, daviedev@users.sourceforge.net
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

#ifdef SUBSYS_LIBUSB
#include "u3_scsi.h"
#include "u3_error.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <usb.h>
#include <errno.h>
#include <regex.h>

// USB maximum packet length
#define USB_MAX_PACKET_LEN 2048

// USB command block wrapper length
#define U3_CBWCB_LEN	12

// USB MSC command block wrapper data direction bits
#define CBW_DATA_IN		0x80
#define CBW_DATA_OUT		0x00
#define CBW_DATA_NONE		0x00
#define CBW_FLAG_DIRECTION	0x80

#define U3_DEVICE_TIMEOUT 2000	//2000 millisecs == 2 seconds 

uint16_t u3_dev_list[][2] = {
	{ 0x08ec, 0x0020 }, // Verbatim Store 'N Go
	{ 0x0781, 0x5406 }, // Sandisk Cruzer Micro
	{ 0x0781, 0x5408 }, // Sandisk Cruzer Titanium
	{ 0x0781, 0x550a }, // Sandisk Cruzer Pattern
	{ 0x0781, 0x5151 }, // Sandisk Cruzer Micro Skin 8GB (Thanks to dnelson @sf)
	{ 0x0781, 0x540e }, // Sandisk Cruzer Contour
	{ 0x0781, 0x5530 }, // Sandisk Cruzer
	{ 0x0781, 0x5535 }, // Sandisk Ultra Backup
	{ 0, 0 },
};

struct u3_usb_handle {
	usb_dev_handle *handle;
	int interface_num;
	int ep_out;
	int ep_in;
};
typedef struct u3_usb_handle u3_usb_handle_t;

struct usb_msc_cbw {
	uint8_t dCBWSignature[4];
	uint32_t dCBWTag;
	uint32_t dCBWDataTransferLength;
	uint8_t bmCBWFlags;
	uint8_t bCBWLUN;
	uint8_t bCBWCBLength;
	uint8_t CBWCB[16];
} __attribute__ ((packed));

struct usb_msc_csw {
	uint8_t dCSWSignature[4];
	uint32_t dCSWTag;
	uint32_t dCSWDataResidue;
	uint8_t bCSWStatus;
} __attribute__ ((packed));

const char *u3_subsystem_name = "libusb";
const char *u3_subsystem_help = "'scan' to automatically use the first detected U3 device, or 'vid:pid' if not detected";

static struct usb_device *
locate_u3_device(uint16_t vid, uint16_t pid)
{
	struct usb_bus *bus;
	struct usb_device *dev;

	// rescan busses and devices
	usb_find_busses();
	usb_find_devices();

	for (bus = usb_get_busses(); bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			if (dev->descriptor.idVendor == vid &&
			    (dev->descriptor.idProduct == pid))
			{
				return dev;
			}
		}
	}

	return NULL;
}

int u3_open(u3_handle_t *device, const char *which) 
{
	uint16_t vid, pid;
	regex_t vid_pid_regex;
	int has_vid_pid = 0;

        struct usb_device *u3_device;
	u3_usb_handle_t *handle_wrapper;

	struct usb_config_descriptor *config_desc;
	struct usb_interface_descriptor *interface_desc;
	struct usb_endpoint_descriptor *endpoint_desc;

	int err;
	char errbuf[U3_MAX_ERROR_LEN];

	// init
	u3_set_error(device, "");

	err = regcomp(&vid_pid_regex, "[::xdigit::]*:[::xdigit::]*", 0);
	if (err != 0) {
		regerror(err, &vid_pid_regex, errbuf, U3_MAX_ERROR_LEN);
		u3_set_error(device, "Failed constructing 'vid:pid' regular "
				"expression: %s", errbuf);
		return U3_FAILURE;
	}
	if (regexec(&vid_pid_regex, which, 0, 0, 0) == 0) {
		// Do de regexing here, this prevents leaking of the
		// vid_pid_regex memory
		char *seperator=0;
		has_vid_pid = 1;
		vid = strtoul(which, &seperator, 16);
		pid = strtoul(seperator+1, &seperator, 16);
	}
	regfree(&vid_pid_regex);

	usb_init();
	if (debug)
		usb_set_debug(255);

	// Find device
	if (has_vid_pid) {
		u3_device = locate_u3_device(vid, pid);

		if (u3_device == NULL) {
			u3_set_error(device, "Could not locate the U3 device '%s', "
					"try 'scan' for first available device", which);
			return U3_FAILURE;
		}
	} else if (strcmp(which, "scan") == 0) {
		int i = 0;
		u3_device = NULL;
		while (u3_device == NULL &&
		       !(u3_dev_list[i][0] == 0 && u3_dev_list[i][1] == 0))
		{
			u3_device = locate_u3_device(u3_dev_list[i][0],
						u3_dev_list[i][1]);
			i++;
		}

		if (u3_device == NULL) {
			u3_set_error(device, "Could not locate any known U3 device, "
					"the VID:PID of your device might not be known to U3 tool");
			return U3_FAILURE;
		}
//	} else if (regexec(&serial_regex, which, 0, 0, 0) == 0) {
//TODO: search for a specific serial number
	} else {
		u3_set_error(device, "Unknown device name '%s', try 'scan' "
			"for first available device", which);
		return U3_FAILURE;
	}

	// Open device
	handle_wrapper = (u3_usb_handle_t *) malloc(sizeof(u3_usb_handle_t));
	if (handle_wrapper == NULL) {
		u3_set_error(device, "Failed allocate memory!!");
		return U3_FAILURE;
	}

	handle_wrapper->handle = usb_open(u3_device);

	// Set configuration
	if (u3_device->descriptor.bNumConfigurations != 1) {
		u3_set_error(device,
			"Multiple USB configuration not supported");
		goto open_fail;
	} 
	config_desc = u3_device->config;
	usb_set_configuration(handle_wrapper->handle,
				config_desc->bConfigurationValue);

	// Claim device
	if (config_desc->bNumInterfaces != 1) {
		u3_set_error(device, "Multiple USB configuration interfaces "
			"not supported");
		goto open_fail;
	}
	if (config_desc->interface->num_altsetting != 1) {
		u3_set_error(device, "Multiple USB configuration interface "
			" alt. settings not supported");
		goto open_fail;
	}
	interface_desc = config_desc->interface->altsetting;
	handle_wrapper->interface_num = interface_desc->bInterfaceNumber;
	err = usb_detach_kernel_driver_np(handle_wrapper->handle, handle_wrapper->interface_num);
	if (err != 0 && err != -61) {
		u3_set_error(device, "failed to detach USB device with %s", strerror(errno));
		goto open_fail;
	}

	if ((err=usb_claim_interface(handle_wrapper->handle,
				handle_wrapper->interface_num)) != 0)
	{
		if (err == -EBUSY) {
			// Don't try to detach driver on Linux, this can cause
			// data loss and leaves the device in a unspecified
			// state. The user should 'unbind' the driver through
			// sysfs, or unload the usb-storage driver.
			u3_set_error(device, "Failed to claim device: Device is busy");
			goto open_fail;
		} else {
			u3_set_error(device, "Failed to claim device: %s", usb_strerror());
			goto open_fail;
		}
	}

	// Set alt. setting
	usb_set_altinterface(handle_wrapper->handle,
			interface_desc->bAlternateSetting);

	// Find the correct endpoints
	if (interface_desc->bNumEndpoints != 2) {
		u3_set_error(device, "Not exactly two usb endpoints defined,"
			" unsupported.");
		goto claimed_fail;
	}
	endpoint_desc = interface_desc->endpoint;

	if (endpoint_desc[0].bEndpointAddress & USB_ENDPOINT_DIR_MASK) {
		handle_wrapper->ep_in  = endpoint_desc[0].bEndpointAddress;
		handle_wrapper->ep_out = endpoint_desc[1].bEndpointAddress;
	} else {
		handle_wrapper->ep_out = endpoint_desc[0].bEndpointAddress;
		handle_wrapper->ep_in  = endpoint_desc[1].bEndpointAddress;
	}

	if (!((handle_wrapper->ep_in & USB_ENDPOINT_DIR_MASK) ^
	     (handle_wrapper->ep_out & USB_ENDPOINT_DIR_MASK)))
	{
		u3_set_error(device, "Both usb endpoints in same direction!");
		goto claimed_fail;
	}

	device->dev = handle_wrapper;
	return U3_SUCCESS;
claimed_fail:
        usb_release_interface(handle_wrapper->handle,
				handle_wrapper->interface_num);
open_fail:
	usb_close(handle_wrapper->handle);
	free(handle_wrapper);
	return U3_FAILURE;

}

void u3_close(u3_handle_t *device) 
{
	u3_usb_handle_t *handle_wrapper = (u3_usb_handle_t *) device->dev;

        usb_release_interface(handle_wrapper->handle,
				handle_wrapper->interface_num);
        usb_close(handle_wrapper->handle);
	free(handle_wrapper);
}

int u3_send_cmd(u3_handle_t *device, uint8_t cmd[U3_CMD_LEN],
		int dxfer_direction, int dxfer_length, uint8_t *dxfer_data,
		uint8_t *status)
{
	unsigned int tag = 1;
	int res;
	u3_usb_handle_t *handle_wrapper = (u3_usb_handle_t *) device->dev;
        struct usb_msc_cbw cbw;
        struct usb_msc_csw csw;

	// translate dxfer_direction
	switch (dxfer_direction) {
		case U3_DATA_TO_DEV:
			dxfer_direction = CBW_DATA_OUT;
			break;
		case U3_DATA_FROM_DEV:
			dxfer_direction = CBW_DATA_IN;
			break;
		case U3_DATA_NONE:
		default:
			dxfer_direction = CBW_DATA_NONE;
			dxfer_length = 0;
			break;
	}

	// Prepare command
	memset(&cbw, 0, sizeof(cbw));
	cbw.dCBWSignature[0] = 'U';
	cbw.dCBWSignature[1] = 'S';
	cbw.dCBWSignature[2] = 'B';
	cbw.dCBWSignature[3] = 'C';
	cbw.dCBWDataTransferLength = dxfer_length;
	cbw.bmCBWFlags = dxfer_direction & CBW_FLAG_DIRECTION;
	cbw.dCBWTag = tag;
	cbw.bCBWLUN = 1;
	cbw.bCBWCBLength = U3_CBWCB_LEN;
	memcpy(&(cbw.CBWCB), cmd, U3_CBWCB_LEN);

	// send command
        if ((res = usb_bulk_write(handle_wrapper->handle, handle_wrapper->ep_out,
			(char *) &cbw, sizeof(cbw), U3_DEVICE_TIMEOUT))
                        != sizeof(cbw))
        {
		u3_set_error(device, "Failed executing scsi command: "
			"Could not write command block to device");
		return U3_FAILURE;
        }

	// read/write extra command data
	if (dxfer_direction == CBW_DATA_OUT) {
		int bytes_send = 0;
		while (bytes_send < dxfer_length) {
			int plen;

			plen = dxfer_length - bytes_send;
			if (plen > USB_MAX_PACKET_LEN)
				plen = USB_MAX_PACKET_LEN;

			if ((res = usb_bulk_write(handle_wrapper->handle,
					handle_wrapper->ep_out,
					(char *) dxfer_data+bytes_send, plen,
					U3_DEVICE_TIMEOUT)) < 0)
			{
				u3_set_error(device, "Failed executing scsi command: "
					"Could not write data to device");
				return U3_FAILURE;
			}

			bytes_send += plen;
		}
	} else if (dxfer_direction == CBW_DATA_IN) {
		if ((res = usb_bulk_read(handle_wrapper->handle, handle_wrapper->ep_in,
				(char *) dxfer_data, dxfer_length,
				U3_DEVICE_TIMEOUT)) < 0)
		{
			u3_set_error(device, "Failed executing scsi command: "
				"Could not read data from device");
			return U3_FAILURE;
		}
	}

	memset(&csw, 0, sizeof(csw));

	// read command status
        if ((res = usb_bulk_read(handle_wrapper->handle, handle_wrapper->ep_in,
			(char *) &csw, sizeof(csw), U3_DEVICE_TIMEOUT))
                        < 0)
        {
		u3_set_error(device, "Failed executing scsi command: "
			"Could not read command status from device");
		return U3_FAILURE;
        }

	assert(tag == csw.dCSWTag);
	tag++;
        *status = csw.bCSWStatus;

	return U3_SUCCESS;
}

#endif //SUBSYS_LIBUSB
