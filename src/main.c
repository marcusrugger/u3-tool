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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "u3.h"
#include "u3_commands.h"
#include "u3_scsi.h"
#include "u3_error.h"

#include "secure_input.h"
#include "display_progress.h"

#define TRUE 1
#define FALSE 0

#define MAX_SIZE_STRING_LENGTH 100
#define MAX_FILENAME_STRING_LENGTH 1024
#define MAX_PASSWORD_LENGTH 1024

char *version = "0.1";

int debug = 0;
int batch_mode = 0;

enum action_t { unknown, load, partition, dump, unlock, change_password,
		enable_security, disable_security };

/**
 * Ask confirmation of user.
 *
 * @returns	TRUE if user wants to continue else FALSE to abort.
 */
int confirm() {
	char c;
	int retval;
	int done;

	if (batch_mode != 0)
		return TRUE;

	retval = FALSE;
	done = FALSE;
	do {
		printf("\nAre you sure you want to continue? [yn] ");

		c = fgetc(stdin);

		if (c == 'y' || c == 'Y') {
			retval = TRUE;
			done = TRUE;
		} else if (c == 'n' || c == 'N') {
			retval = FALSE;
			done = TRUE;
		}

		// drop any extra chars and the new line
		while (c != '\n' && c != EOF) {
			c = fgetc(stdin);
		}
	} while (!done);

	return retval;
}

/********************************** Actions ***********************************/

int do_load(u3_handle_t *device, char *iso_filename) {
	struct stat file_stat;
	struct part_info pinfo;
	uint32_t cd_size;
	FILE *fp;
	uint8_t	buffer[U3_BLOCK_SIZE];
	unsigned int bytes_read=0;
	unsigned int block_num=0;
	unsigned int block_cnt=0;

	// determine file size
	if (stat(iso_filename, &file_stat) == -1) {
		perror("Failed stating iso file");
		return EXIT_FAILURE;
	}
	if (file_stat.st_size == 0) {
		fprintf(stderr, "ISO file is empty\n");
		return EXIT_FAILURE;
	}

	cd_size = file_stat.st_size / U3_SECTOR_SIZE;
	if (file_stat.st_size % U3_SECTOR_SIZE)
		cd_size++;
	block_cnt = file_stat.st_size / U3_BLOCK_SIZE;
	if (file_stat.st_size % U3_BLOCK_SIZE)
		block_cnt++;

	// check partition size
	if (u3_partition_info(device, &pinfo) != U3_SUCCESS) {
		fprintf(stderr, "u3_partition_info() failed: %s\n",
			u3_error_msg(device));
		return EXIT_FAILURE;
	}

	if (cd_size > pinfo.cd_size) {
		fprintf(stderr, "CD image(%lu byte) is to big for current cd "
			"partition(%u byte), please repartition device.\n",
			file_stat.st_size,
			pinfo.cd_size * U3_SECTOR_SIZE);
		return EXIT_FAILURE;
	}

	// open file
	if ((fp = fopen(iso_filename, "rb")) == NULL) {
		perror("Failed opening iso file");
		return EXIT_FAILURE;
	}

	// write file to device
	block_num = 0;
	do {
		display_progress(block_num, block_cnt);

		bytes_read = fread(buffer, sizeof(uint8_t), U3_BLOCK_SIZE, fp);
		if (bytes_read != U3_BLOCK_SIZE) {
			if (ferror(fp)) {
				fclose(fp);
				perror("\nFailed reading iso file");
				return EXIT_FAILURE;
			} else if (bytes_read == 0) {
				continue;
			} else if (bytes_read > U3_BLOCK_SIZE) {
				fprintf(stderr, "\nRead more bytes then requested, impossible");
				return EXIT_FAILURE;
			} else {
				// zeroize rest of buffer to prevent writing garbage
				memset(buffer+bytes_read, 0, U3_BLOCK_SIZE-bytes_read);
			}
		} 
		
		if (u3_cd_write(device, block_num, buffer) != U3_SUCCESS) {
			fclose(fp);
			fprintf(stderr, "\nu3_cd_write() failed: %s\n", u3_error_msg(device));
			return EXIT_FAILURE;
		}

		block_num++;
	} while (!feof(fp));
	display_progress(block_num, block_cnt);
	putchar('\n');

	fclose(fp);

	printf("OK\n");
	return EXIT_SUCCESS;
}

int do_partition(u3_handle_t *device, char *size_string) {
	uint64_t size;
	uint32_t cd_sectors;

	// determine file size
	size = strtoll(size_string, NULL, 0);
	cd_sectors = size / U3_SECTOR_SIZE;
	if (size % U3_SECTOR_SIZE)
		cd_sectors++;

	// repartition
	if (u3_partition(device, cd_sectors) != U3_SUCCESS) {
		fprintf(stderr, "u3_partition() failed: %s\n", u3_error_msg(device));
		return EXIT_FAILURE;
	}

	// reset device to make partitioning active
	if (u3_reset(device) != U3_SUCCESS) {
		fprintf(stderr, "u3_reset() failed: %s\n", u3_error_msg(device));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int do_unlock(u3_handle_t *device, char *password) {
	int result=0;

	if (u3_unlock(device, password, &result) != U3_SUCCESS) {
		fprintf(stderr, "u3_unlock() failed: %s\n", u3_error_msg(device));
		return EXIT_FAILURE;
	}

	if (result) {
		printf("OK\n");
		return EXIT_SUCCESS;
	} else {
		printf("Password incorrect\n");
		return EXIT_FAILURE;
	}
}

int do_change_password(u3_handle_t *device, char *password,
	char *new_password)
{
	int result=0;

	if (u3_change_password(device, password, new_password, &result)
		!= U3_SUCCESS)
	{
		fprintf(stderr, "u3_change_password() failed: %s\n", u3_error_msg(device));
		return EXIT_FAILURE;
	}

	if (result) {
		printf("OK\n");
		return EXIT_SUCCESS;
	} else {
		printf("Password incorrect\n");
		return EXIT_FAILURE;
	}
}

int do_enable_security(u3_handle_t *device, char *password) {
	if (u3_enable_security(device, password) != U3_SUCCESS) {
		fprintf(stderr, "u3_enable_security() failed: %s\n",
			u3_error_msg(device));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int do_disable_security(u3_handle_t *device, char *password) {
	int result=0;

	if (u3_disable_security(device, password, &result) != U3_SUCCESS) {
		fprintf(stderr, "u3_disable_security() failed: %s\n",
			u3_error_msg(device));
		return EXIT_FAILURE;
	}

	if (result) {
		printf("OK\n");
		return EXIT_SUCCESS;
	} else {
		printf("Password incorrect\n");
		return EXIT_FAILURE;
	}
}

int do_dump(u3_handle_t *device) {
	int retval = EXIT_SUCCESS;

	struct part_info pinfo;
	struct dpart_info dpinfo;
	struct chip_info cinfo;
	struct property_03 device_properties;
	struct property_0C security_properties;
	
	if (u3_partition_info(device, &pinfo) != U3_SUCCESS) {
		fprintf(stderr, "u3_partition_info() failed: %s\n",
			u3_error_msg(device));
		retval = EXIT_FAILURE;
	} else {
		printf("Partition info:\n");
		printf(" - Partition count: 0x%.2x\n", pinfo.partition_count);
		printf(" - Data partition size: %d byte(0x%.8x)\n",
			pinfo.data_size * U3_SECTOR_SIZE, pinfo.data_size);
		printf(" - Unknown1: 0x%.8x\n", pinfo.unknown1);
		printf(" - CD size: %d byte(0x%.8x)\n", pinfo.cd_size*U3_SECTOR_SIZE,
			pinfo.cd_size);
		printf(" - Unknown2: 0x%.8x\n", pinfo.unknown2);
		printf("\n");
	}

	if (u3_data_partition_info(device, &dpinfo) != U3_SUCCESS) {
		fprintf(stderr, "u3_data_partition_info() failed: %s\n",
			u3_error_msg(device));
		retval = EXIT_FAILURE;
	} else {
		printf("Data partition info:\n");
		printf(" - Data partition size: %d byte(0x%.8x)\n",
			dpinfo.total_size * U3_SECTOR_SIZE, dpinfo.total_size);
		printf(" - Secured zone size: 0x%.8x\n", dpinfo.secured_size);
		printf(" - Unlocked: 0x%.8x\n", dpinfo.unlocked);
		printf(" - Password try: 0x%.8x\n", dpinfo.pass_try);
		printf("\n");
	}

	if (u3_chip_info(device, &cinfo) != U3_SUCCESS) {
		fprintf(stderr, "u3_chip_info() failed: %s\n",
			u3_error_msg(device));
		retval = EXIT_FAILURE;
	} else {
		printf("Chip info:\n");
		printf(" - Manufacturer: %.*s\n", U3_MAX_CHIP_MANUFACTURER_LEN,
			cinfo.manufacturer);
		printf(" - Revision: %.*s\n", U3_MAX_CHIP_REVISION_LEN,
			cinfo.revision);
		printf("\n");
	}

	if (u3_read_device_property(device, 0x03,
				(uint8_t *) &device_properties,
				sizeof(device_properties)
				) != U3_SUCCESS)
	{
		fprintf(stderr, "u3_read_device_property() failed for "
			"property 0x03: %s\n", u3_error_msg(device));
		retval = EXIT_FAILURE;
	} else {
		if (device_properties.hdr.length !=
				sizeof(device_properties))
		{
			fprintf(stderr, "Length of property 0x03 is not the "
				"expected length. (len=%u)\n",
				device_properties.hdr.length);
			retval = EXIT_FAILURE;
		} else {
			printf("Property page 0x03:\n");
			printf(" - Device size: 0x%.8x\n",
				device_properties.device_size);
			printf(" - Device serial: %.*s\n",U3_MAX_SERIAL_LEN,
				device_properties.serial);
			printf(" - Full record length: 0x%.8x\n",
				device_properties.full_length);
			printf(" - Unknown1: 0x%.2x\n",
				device_properties.unknown1);
			printf(" - Unknown2: 0x%.8x\n",
				device_properties.unknown2);
			printf(" - Unknown3: 0x%.8x\n",
				device_properties.unknown3);
			printf("\n");
		}
	}



	if (u3_read_device_property(device, 0x0C,
				(uint8_t *) &security_properties,
				sizeof(security_properties)
				) != U3_SUCCESS)
	{
		fprintf(stderr, "u3_read_device_property() failed for "
			"property 0x0C: %s\n", u3_error_msg(device));
		retval = EXIT_FAILURE;
	} else {
		if (security_properties.hdr.length !=
				sizeof(security_properties))
		{
			fprintf(stderr, "Length of property 0x0C is not the "
				"expected length. (len=%u)\n",
				security_properties.hdr.length);
			retval = EXIT_FAILURE;
		} else {
			printf("Property page 0x0C:\n");
			printf(" - Max. pass. try: %u",
				security_properties.max_pass_try);
			printf("\n");
		}
	}

	return EXIT_SUCCESS;
}

/************************************ Main ************************************/

void usage(const char *name) {
	printf("u3_tool %s\n", version);
	printf("U3 USB stick manager\n");
	printf("\n");
	printf("Usage: %s [options] <device name>\n", name);
	printf("\n");
	printf("Options:\n");
	printf("\t-c                Change password\n");
	printf("\t-d                Disable device security\n");
	printf("\t-D                Dump all raw info(for debug)\n");
	printf("\t-e                Enable device security\n");
	printf("\t-h                Print this help message\n");
//TODO:	printf("\t-i                Display device info\n");
	printf("\t-l <cd image>     Load CD image into device\n");
	printf("\t-p <cd size>      Repartition device\n");
	printf("\t-u                Unlock device\n");
	printf("\t-v                Use verbose output\n");
	printf("\t-V                Print version information\n");
	printf("\n");
	printf("The device name depends on the used subsystem.\n");
	printf("Examples: '/dev/sda0'(sg), 'scan'(USB),'vid:pid'(USB), "
			"'e'(Windows)\n");
}

void print_version() {
	printf("u3_tool %s\n", version);
	printf("\n");
	printf("Copyright (C) 2007\n");
	printf("This is free software; see the source for copying "
		"conditions. There is NO\n warranty; not even for "
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");
}

int main(int argc, char *argv[]) {
	u3_handle_t device;

	int c;
	enum action_t action = unknown;
	char *device_name;

	char	filename_string[MAX_FILENAME_STRING_LENGTH+1];
	char	size_string[MAX_SIZE_STRING_LENGTH+1];

	int	ask_password = TRUE;
	char	password[MAX_PASSWORD_LENGTH+1];

	int	ask_new_password = TRUE;
	char	new_password[MAX_PASSWORD_LENGTH+1];

	int retval = EXIT_SUCCESS;

	//
	// parse options
	//
	while ((c = getopt(argc, argv, "cdDehl:p:uvVz")) != -1) {
		switch (c) {
			case 'c':
				action = change_password;
				break;
			case 'd':
				action = disable_security;
				break;
			case 'e':
				action = enable_security;
				break;
			case 'l':
				action = load;
				strncpy(filename_string, optarg, MAX_FILENAME_STRING_LENGTH);
				filename_string[MAX_FILENAME_STRING_LENGTH] = '\0';
				break;
			case 'p':
				action = partition;
				strncpy(size_string, optarg, MAX_SIZE_STRING_LENGTH);
				size_string[MAX_SIZE_STRING_LENGTH] = '\0';
				break;
			case 'u':
				action = unlock;
				break;
			case 'v':
				debug = 1;
				break;
			case 'V':
				print_version();
				exit(EXIT_SUCCESS);
				break;
			case 'D':
				action = dump;
				break;
			case 'h':
			default:
				usage(argv[0]);
				exit(EXIT_FAILURE);
				break;
		}
	}

	//
	// parse arguments
	//
	if (argc-optind < 1) {
		fprintf(stderr, "Not enough arguments\n");
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	device_name = argv[optind];
		
	if ((action == unlock || action == change_password
	     || action == disable_security)
	     && ask_password)
	{
		int proceed = 0;
		do {
			fprintf(stderr, "Enter password: ");
			secure_input(password, sizeof(password));
			if (strlen(password) == 0) {
				fprintf(stderr, "Password Empty\n");
			} else {
				proceed = 1;
			}
		} while (!proceed);
	}

	if ((action == change_password || action == enable_security)
		&& ask_new_password)
	{
		int proceed = 0;
		char validate_password[MAX_PASSWORD_LENGTH+1];
		do {
			fprintf(stderr, "Enter new password: ");
			secure_input(new_password, sizeof(new_password));
			if (strlen(new_password) == 0) {
				fprintf(stderr, "Password Empty\n");
				continue;
			}

			fprintf(stderr, "Reenter new password: ");
			secure_input(validate_password, sizeof(validate_password));
			if (strcmp(new_password, validate_password) == 0) {
				proceed = 1;
			} else {
				fprintf(stderr, "Passwords don't match\n");
			}
		} while (!proceed);
	}

	//
	// open the device
	// 
	if (u3_open(&device, device_name)) {
		fprintf(stderr, "Error opening device: %s\n", u3_error_msg(&device));
		exit(EXIT_FAILURE);
	}

	//
	// preform action
	//
	switch (action) {
		case load:
			retval = do_load(&device, filename_string);
			break;
		case partition:
			printf("\n");
			printf("WARNING: Loading a new cd image causes the ");
			printf("whole device to be whiped. This INCLUDES\n ");
			printf("the data partition.\n");
			printf("I repeat: ANY EXCISTING DATA WILL BE LOST!\n");
			if (confirm())
				retval = do_partition(&device, size_string);
			break;
		case dump:
			retval = do_dump(&device);
			break;
		case unlock:
			retval = do_unlock(&device, password);
			break;
		case change_password:
			retval = do_change_password(&device, password, new_password);
			break;
		case enable_security:
			retval = do_enable_security(&device, new_password);
			break;
		case disable_security:
			retval = do_disable_security(&device, password);
			break;
		default:
			printf("Don't know what to do... please specify an action\n");
			break;
	}

	//
	// clean up
	//
	u3_close(&device);

	return retval;
}
