/*
 * config.c, part of the riffpga project
 *
 *  Created on: Jan 23, 2025
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

#include "sui/commands/config.h"
#include "cdc_interface.h"
#include "bitstream.h"
#include "sui/commands/dump.h"
#include "sui/commands/fpga.h"
#include "../../fpga.h"

void cmd_factory_reset_config(SUIInteractionFunctions *funcs) {
	CDCWRITESTRING("\r\nConfiguration Factory Reset!\r\n");
	boardconfig_factoryreset();
	cmd_dump_state(funcs);
}
void cmd_save_config(SUIInteractionFunctions *funcs) {
	boardconfig_write();
	CDCWRITESTRING("\r\nConfiguration saved.\r\n");
	cmd_dump_state(funcs);
}

void cmd_select_active_slot(SUIInteractionFunctions *funcs) {

	Bitstream_Slot_Content slot_contents[POSITION_SLOTS_ALLOWED];
	uint8_t slot_programmed[POSITION_SLOTS_ALLOWED] = { 0 };
	uint8_t num_found = bs_slot_contents(slot_contents);

	CDCWRITESTRING("\r\n");
	for (uint8_t i = 0; i < POSITION_SLOTS_ALLOWED; i++) {
		CDCWRITESTRING("  * ");
		cdc_write_dec_u8((i + 1));
		CDCWRITESTRING(": ");
		if (slot_contents[i].found == true) {
			if (slot_contents[i].namelen) {
				cdc_write(slot_contents[i].name, slot_contents[i].namelen);
				CDCWRITESTRING("\r\n");
				slot_programmed[i] = 1;
			} else {
				CDCWRITESTRING("-unnammed-\r\n");
			}
		} else {
			CDCWRITESTRING("-empty-\r\n");

		}
		funcs->wait();
	}
	CDCWRITEFLUSH();
	if (!num_found) {
		CDCWRITESTRING("\r\nNo programmed slots, aborting.\r\n");
		return;
	}

	const char *prompt = "\r\nEnter slot to use [1-3]: ";
	//BoardConfigPtrConst bc = boardconfig_get();
	const Bitstream_Marker_State *bsmarkorig = bs_marker_get();

	CDCWRITESTRING("\r\nCurrent active slot is ");
	cdc_write_dec_u8(boardconfig_selected_bitstream_slot() + 1);

	if (bsmarkorig->size) {
		CDCWRITESTRING(", have config of size ");
		cdc_write_dec_u32(bsmarkorig->size);
		CDCWRITESTRING(" @ 0x");
		cdc_write_u32_ln(bsmarkorig->start_address);
	} else {

		CDCWRITESTRING(", but no valid stream present.\r\n");
	}

	uint32_t slot_sel = sui_prompt_for_integer(prompt, strlen(prompt),
			funcs->write, funcs->read, funcs->avail, funcs->wait);

	if (slot_sel < 1 || slot_sel > POSITION_SLOTS_ALLOWED) {
		CDCWRITESTRING("\r\nInvalid slot, skipping\r\n");
		return;
	}

	uint8_t slotidx = (uint8_t) slot_sel - 1;

	if (!slot_programmed[slotidx]) {
		CDCWRITESTRING("\r\nHave selected an empty slot.\r\n");
	}

	boardconfig_set_bitstream_slot(slotidx);
	CDCWRITESTRING("Slot ");
	cdc_write_dec_u8(slotidx + 1);
	CDCWRITESTRING(" activated (marker @ 0x");
	cdc_write_u32(boardconfig_bs_marker_address_for(slotidx));
	CDCWRITESTRING(")\r\n\r\n");
	boardconfig_write();
	bs_clear_size_check_flag();

	uint32_t bs_size = bs_check_for_marker();
	if (bs_size) {

		const Bitstream_Marker_State *bsmark = bs_marker_get();

		CDCWRITESTRING("Found bitstream of size ");
		cdc_write_dec_u32(bs_size);
		CDCWRITESTRING(" in slot, located at 0x");
		cdc_write_u32_ln(bsmark->start_address);
		cmd_fpga_prog(funcs);
	} else {
		CDCWRITESTRING("No bitstream present in slot.\r\n");
		fpga_reset(true);
	}

}

