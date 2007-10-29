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
#ifndef __U3_COMMAND__
#define __U3_COMMAND__
/**
 * @file	u3_commands.h
 *
 *		This file declares the functions for controlling the
 *		U3 device.
 */

#include "u3.h"

/********************************* structures *********************************/

/**
 * Device property header structure
 */
struct property_header {
	uint16_t id;		/* Property ID */
	uint16_t unknown1;
	uint16_t length;	/* Length of property including header */
} __attribute__ ((packed));

/**
 * Device property 0x03 structure
 */
#define U3_MAX_SERIAL_LEN	16
struct property_03 {
	struct property_header hdr;	/* property header */
	uint32_t full_length;		/* Actual Full length of property???? */
	uint8_t  unknown1;
	uint32_t unknown2;
	char	 serial[U3_MAX_SERIAL_LEN];	/* Device Serial number */
	uint32_t unknown3;
	uint32_t device_size;		/* Device size in sectors */
} __attribute__ ((packed));

/**
 * Device property 0x0C structure
 */
struct property_0C {
	struct property_header hdr;	/* property header */
	uint32_t max_pass_try;		/* Maximum wrong password try */
} __attribute__ ((packed));

/**
 * partition information structure
 */
struct part_info {
	uint8_t  partition_count;
	uint32_t unknown1;
	uint32_t data_size;		// in sectors
	uint32_t unknown2;
	uint32_t cd_size;		// in sectors
} __attribute__ ((packed));

/**
 * Data partition information structure
 */
struct dpart_info {
	uint32_t total_size;	// size of data partition in sectors
	uint32_t secured_size;	// size of secured zone in sectors
	uint32_t unlocked;	// 1 = unlocked, 0 = locked
	uint32_t pass_try;	// number of times incorrect password tries
} __attribute__ ((packed));

/**
 * Chip information structure
 */
#define U3_MAX_CHIP_REVISION_LEN	8
#define U3_MAX_CHIP_MANUFACTURER_LEN	16
struct chip_info {
	char revision[U3_MAX_CHIP_REVISION_LEN];
	char manufacturer[U3_MAX_CHIP_MANUFACTURER_LEN];
} __attribute__ ((packed));


/********************************** functions *********************************/
/**
 * Read device property page
 *
 * This reads a page of device properties from the device. A requested page is
 * stored, including there page header, in 'buffer'. If the buffer size, as
 * specified by 'buffer_length', is to small to store the whole page, only the
 * first 'buffer_length' bytes are stored. The caller should check if the full
 * page fitted into the buffer, by comparing the page length in the header with
 * the buffer length. If the function returns successfull then atleast 
 * sizeof(struct property_header) bytes will be valid in the buffer, therefore
 * 'buffer_length should be atleast 6 byte. If a property page does not excist,
 * this function will fail.
 *
 * @param device	U3 device handle
 * @param property_id	The index of the requested property page
 * @param buffer	The buffer to return the requested property page in.
 * @param buffer_length	The allocated length of 'buffer' in bytes.
 *
 * @returns		U3_SUCCESS if successful, else U3_FAILURE and
 * 			an error string can be obtained using u3_error()
 *
 * @see 'struct property_header', 'struct property_*'
 */
int u3_read_device_property(u3_handle_t *device, uint16_t property_id,
		uint8_t *buffer, uint16_t buffer_length);

/**
 * Repartiton device
 *
 * This repatitions the device into a CD and a data partition. All current data
 * will be lost. Creating a CD partition of size 0 will make this device an
 * ordenary USB stick.
 *
 * @param device	U3 device handle
 * @param cd_size	The size of the CD partition in 512 byte sectors.
 *
 * @returns		U3_SUCCESS if successful, else U3_FAILURE and
 * 			an error string can be obtained using u3_error()
 */
int u3_partition(u3_handle_t *device, uint32_t cd_size);

/**
 * Write CD block
 *
 * This function write's a block to the CD partition. Blocks are 2048 bytes in
 * size.
 *
 * @param device	U3 device handle
 * @param block_num	The block number to write
 * @param block		A pointer to buffer containing block
 *
 * @returns		U3_SUCCESS if successful, else U3_FAILURE and
 * 			an error string can be obtained using u3_error()
 */
int u3_cd_write(u3_handle_t *device, uint32_t block_num, uint8_t *block);

/**
 * Direction to round sector count.
 * 
 * @see 'u3_sector_round()'
 */
enum round_dir { round_down=0, round_up=1 };

/**
 * Round partition sector count to prevered value
 *
 * Round a number of sectors to a sector count that is allowed for partitioning
 * the device.
 *
 * @param device	U3 device handle
 * @param direction	direction to round(up or down).
 * @param size		Size value to round. Rounded value is writen back to
 *			this variable.
 *
 * @returns		U3_SUCCESS if successful, else U3_FAILURE and
 * 			an error string can be obtained using u3_error()
 *
 * @see 'u3_partition()'
 */
int u3_partition_sector_round(u3_handle_t *device,
		enum round_dir direction, uint32_t *size);

/**
 * Round secure zone sector count to prevered value
 *
 * Round a number of sectors to a sector count that is allowed for a secure
 * zone on the data partition.
 *
 * @param device	U3 device handle
 * @param direction	direction to round(up or down).
 * @param size		Size value to round. Rounded value is writen back to
 *			this variable.
 *
 * @returns		U3_SUCCESS if successful, else U3_FAILURE and
 * 			an error string can be obtained using u3_error()
 *
 * @see 'u3_enable_security()'
 */
int u3_security_sector_round(u3_handle_t *device,
		enum round_dir direction, uint32_t *size);


/**
 * Request partitioning information
 *
 * This request some information about the partition sizes
 *
 * @param device	U3 device handle
 * @param info		Pointer to structure used to return requested info.
 *
 * @returns		U3_SUCCESS if successful, else U3_FAILURE and
 * 			an error string can be obtained using u3_error()
 *
 * @see 'struct part_info'
 */
int u3_partition_info(u3_handle_t *device, struct part_info *info);

/**
 * Request data partition information
 *
 * This request some information about the data partition
 *
 * @param device	U3 device handle
 * @param info		Pointer to structure used to return requested info.
 *
 * @returns		U3_SUCCESS if successful, else U3_FAILURE and
 * 			an error string can be obtained using u3_error()
 *
 * @see 'struct dpart_info'
 */
int u3_data_partition_info(u3_handle_t *device, struct dpart_info *info);

/**
 * Request chip information
 *
 * This request some information about the USB chip used on the device.
 *
 * @param device	U3 device handle
 * @param info		Pointer to structure used to return requested info.
 *
 * @returns		U3_SUCCESS if successful, else U3_FAILURE and
 * 			an error string can be obtained using u3_error()
 *
 * @see 'struct chip_info'
 */
int u3_chip_info(u3_handle_t *device, struct chip_info *info);

/**
 * Reset device
 *
 * This function tell's the device to reconnect. When the function returns
 * the device should have been reset and reconnected.
 * The exact working of this action is still vague
 *
 * @param device	U3 device handle
 *
 * @returns		U3_SUCCESS if successful, else U3_FAILURE and
 * 			an error string can be obtained using u3_error()
 */
int u3_reset(u3_handle_t *device);

/**
 * Enable device security
 *
 * This sets a password on the data partition.
 *
 * @param device	U3 device handle
 * @param password      The password for the private zone
 *
 * @returns		U3_SUCCESS if successful, else U3_FAILURE and
 * 			an error string can be obtained using u3_error()
 */
int u3_enable_security(u3_handle_t *device, const char *password);

/**
 * Disable device security
 *
 * This removes the password from the data partition.
 *
 * @param device	U3 device handle
 * @param password      The password for the private zone
 * @param result        Return variable for result True if unlocking succeeded,
 *                      else false
 *
 * @returns		U3_SUCCESS if successful, else U3_FAILURE and
 * 			an error string can be obtained using u3_error()
 */
int u3_disable_security(u3_handle_t *device, const char *password,
		int * result);

/**
 * Unlock data partition
 *
 * This unlocks the data partition if the device is secured.
 *
 * @param device	U3 device handle
 * @param password      The password for the private zone
 * @param result        Return variable for result True if unlocking succeeded,
 *                      else false
 *
 * @returns		U3_SUCCESS if successful, else U3_FAILURE and
 * 			an error string can be obtained using u3_error()
 */
int u3_unlock(u3_handle_t *device, const char *password, int *result);

/**
 * Change password of data partition
 *
 * This changes the password of the data partition.
 *
 * @param device	U3 device handle
 * @param old_password  The current password of the private zone
 * @param new_password  The new password of the private zone
 * @param result        Return variable for result: True if password changed,
 *                      else False if old password incorrect.
 *
 * @returns		U3_SUCCESS if successful, else U3_FAILURE and
 * 			an error string can be obtained using u3_error()
 */
int u3_change_password(u3_handle_t *device, const char *old_password,
		const char *new_password, int *result);


#endif // __U3_COMMAND__
