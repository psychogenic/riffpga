/*
 * bitstream.h, part of the riffpga project
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

#include "board_includes.h"
#include "uf2.h"


typedef  void (*bs_prog_yield_cb)(void);

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


void bs_write_marker(uint32_t num_blocks, uint32_t bitstream_size, uint32_t address_start);
void bs_write_marker_to_slot(uint8_t slot, uint32_t num_blocks, uint32_t bitstream_size, uint32_t address_start);



typedef struct bitstream_state_struct {
	bool have_checked;
	UF2_Block info;
	uint32_t size;
	uint32_t uf2_file_size;
	uint32_t start_address;

} Bitstream_Marker_State;
const Bitstream_Marker_State * bs_marker_get(void);



#endif /* SRC_BITSTREAM_H_ */
