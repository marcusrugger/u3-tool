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

#include "u3_commands.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "u3_scsi.h"
#include "u3_error.h"
#include "md5.h"

#ifdef WIN32
# include <windows.h>
# define sleep Sleep
// Barf, winsock... NOT
# define htonl(A)  ((((uint32_t)(A) & 0xff000000) >> 24) | \
                   (((uint32_t)(A) & 0x00ff0000) >> 8)  | \
                   (((uint32_t)(A) & 0x0000ff00) << 8)  | \
                   (((uint32_t)(A) & 0x000000ff) << 24))
#else
# include <arpa/inet.h> // for htonl()
#endif

#if __BYTE_ORDER != __LITTLE_ENDIAN
# error "This tool only runs on Little Endian machines"
#endif

#define U3_PASSWORD_HASH_LEN 16

/**
 * Convert textual password to hash.
 *
 * @param pass	The password string, null terminated.
 * @param hash	Buffer to write hash to, should be atleast
 * 		U3_PASSWORD_HASH_LEN long.
 */
void u3_pass_to_hash(const char *password, uint8_t *hash) {
	unsigned int passlen;
	unsigned int unicode_passlen;
	uint8_t *unicode_pass;
	int i;

	passlen = strlen(password);
	unicode_passlen = (passlen+1)*2;
	unicode_pass = (uint8_t *) calloc(passlen+1, 2);

	for (i = 0; i < passlen + 1; i++) {
		unicode_pass[i*2] = password[i];
	}
	
	md5(unicode_pass, unicode_passlen, hash);

	memset(unicode_pass, 0xFF, unicode_passlen);
	memset(unicode_pass, 0, unicode_passlen);
	free(unicode_pass);
}

/**** public function ***/

int u3_read_device_property(u3_handle_t *device, uint16_t property_id,
		uint8_t *buffer, uint16_t buffer_length)
{
	uint8_t status;
	uint8_t cmd[U3_CMD_LEN] = {
		0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};
	struct _write_cmd_t {
		uint8_t	 scsi_opcode;
		uint16_t command;
		uint16_t id;
		uint16_t length;
		uint16_t unknown1;
		uint8_t unknown2;
	} __attribute__ ((packed)) *property_command;
	struct property_header *hdr;


	if (buffer_length < 6) {
		u3_set_error(device, "buffer supplied to "
				"u3_read_device_property() is to small");
		return U3_FAILURE;
	}

	// First read header to prevent reading to much data
	hdr = (struct property_header *) buffer;
	property_command = (struct _write_cmd_t *) &cmd;
	property_command->id = property_id;
	property_command->length = sizeof(struct property_header);

	if (u3_send_cmd(device, cmd, U3_DATA_FROM_DEV, property_command->length,
		buffer, &status) != U3_SUCCESS)
	{
		return U3_FAILURE;
	}

	if (status != 0) {
		u3_set_error(device, "Header of property 0x%.4X could not be "
			"read.", property_id);
		return U3_FAILURE;
	}

	if (hdr->length < sizeof(struct property_header)) {
		u3_set_error(device, "Header of property 0x%.4X indicates a "
			"length of smaller then the header size. (len = %u)"
			, property_id, hdr->length);
		return U3_FAILURE;
	}

	// Read full property
	if (hdr->length > buffer_length) {
		property_command->length = buffer_length;
	} else {
		property_command->length = hdr->length;
	}

	if (u3_send_cmd(device, cmd, U3_DATA_FROM_DEV, property_command->length,
		buffer, &status) != U3_SUCCESS)
	{
		return U3_FAILURE;
	}

	if (status != 0) {
		u3_set_error(device, "Property 0x%.4X could not be read.",
			property_id);
		return U3_FAILURE;
	}

	return U3_SUCCESS;
}

int u3_partition(u3_handle_t *device, uint32_t cd_size) {
	uint8_t status;
	uint8_t cmd[U3_CMD_LEN] = {
		0xff, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};
	struct part_info data;
	struct property_03 device_properties;
	uint32_t data_size;

	// calculate data partition size
	if (cd_size != 0) {
		if (u3_partition_sector_round(device, round_up, &cd_size) != U3_SUCCESS) {
			return U3_FAILURE;
		}
	}

	if (u3_read_device_property(device, 3, (uint8_t *) &device_properties,
			sizeof(device_properties)) != U3_SUCCESS)
	{
		return U3_FAILURE;
	}

	if (device_properties.hdr.length != sizeof(device_properties)) {
		u3_set_error(device, "Unexpected device property(0x03) length"
			", won't risk breaking device. Aborting! (len=%X)", 
			device_properties.hdr.length);
		return U3_FAILURE;
	}
	if (cd_size > device_properties.device_size) {
		u3_set_error(device, "Requested CD size is to big for device");
		return U3_FAILURE;
	}

	data_size = device_properties.device_size - cd_size;
	if (u3_partition_sector_round(device, round_down, &data_size) != U3_SUCCESS) {
		return U3_FAILURE;
	}
	
	// fill command data
	memset(&data, 0, sizeof(data));
	if (cd_size == 0) {
		data.partition_count = 1;
		data.unknown1 = 0x02;
		data.data_size = data_size;
		data.unknown2 = 0;
		data.cd_size = 0;
	} else {
		data.partition_count = 2;
		data.unknown1 = 0x02;
		data.data_size = data_size;
		data.unknown2 = 0x103;
		data.cd_size = cd_size;
	}

	if (u3_send_cmd(device, cmd, U3_DATA_TO_DEV, sizeof(data),
		(uint8_t *) &data, &status) != U3_SUCCESS)
	{
		return U3_FAILURE;
	}

	if (status != 0) {
		u3_set_error(device, "Device reported command failed: status %d", status);
		return U3_FAILURE;
	}

	return U3_SUCCESS;
}

int u3_cd_write(u3_handle_t *device, uint32_t block_num, uint8_t *block) {
	uint8_t status;
	uint8_t cmd[U3_CMD_LEN] = {
		0xff, 0x42, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x01
	};
	struct _write_cmd_t {
		uint8_t	 scsi_opcode;
		uint16_t command;
		uint8_t  unknown1;
		uint32_t block_num;
		uint32_t unknown2;
	} __attribute__ ((packed)) *write_command;

	// fill command data
	write_command = (struct _write_cmd_t *) &cmd;
	write_command->block_num = htonl(block_num);

	if (u3_send_cmd(device, cmd, U3_DATA_TO_DEV, 2048,
		block, &status) != U3_SUCCESS)
	{
		return U3_FAILURE;
	}

	if (status != 0) {
		u3_set_error(device, "Device reported command failed: status %d", status);
		return U3_FAILURE;
	}

	return U3_SUCCESS;
}


int u3_partition_sector_round(u3_handle_t *device,
		enum round_dir direction, uint32_t *size)
{
	uint8_t status;
	uint8_t cmd[U3_CMD_LEN] = {
		0xff, 0x20, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};
	uint32_t rounded_size;
	
	// fill command data
	*((uint32_t *)(cmd+4)) = *size;
	cmd[8] = direction;

	if (u3_send_cmd(device, cmd, U3_DATA_FROM_DEV, sizeof(rounded_size),
		(uint8_t *) &rounded_size, &status) != U3_SUCCESS)
	{
		return U3_FAILURE;
	}

	if (status != 0) {
		u3_set_error(device,
			"Device reported command failed: status %d", status);
		return U3_FAILURE;
	}

	*size = rounded_size;

	return U3_SUCCESS;
}

int u3_security_sector_round(u3_handle_t *device,
		enum round_dir direction, uint32_t *size)
{
	uint8_t status;
	uint8_t cmd[U3_CMD_LEN] = {
		0xff, 0xA3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};
	uint32_t rounded_size;
	
	// fill command data
	*((uint32_t *)(cmd+4)) = *size;
	cmd[8] = direction;

	if (u3_send_cmd(device, cmd, U3_DATA_FROM_DEV, sizeof(rounded_size),
		(uint8_t *) &rounded_size, &status) != U3_SUCCESS)
	{
		return U3_FAILURE;
	}

	if (status != 0) {
		u3_set_error(device,
			"Device reported command failed: status %d", status);
		return U3_FAILURE;
	}

	*size = rounded_size;

	return U3_SUCCESS;
}

int u3_partition_info(u3_handle_t *device, struct part_info *info) {
	uint8_t status;
	uint8_t cmd[U3_CMD_LEN] = {
		0xff, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};
	
	memset(info, 0, sizeof(struct part_info));
	
	if (u3_send_cmd(device, cmd, U3_DATA_FROM_DEV, 9,
		(uint8_t *)info, &status) != U3_SUCCESS)
	{
		return U3_FAILURE;
	}

//TODO: Find out if it is possible to define more then 2 partition. if so, make
//this more dynamic
	if (info->partition_count == 2) {
		if (u3_send_cmd(device, cmd, U3_DATA_FROM_DEV, 16,
			(uint8_t *)info, &status) != U3_SUCCESS)
		{
			return U3_FAILURE;
		}
	}

	if (status != 0) {
		u3_set_error(device, "Device reported command failed: status %d", status);
		return U3_FAILURE;
	}

	return U3_SUCCESS;
}

int u3_data_partition_info(u3_handle_t *device, struct dpart_info *info) {
	uint8_t status;
	uint8_t cmd[U3_CMD_LEN] = {
		0xff, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};
	
	memset(info, 0, sizeof(struct dpart_info));
	
	if (u3_send_cmd(device, cmd, U3_DATA_FROM_DEV, sizeof(struct dpart_info),
		(uint8_t *)info, &status) != U3_SUCCESS)
	{
		return U3_FAILURE;
	}

	if (status != 0) {
		u3_set_error(device, "Device reported command failed: status %d", status);
		return U3_FAILURE;
	}

	return U3_SUCCESS;
}

int u3_chip_info(u3_handle_t *device, struct chip_info *info) {
	uint8_t status;
	uint8_t cmd[U3_CMD_LEN] = {
		0xff, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};
	
	memset(info, 0, sizeof(struct chip_info));
	
	if (u3_send_cmd(device, cmd, U3_DATA_FROM_DEV, sizeof(struct chip_info),
		(uint8_t *)info, &status) != U3_SUCCESS)
	{
		return U3_FAILURE;
	}

	if (status != 0) {
		u3_set_error(device, "Device reported command failed: status %d", status);
		return U3_FAILURE;
	}

	return U3_SUCCESS;
}

int u3_reset(u3_handle_t *device) {
	uint8_t status;
	uint8_t cmd[U3_CMD_LEN] = {
		0xff, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};
	uint8_t data[12] = { // the magic numbers
		0x50, 0x00, 0x00, 0x00, 0x40, 0x9c, 0x00, 0x00,
		0x01, 0x00, 0x00, 0x00
	};
	
	if (u3_send_cmd(device, cmd, U3_DATA_TO_DEV, sizeof(data),
		data, &status) != U3_SUCCESS)
	{
		return U3_FAILURE;
	}

	if (status != 0) {
		u3_set_error(device, "Device reported command failed: status %d", status);
		return U3_FAILURE;
	}

	
	sleep(2); // wait for device to reset

//	u3_reopen(device);

	return U3_SUCCESS;
}

int u3_enable_security(u3_handle_t *device, const char *password) {
	uint8_t status;
	uint8_t cmd[U3_CMD_LEN] = {
		0xff, 0xA2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};
	struct {
		uint32_t size;
		uint8_t  hash[U3_PASSWORD_HASH_LEN];
	} __attribute__ ((packed)) data;
	struct dpart_info dp_info;
	uint32_t secure_zone_size;

	// determine size
//TODO: allow user to determine secure zone size... However currently we don't
// understand why disable security only works on a fully secured partition...
	if (u3_data_partition_info(device, &dp_info) != U3_SUCCESS) {
		return U3_FAILURE;
	}
	secure_zone_size = dp_info.total_size;

	if (secure_zone_size != 0) {
		if (u3_security_sector_round(device, round_up,
				&secure_zone_size) != U3_SUCCESS)
		{
			return U3_FAILURE;
		}
	}

	// fill command data
	data.size = secure_zone_size;
	u3_pass_to_hash(password, data.hash);

	if (u3_send_cmd(device, cmd, U3_DATA_TO_DEV, sizeof(data),
		(uint8_t *) &data, &status) != U3_SUCCESS)
	{
		return U3_FAILURE;
	}

	if (status != 0) {
		u3_set_error(device, "Device reported command failed: status %d", status);
		return U3_FAILURE;
	}

	return U3_SUCCESS;
}

int u3_disable_security(u3_handle_t *device, const char *password,
		int * result)
{
	uint8_t status;
	uint8_t cmd[U3_CMD_LEN] = {
		0xff, 0xA7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};
        uint8_t passhash_buf[U3_PASSWORD_HASH_LEN];

	*result = 0;
	u3_pass_to_hash(password, passhash_buf);
	
	if (u3_send_cmd(device, cmd, U3_DATA_TO_DEV, sizeof(passhash_buf),
		passhash_buf, &status) != U3_SUCCESS)
	{
		return U3_FAILURE;
	}

	if (status == 0) {
		*result = 1;
	}

	return U3_SUCCESS;
}

int u3_unlock(u3_handle_t *device, const char *password, int *result) {
	uint8_t status;
	uint8_t cmd[U3_CMD_LEN] = {
		0xff, 0xA4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};
        uint8_t passhash_buf[U3_PASSWORD_HASH_LEN];

	*result = 0;
	u3_pass_to_hash(password, passhash_buf);
	
	if (u3_send_cmd(device, cmd, U3_DATA_TO_DEV, sizeof(passhash_buf),
		passhash_buf, &status) != U3_SUCCESS)
	{
		return U3_FAILURE;
	}

	if (status == 0) {
		*result = 1;
	}

	return U3_SUCCESS;
}

int u3_change_password(u3_handle_t *device, const char *old_password,
		const char *new_password, int *result)
{
	uint8_t status;
	uint8_t cmd[U3_CMD_LEN] = {
		0xff, 0xA6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};
        uint8_t passhash_buf[U3_PASSWORD_HASH_LEN * 2];

	*result = 0;
	u3_pass_to_hash(old_password, passhash_buf);
	u3_pass_to_hash(new_password, passhash_buf+U3_PASSWORD_HASH_LEN);
	
	if (u3_send_cmd(device, cmd, U3_DATA_TO_DEV, sizeof(passhash_buf),
		(uint8_t *) passhash_buf, &status) != U3_SUCCESS)
	{
		return U3_FAILURE;
	}

	if (status == 0) {
		*result = 1;
	}

	return U3_SUCCESS;
}

