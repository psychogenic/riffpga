/*
 * fpga.c, part of the riffpga project
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

#include "sui/commands/fpga.h"
#include "cdc_interface.h"
#include "board_config.h"
#include "../../fpga.h"
#include "../../board.h"
#include "bitstream.h"

void cmd_fpga_erase(SUIInteractionFunctions *funcs) {

	CDCWRITESTRING("\r\n Erasing FPGA bitstreams\r\n");
	for (uint8_t i = 0; i < POSITION_SLOTS_NUM; i++) {
		bs_erase_slot(i);
	}

	board_flash_pages_erased_clear();
	bs_init();
	fpga_reset(true);

}

void cmd_fpga_reset(SUIInteractionFunctions *funcs) {
	CDCWRITESTRING("\r\n Toggle FPGA reset, now: ");
	if (fpga_is_in_reset() == false) {
		fpga_reset(true);
		CDCWRITESTRING("in RESET.");
	} else {
		fpga_reset(false);
		CDCWRITESTRING("enabled (not reset).");
	}

}
void cmd_fpga_prog(SUIInteractionFunctions *funcs) {

	BoardConfigPtrConst bc = boardconfig_get();

	CDCWRITESTRING("\r\nProgramming FPGA...");
	if (bs_program_fpga(funcs->wait) == false) {
		CDCWRITESTRING("Failed to prog?\r\n");
	} else {
		CDCWRITESTRING("done!\r\n");
	}

	CDCWRITESTRING("cdone is: ");
#ifdef FPGA_PROG_DONE_LEVEL
	if (gpio_get(bc->fpga_cram.pin_done)) {
		CDCWRITESTRING("HIGH\r\n");
	} else {
		CDCWRITESTRING("LOW\r\n");
	}
#else
	CDCWRITESTRING("n/a\r\n");
#endif

}
