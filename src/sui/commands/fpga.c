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
#include "bitstream.h"


void cmd_fpga_reset(SUIInteractionFunctions * funcs) {
	CDCWRITESTRING("\r\n Toggle FPGA reset, now: ");
	if (fpga_is_in_reset() == false) {
		fpga_reset(true);
		CDCWRITESTRING("in RESET.");
	} else {
		fpga_reset(false);
		CDCWRITESTRING("enabled (not reset).");
	}

}
void cmd_fpga_prog(SUIInteractionFunctions * funcs) {

	/*CDCWRITESTRING("\r\nDEBUG SPI PINS\r\n");
	fpga_debug_spi_pins();

	return;
	*/

	CDCWRITESTRING("\r\nProgramming FPGA...");
	if (bs_program_fpga(funcs->wait) == false) {
		CDCWRITESTRING("Failed to prog?\r\n");
	} else {
		CDCWRITESTRING("done!\r\n");
	}

}
