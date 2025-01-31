/*
 * bitstream.h, part of the riffpga project
 *
 * Functionality related to the binary bitstream stored in
 * a slot in flash, as well as the related meta-data.
 *
 *  Created on: Dec 18, 2024
 *      Author: Pat Deegan
 *    Copyright (C) 2025 Pat Deegan, https://psychogenic.com
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

#ifndef SRC_BITSTREAM_H_
#define SRC_BITSTREAM_H_

#include "board_defs.h"

#include "uf2.h"


typedef  void (*bs_prog_yield_cb)(void);

/*
 * Bitstream_MetaInfo
 * contents of meta info UF2 data block
 * passed within written UF2 bin files
 */
typedef struct RIF_PACKED_STRUCT bitstream_metainfo_struct {
	uint32_t bssize;
	uint8_t namelen;
	char name[BITSTREAM_NAME_MAXLEN];
	uint32_t clock_hz;
} Bitstream_MetaInfo;



typedef struct bitstream_metainfo_payloadstruct {
	uint8_t header[8]; // RFMETA01
	Bitstream_MetaInfo info;
} Bitstream_MetaInfo_Payload;


typedef struct bsslotstatestruct {
	bool found;
	Bitstream_MetaInfo info;
} Bitstream_Slot_Content;



void bs_init(void);

bool bs_have_checked_for_marker(void);
void bs_clear_size_check_flag(void);

/* performs check for a valid size marker in flash,
 * returns actual bitstream size if found, 0 otherwise.
 */
uint32_t bs_check_for_marker(void);

/* if check_for_info succeeded,
 * the info returns the block itself,
 * and uf2_file_size() returns the UF2 file
 * size to declare (which is bigger, because of meta data)
 */
UF2_Block * bs_info(void);
uint32_t bs_uf2_file_size(void);
uint32_t bs_file_size(void);


bool bs_program_fpga(bs_prog_yield_cb cb);


// pass it an array of Bitstream_Slot_Content[POSITION_SLOTS_ALLOWED]
// returns num found
uint8_t bs_slot_contents(Bitstream_Slot_Content * contents);


void bs_write_marker(uint32_t num_blocks, uint32_t bitstream_size,
		uint32_t address_start, Bitstream_MetaInfo *info);

void bs_write_marker_to_slot(uint8_t slot, uint32_t num_blocks, uint32_t bitstream_size,
		uint32_t address_start, Bitstream_MetaInfo *info);

void bs_erase_slot(uint8_t slot);
void bs_erase_all(void);

typedef struct bitstream_settings_struct {
	uint32_t size;
	uint32_t uf2_file_size;
	uint32_t start_address;
	Bitstream_MetaInfo user_info;
} Bitstream_Settings;

typedef struct bitstream_state_struct {
	bool have_checked;
	UF2_Block info;
	Bitstream_Settings settings;
} Bitstream_Marker_State;

const Bitstream_Marker_State * bs_marker_get(void);
const Bitstream_Settings * bs_settings_get(void);

uint32_t bs_load_marker(uint8_t slot, Bitstream_Marker_State * into);



#endif /* SRC_BITSTREAM_H_ */
