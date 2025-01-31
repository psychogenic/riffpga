/*
 * board_defs.h, part of the riffpga project
 *
 * Defines used internally.
 *
 *
 *      Author: Pat Deegan
 *    Copyright (C) 2025 Pat Deegan, https://psychogenic.com
 *
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#ifndef _BOARD_DEFINITIONS_H_
#define _BOARD_DEFINITIONS_H_

#include "board_includes.h"
#include "board_config_defaults.h"
#include "config_defaults/sys_version.h"

#define TINYUF2_CONST

#define BOARD_FLASH_SIZE  		(512 * 1024)
#define BOARD_FLASH_ADDR_ZERO   0
// Comes from #define hardware_flash/include/hardware/flash.h FLASH_SECTOR_SIZE		(4*1024)

#define FLASH_SPI_XFER_BLOCKSIZE 	256 /* keep it short so we stay responsive */



// All entries are little endian.
#define UF2_MAGIC_START0    0x0A324655UL // "UF2\n"


#define FLASH_RESET_DELAY_MS		2

#define DEBUG_START_PROGRAM_DELAY_MS	500


#define UF2_VERSION BOARD_VERSION_MAJOR "." BOARD_VERSION_MINOR "." BOARD_VERSION_PATCH
#define UF2_PRODUCT_NAME 	"riffpga"
#define UF2_BOARD_ID 		BOARD_NAME
#define UF2_INDEX_URL 		"https://psychogenic.com/riffpga"
#define UF2_VOLUME_LABEL 	DRIVE_VOLUME_LABEL
//define TINYUF2_FAVICON_HEADER





#endif
