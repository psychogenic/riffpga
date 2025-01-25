/*
 * sui_handler.c, part of the riffpga project
 *
 *  Created on: Jan 6, 2025
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

#include "sui_handler.h"
#include "sui_command.h"
#include "cdc_interface.h"
#include "debug.h"


void sui_handle_request(writestring_func wr, readchar_func rd, charavail_func avail, bgwaittask waittask) {
	char input_buffer[SUI_COMMAND_MAXLEN+1];

	SUIInteractionFunctions funcs = {
			.wait = waittask,
			.read = rd,
			.write = wr,
			.avail = avail

	};

	if (! avail() ) {
		return;
	}

	uint8_t numchars = sui_read_string(rd, avail, waittask, input_buffer, SUI_COMMAND_MAXLEN);
	if (! numchars ) {
		return;
	}


	CommandInfo * cmd = sui_command_by_name(input_buffer);
	if (cmd != NULL) {
		// CDCWRITESTRING("Got command:");
		// cdc_write(input_buffer, numchars);
		cmd->cb(&funcs);
		CDCWRITESTRING("\r\n> ");
		CDCWRITEFLUSH();
	} else {
		/*
		CDCWRITESTRING("Unknown cmd: ");
		cdc_write(input_buffer, numchars);
		sui_command_show_help();
		*/
	}
}

