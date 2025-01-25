/*
 * io.c, part of the riffpga project
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

#include "sui/commands/io.h"
#include "cdc_interface.h"
#include "board_config.h"
#include "io_inputs.h"

void cmd_read_io_inputs(SUIInteractionFunctions * funcs) {
	uint16_t readVal = io_inputs_value();
	CDCWRITESTRING("\r\nInputs currently: ");
	cdc_write_dec_u16(readVal);
	CDCWRITESTRING(", 0x");
	cdc_write_u16_ln(readVal);

}

