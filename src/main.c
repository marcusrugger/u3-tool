/**
 * u3-tool - U3 USB stick manager
 * Copyright (C) 2009 Daviedev, daviedev@users.sourceforge.net
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
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

static char *version = VERSION;

int debug = 0;
static int batch_mode = 0;
static int quit = 0;

enum action_t { unknown, load, partition, dump, info, unlock, change_password,
		enable_security, disable_security, reset_security };

/********************************** Helpers ***********************************/

/**
 * Ask confirmation of user.
 *
 * @returns	TRUE if user wants to continue else FALSE to abort.
 */
static int confirm(void) {
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


/**
 * Symbols of size multiplication factors.
 */
static char factor_symbols[] = " kMGTPE";

/**
 * Print bytes is human readable fashion.
 *
 * @param size	Data size to print
 */
static void print_human_size(uint64_t size) {
	double fsize = 0;
	unsigned int factor = 0;
	
	fsize = size;
	while (fsize >= 1024) {
		fsize /= 1024;
		factor++;
	}

	printf("%.2f %cB", fsize, factor_symbols[factor]);
}

/**
 * Get pin tries left
 *
 * @param device	U3 device handle
 * @param tries		Used to return the number of tries left till device is blocked, or 0 if already blocked
 *
 * @returns		U3_SUCCESS if successful, else U3_FAILURE and
 * 			an error string can be obtained using u3_error()
 */
static int get_tries_left(u3_handle_t *device, int *tries) {
	struct dpart_info dpinfo;
	struct property_0C security_properties;

 	*tries = 0;

	if (u3_data_partition_info(device, &dpinfo) != U3_SUCCESS) {
		fprintf(stderr, "u3_data_partition_info() failed: %s\n",
			u3_error_msg(device));
		return U3_FAILURE;
	}

	if (u3_read_device_property(device, 0x0C,
				(uint8_t *) &security_properties,
				sizeof(security_properties)
				) != U3_SUCCESS)
	{
		fprintf(stderr, "u3_read_device_property() failed for "
			"property 0x0C: %s\n", u3_error_msg(device));
		return U3_FAILURE;
	}

	*tries = security_properties.max_pass_try - dpinfo.pass_try;

	return U3_SUCCESS;
}

/********************************** Actions ***********************************/

static int do_load(u3_handle_t *device, char *iso_filename) {
	struct stat file_stat;
	struct part_info pinfo;
	off_t cd_size;
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

	cd_size = ((uintmax_t) file_stat.st_size) / U3_SECTOR_SIZE;
	if (file_stat.st_size % U3_SECTOR_SIZE)
		cd_size++;
	block_cnt = ((uintmax_t) file_stat.st_size) / U3_BLOCK_SIZE;
	if (file_stat.st_size % U3_BLOCK_SIZE)
		block_cnt++;

	// check partition size
	if (u3_partition_info(device, &pinfo) != U3_SUCCESS) {
		fprintf(stderr, "u3_partition_info() failed: %s\n",
			u3_error_msg(device));
		return EXIT_FAILURE;
	}

	if (cd_size > pinfo.cd_size) {
		fprintf(stderr, "CD image (%ju bytes) is to big for current CD "
			"partition (%llu bytes)\n",
			(uintmax_t) file_stat.st_size,
			1ll * U3_SECTOR_SIZE * pinfo.cd_size);
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
	} while (!feof(fp) && !quit);
	display_progress(block_num, block_cnt);
	putchar('\n');

	fclose(fp);

	printf("OK\n");
	return EXIT_SUCCESS;
}

static int do_partition(u3_handle_t *device, char *size_string) {
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

static int do_unlock(u3_handle_t *device, char *password) {
	int result=0;
	int tries_left=0;

	// check password retry counter
	if (get_tries_left(device, &tries_left) != U3_SUCCESS) {
		return EXIT_FAILURE;
	}

	if (tries_left == 0) {
		printf("Unable to unlock, device is blocked\n");
		return EXIT_FAILURE;
	} else if (tries_left == 1) {
		printf("Warning: This is the your last password try. If this attempt fails,");
		printf(" all data on the data partition is lost.\n");
		if (!confirm())
			return EXIT_FAILURE;
	}

	// unlock device
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

static int do_change_password(u3_handle_t *device, char *password,
	char *new_password)
{
	int result=0;
	int tries_left=0;

	// check password retry counter
	if (get_tries_left(device, &tries_left) != U3_SUCCESS) {
		return EXIT_FAILURE;
	}

	if (tries_left == 0) {
		printf("Unable to change password, device is blocked\n");
		return EXIT_FAILURE;
	} else if (tries_left == 1) {
		printf("Warning: This is the your last password try. If this attempt fails,");
		printf(" all data on the data partition is lost.\n");
		if (!confirm())
			return EXIT_FAILURE;
	}

	// change password
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

static int do_enable_security(u3_handle_t *device, char *password) {
	if (u3_enable_security(device, password) != U3_SUCCESS) {
		fprintf(stderr, "u3_enable_security() failed: %s\n",
			u3_error_msg(device));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static int do_disable_security(u3_handle_t *device, char *password) {
	int result=0;
	int tries_left=0;

	// check password retry counter
	if (get_tries_left(device, &tries_left) != U3_SUCCESS) {
		return EXIT_FAILURE;
	}

	if (tries_left == 0) {
		printf("Unable to disable security, device is blocked\n");
		return EXIT_FAILURE;
	} else if (tries_left == 1) {
		printf("Warning: This is the your last password try. If this attempt fails,");
		printf(" all data on the data partition is lost.\n");
		if (!confirm())
			return EXIT_FAILURE;
	}

	// disable security
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

static int do_reset_security(u3_handle_t *device) {
	int result=0;

	// the enable security command is always possible without the correct
	// password, even if the device is locked. To work around asking for
	// a password we first call the enable security command with a known
	// password before calling disable security.
	if (u3_enable_security(device, "password") != U3_SUCCESS) {
		fprintf(stderr, "u3_enable_security() failed: %s\n",
			u3_error_msg(device));
		return EXIT_FAILURE;
	}

	if (u3_disable_security(device, "password", &result) != U3_SUCCESS) {
		fprintf(stderr, "u3_disable_security() failed: %s\n",
			u3_error_msg(device));
		return EXIT_FAILURE;
	}

	if (result) {
		printf("OK\n");
		return EXIT_SUCCESS;
	} else {
		printf("Failed\n");
		return EXIT_FAILURE;
	}
}

static int do_info(u3_handle_t *device) {
	struct part_info pinfo;
	struct dpart_info dpinfo;
	struct chip_info cinfo;
	struct property_03 device_properties;
	struct property_0C security_properties;

	if (u3_partition_info(device, &pinfo) != U3_SUCCESS) {
		fprintf(stderr, "u3_partition_info() failed: %s\n",
			u3_error_msg(device));
		return EXIT_FAILURE;
	}

	if (u3_data_partition_info(device, &dpinfo) != U3_SUCCESS) {
		fprintf(stderr, "u3_data_partition_info() failed: %s\n",
			u3_error_msg(device));
		return EXIT_FAILURE;
	}

	if (u3_chip_info(device, &cinfo) != U3_SUCCESS) {
		fprintf(stderr, "u3_chip_info() failed: %s\n",
			u3_error_msg(device));
		return EXIT_FAILURE;
	}

	if (u3_read_device_property(device, 0x03,
				(uint8_t *) &device_properties,
				sizeof(device_properties)
				) != U3_SUCCESS)
	{
		fprintf(stderr, "u3_read_device_property() failed for "
			"property 0x03: %s\n", u3_error_msg(device));
		return EXIT_FAILURE;
	}

	if (u3_read_device_property(device, 0x0C,
				(uint8_t *) &security_properties,
				sizeof(security_properties)
				) != U3_SUCCESS)
	{
		fprintf(stderr, "u3_read_device_property() failed for "
			"property 0x0C: %s\n", u3_error_msg(device));
		return EXIT_FAILURE;
	}

	printf("Total device size:   ");
	print_human_size(1ll * U3_SECTOR_SIZE * device_properties.device_size);
	printf(" (%llu bytes)\n", 1ll * U3_SECTOR_SIZE * device_properties.device_size);
	fflush(stdout);    /* flush stdout stream to redirect stdout*/ 

	printf("CD size:             ");
	print_human_size(1ll * U3_SECTOR_SIZE * pinfo.cd_size);
	printf(" (%llu bytes)\n", 1ll * U3_SECTOR_SIZE * pinfo.cd_size);

	if (dpinfo.secured_size == 0) {
		printf("Data partition size: ");
		print_human_size(1ll * U3_SECTOR_SIZE * dpinfo.total_size);
		printf(" (%llu bytes)\n", 1ll * U3_SECTOR_SIZE * dpinfo.total_size);
	} else {
		printf("Secured zone size:   ");
		print_human_size(1ll * U3_SECTOR_SIZE * dpinfo.secured_size);
		printf(" (%llu bytes)\n", 1ll * U3_SECTOR_SIZE * dpinfo.secured_size);

		printf("Secure zone status:  ");
		if(dpinfo.unlocked) {
			printf("unlocked\n");
		} else {
			if (security_properties.max_pass_try == dpinfo.pass_try) {
				printf("BLOCKED!\n");
			} else {
				printf("locked (%u tries left)\n", security_properties.max_pass_try - dpinfo.pass_try);
			}
		}
	}
	return EXIT_SUCCESS;
}

static int do_dump(u3_handle_t *device) {
	int retval;

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
		printf(" - Data partition size: %llu byte(0x%.8x)\n",
			1ll * U3_SECTOR_SIZE * pinfo.data_size, pinfo.data_size);
		printf(" - Unknown1: 0x%.8x\n", pinfo.unknown1);
		printf(" - CD size: %llu byte(0x%.8x)\n", 1ll * U3_SECTOR_SIZE * pinfo.cd_size,
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
		printf(" - Data partition size: %llu byte(0x%.8x)\n",
			1ll * U3_SECTOR_SIZE * dpinfo.total_size , dpinfo.total_size);
		printf(" - Secured zone size: %llu byte(0x%.8x)\n",
			1ll * U3_SECTOR_SIZE *  dpinfo.secured_size, dpinfo.secured_size);
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
			printf(" - Device size: %llu byte(0x%.8x)\n",
				1ll * U3_SECTOR_SIZE * device_properties.device_size, device_properties.device_size);
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

	return retval;
}

/************************************ Main ************************************/

static void usage(const char *name) {
	printf("u3-tool %s - U3 USB stick manager\n", version);
	printf("\n");
	printf("Usage: %s [options] <device name>\n", name);
	printf("\n");
	printf("Options:\n");
	printf("\t-c                Change password\n");
	printf("\t-d                Disable device security\n");
	printf("\t-D                Dump all raw info(for debug)\n");
	printf("\t-e                Enable device security\n");
	printf("\t-h                Print this help message\n");
	printf("\t-i                Display device info\n");
	printf("\t-l <cd image>     Load CD image into device\n");
	printf("\t-p <cd size>      Repartition device\n");
	printf("\t-R                Reset device security, destroying private data\n");
	printf("\t-u                Unlock device\n");
	printf("\t-v                Use verbose output\n");
	printf("\t-V                Print version information\n");
	printf("\n");
	printf("For the device name use:\n  %s\n", u3_subsystem_help);
}

static void print_version(void) {
	printf("u3-tool %s\n", version);
	printf("subsystem: %s\n", u3_subsystem_name);
	printf("\n");
	printf("Copyright (C) 2009\n");
	printf("This is free software; see the source for copying "
		"conditions. There is NO\nwarranty; not even for "
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");
}

static void set_quit(int sig) {
	quit = 1;
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
	while ((c = getopt(argc, argv, "cdDehil:p:RuvVz")) != -1) {
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
			case 'i':
				action = info;
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
			case 'R':
				action = reset_security;
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

	assert(signal(SIGINT, set_quit) != SIG_ERR);
	assert(signal(SIGTERM, set_quit) != SIG_ERR);

	//
	// open the device
	// 
	if (u3_open(&device, device_name)) {
		fprintf(stderr, "Error opening device: %s\n", u3_error_msg(&device));
		exit(EXIT_FAILURE);
	}

	//
	// ask passwords
	//
	if ((action == unlock || action == change_password || action == disable_security)
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
			
			memset(validate_password, 0, sizeof(validate_password));
		} while (!proceed);
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
			printf("whole device to be wiped. This INCLUDES\n ");
			printf("the data partition.\n");
			printf("I repeat: ANY EXISTING DATA WILL BE LOST!\n");
			if (confirm())
				retval = do_partition(&device, size_string);
			break;
		case dump:
			retval = do_dump(&device);
			break;
		case info:
			retval = do_info(&device);
			break;
		case unlock:
			retval = do_unlock(&device, password);
			break;
		case change_password:
			retval = do_change_password(&device, password, new_password);
			break;
		case enable_security:
			printf("WARNING: This will delete all data on the data ");
			printf("partition\n");
			if (confirm())
				retval = do_enable_security(&device, new_password);
			break;
		case disable_security:
			retval = do_disable_security(&device, password);
			break;
		case reset_security:
			printf("WARNING: This will delete all data on the data ");
			printf("partition\n");
			if (confirm())
				retval = do_reset_security(&device);
			break;
		default:
			fprintf(stderr, "No action specified, use '-h' option for help.\n");
			break;
	}

	//
	// clean up
	//
	memset(password, 0, sizeof(password));
	memset(new_password, 0, sizeof(new_password));
	u3_close(&device);

	return retval;
}
