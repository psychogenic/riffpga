/*
 * bitstream.c, part of the riffpga project
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

#include "bitstream.h"
#include "board_defs.h"
#include "board.h"
#include "debug.h"
#include "fpga.h"
#include "board_config.h"

// define BS_DEBUG_ENABLE
#ifdef BS_DEBUG_ENABLE
#define BS_DEBUG_ENDLN()	DEBUG_ENDLN()
#define BS_DEBUG_BUF(b, len) DEBUG_BUF(b, len)
#define BS_DEBUG(s) DEBUG(s)
#define BS_DEBUG_LN(s)	DEBUG_LN(s)

#define BS_DEBUG_U32(v) DEBUG_U32(v)
#define BS_DEBUG_U32_LN(v) DEBUG_U32_LN(v)
#define BS_DEBUG_U16(v) DEBUG_U16(v)

#define BS_DEBUG_U16_LN(v) DEBUG_U16_LN(v)
#define BS_DEBUG_U8(v) DEBUG_U8(v)
#define BS_DEBUG_U8_LN(v) DEBUG_U8_LN(v)
#else

#define BS_DEBUG_ENDLN()
#define BS_DEBUG_BUF(b, len)
#define BS_DEBUG(s)
#define BS_DEBUG_LN(s)

#define BS_DEBUG_U32(v)
#define BS_DEBUG_U32_LN(v)
#define BS_DEBUG_U16(v)

#define BS_DEBUG_U16_LN(v)
#define BS_DEBUG_U8(v)
#define BS_DEBUG_U8_LN(v)
#endif

static Bitstream_Marker_State bs_marker_state = { 0 };

void bs_init(void) {
	memset(&bs_marker_state, 0, sizeof(bs_marker_state));
}

UF2_Block* bs_info() {
	return &(bs_marker_state.info);
}

const Bitstream_Marker_State* bs_marker_get(void) {
	return &bs_marker_state;
}

uint32_t bs_file_size(void) {
	return bs_marker_state.size;
}
uint32_t bs_uf2_file_size(void) {
	return bs_marker_state.uf2_file_size;
}

bool bs_have_checked_for_marker(void) {
	return bs_marker_state.have_checked;
}

void bs_clear_size_check_flag(void) {
	bs_marker_state.have_checked = false;
}

// pass it an array of Bitstream_Slot_Content[POSITION_SLOTS_ALLOWED]
// returns num found
uint8_t bs_slot_contents(Bitstream_Slot_Content *contents) {
	uint8_t num_found = 0;
	Bitstream_Marker_State markercheck;
	for (uint8_t i = 0; i < POSITION_SLOTS_ALLOWED; i++) {
		if (!bs_load_marker(i, &markercheck)) {

			contents[i].namelen = 0;
			contents[i].found = false;
		} else {

			num_found++;
			contents[i].found = true;
			contents[i].namelen = markercheck.namelen;
			if (markercheck.namelen) {
				memcpy(contents[i].name, markercheck.name, markercheck.namelen);
				contents[i].name[markercheck.namelen] = '\0';
			}
		}
	}
	return num_found;
}

uint32_t bs_load_marker(uint8_t slot, Bitstream_Marker_State *into) {

	BoardConfigPtrConst bc = boardconfig_get();
	board_flash_read(boardconfig_bs_marker_address_for(slot), &(into->info),
			sizeof(into->info));

	into->have_checked = true;
	into->size = 0;
	into->uf2_file_size = 0;
	if (!(into->info.magicStart0 == UF2_MAGIC_START0
			&& into->info.magicStart1 == bc->bs_marker.magic_start
			&& into->info.familyID == bc->bs_marker.family_id
			&& into->info.magicEnd == bc->bs_marker.magic_end)) {
		return 0;
	}

	BS_DEBUG("Have size info! ");
	uint8_t idx = 0;
	memcpy(&(into->size), into->info.data, 4);
	idx += 4;

	BS_DEBUG_U32(into->size);

	memcpy(&(into->start_address), &(into->info.data[idx]), 4);
	idx += 4;
	BS_DEBUG(" @ "); BS_DEBUG_U32_LN(into->start_address);

	into->namelen = into->info.data[idx];
	idx += 1;

	if (into->namelen) {
		memcpy(&(into->name), &(into->info.data[idx]), into->namelen);
		BS_DEBUG_BUF(into->name, into->namelen);
	}

	// store the "file size" we will declare such that
	// the host can download the entire UF2, basically
	// 512 bytes per block
	into->uf2_file_size = into->info.numBlocks * 512;

	return into->size; // bitstream_size;

}
uint32_t bs_check_for_marker(void) {
	BoardConfigPtrConst bc = boardconfig_get();
	Bitstream_Marker_State statecheck;

	bs_marker_state.have_checked = true;
	bs_marker_state.size = 0;
	bs_marker_state.uf2_file_size = 0;

	if (bs_load_marker(boardconfig_selected_bitstream_slot(), &statecheck)) {
		// ok we found it!
		memcpy(&bs_marker_state, &statecheck, sizeof(Bitstream_Marker_State));
		return bs_marker_state.size;
	}
	// no worky
	BS_DEBUG_LN("BAD SIZE MARK"); BS_DEBUG_U32_LN(statecheck.info.magicStart0); BS_DEBUG_U32_LN(statecheck.info.magicStart1); BS_DEBUG_U32_LN(statecheck.info.magicEnd);

	memset(&bs_marker_state.info, 0, sizeof(bs_marker_state.info));
	return 0;
}

void bs_write_marker(uint32_t num_blocks, uint32_t bitstream_size,
		uint32_t address_start, uint8_t name_len, uint8_t *name) {
	bs_write_marker_to_slot(boardconfig_bs_marker_address(), num_blocks,
			bitstream_size, address_start, name_len, name);

}

void bs_write_marker_to_slot(uint8_t slotidx, uint32_t num_blocks,
		uint32_t bitstream_size, uint32_t address_start, uint8_t name_len,
		uint8_t *name) {

	BoardConfigPtrConst bc = boardconfig_get();
	bs_marker_state.info.magicStart0 = UF2_MAGIC_START0;
	bs_marker_state.info.magicStart1 = bc->bs_marker.magic_start;
	bs_marker_state.info.magicEnd = bc->bs_marker.magic_end;
	bs_marker_state.info.familyID = bc->bs_marker.family_id;
	bs_marker_state.info.flags = UF2_FLAG_FAMILYID | UF2_FLAG_NOFLASH;
	bs_marker_state.info.blockNo = 1;
	bs_marker_state.info.numBlocks = num_blocks;
	uint32_t total_len = bitstream_size;

	uint8_t idx = 0;
	memcpy(bs_marker_state.info.data, &total_len, 4);
	idx += 4;
	memcpy(&(bs_marker_state.info.data[idx]), &address_start, 4);
	idx += 4;
	memcpy(&(bs_marker_state.info.data[idx]), &name_len, 1);
	idx += 1;
	if (name_len) {
		memcpy(&(bs_marker_state.info.data[idx]), name, name_len);
	}
	idx += name_len;

	CDCWRITEFLUSH(); BS_DEBUG("Writing bs mrk len: "); BS_DEBUG_U32(total_len); BS_DEBUG(" start: "); BS_DEBUG_U32(address_start);
	if (name_len) {
		BS_DEBUG(" for "); BS_DEBUG_BUF(name, name_len);
	} BS_DEBUG("\r\n");

	CDCWRITEFLUSH();

	board_flash_write(boardconfig_bs_marker_address_for(slotidx),
			&bs_marker_state.info, sizeof(bs_marker_state.info));
	board_flash_pages_erased_clear();
}

void bs_erase_slot(uint8_t slot) {
	Bitstream_Marker_State empty = { 0 };
	board_flash_write(boardconfig_bs_marker_address_for(slot), &empty.info,
			sizeof(bs_marker_state.info));
}

bool bs_program_fpga(bs_prog_yield_cb cb) {

	uint8_t xfer_block[FLASH_SPI_XFER_BLOCKSIZE];

	if (!bs_have_checked_for_marker()) {
		if (!bs_check_for_marker()) {
			BS_DEBUG_LN("No bitstream in flash?");
			return false;
		}
	}

	fpga_enter_programming_mode();
	uint32_t cur_addr = bs_marker_state.start_address;
	uint32_t end_addr = bs_marker_state.start_address + bs_marker_state.size;
	BS_DEBUG("FLSH prog "); BS_DEBUG_U32(cur_addr); BS_DEBUG("-"); BS_DEBUG_U32_LN(end_addr);

	uint16_t xfer_size;
	uint32_t total_xfered = 0;
#ifdef BS_DEBUG_ENABLE
	uint32_t bytes_sum = 0;
#endif
	while (cur_addr < end_addr) {
		xfer_size = FLASH_SPI_XFER_BLOCKSIZE;
		if ((cur_addr + xfer_size) >= end_addr) {
			xfer_size = (uint16_t) (end_addr - cur_addr);
		}

		if (cb != NULL) {
			cb();
		}
		if (xfer_size) {
			/*
			 BS_DEBUG_U32(cur_addr);
			 BS_DEBUG(" ");
			 BS_DEBUG_U32_LN(xfer_size);
			 cb();
			 */
			board_flash_read(cur_addr, xfer_block, xfer_size);
			fpga_spi_write(xfer_block, xfer_size);
			// FORCE_BS_DEBUG_BUF(xfer_block, xfer_size);
			// sleep_ms(FLASH_SPI_XFER_BLOCKSIZE * 2);

			cur_addr += xfer_size;
			total_xfered += xfer_size;
#ifdef BS_DEBUG_ENABLE
				for (uint16_t i=0; i<xfer_size; i++) {
					bytes_sum += xfer_block[i];
				}
			#endif

		} else {
			BS_DEBUG_LN("Uuugh ");
			cur_addr += 1;
		}

	} BS_DEBUG("Tot: "); BS_DEBUG_U32(total_xfered); BS_DEBUG_LN(" bytes"); BS_DEBUG("BS bytes sum: "); BS_DEBUG_U32_LN(bytes_sum);
	fpga_exit_programming_mode();
	fpga_set_programmed(true);
	return (total_xfered > 0);

}
