/*
 * sui_command.c, part of the riffpga project
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

#include "cdc_interface.h"
#include "sui_command.h"
#include "sui/commands/clocking.h"
#include "sui/commands/config.h"
#include "sui/commands/dump.h"
#include "sui/commands/fpga.h"
#include "sui/commands/io.h"
#include "sui/commands/uart.h"



/*
 * Commands available in minishell
 *
 * Each command has
 *   - a name, to trigger it
 *   - description (help), displayed in help
 *   - hotkey, a single KEY to trigger
 *   - a callback (cb)
 *
 */



static CommandInfo commands[] = {


		{
				.command = "slot",
				.help = "FPGA bitstream slot",
				.hotkey = 'S',
				.cb = cmd_select_active_slot
		},
		{
				.command = "projclock",
				.help = "Project auto-clock Hz",
				.hotkey = 'A',
				.cb = cmd_set_autoclock_hz
		},
		{
				.command = "manualclock",
				.help = "Stop auto-clocking",
				.hotkey = 'M',
				.cb = cmd_set_autoclock_manual
		},
		{
				.command = "fpgareset",
				.help = "Toggle FPGA reset",
				.hotkey = 'R',
				.cb = cmd_fpga_reset
		},
		{
				.command = "fpgaprogram",
				.help = "Program FPGA",
				.hotkey = 'P',
				.cb = cmd_fpga_prog
		},
		{
				.command = "sysclock",
				.help = "System clock Hz",
				.hotkey = 'C',
				.cb = cmd_set_sys_clock_hz
		},
		{
				.command = "uartbridge",
				.help = "Enable UART bridge",
				.hotkey = 'U',
				.cb = cmd_uartbridge_enable
		},
		{
				.command = "baudrate",
				.help = "UART bridge baudrate",
				.hotkey = 'B',
				.cb = cmd_uartbridge_baudrate
		},
#if SYSTEM_INPUTS_NUM > 0
		{
				.command = "readinputs",
				.help = "Read inputs",
				.hotkey = 'I',
				.cb = cmd_read_io_inputs
		},
#endif

		{
				.command = "dumpstate",
				.help = "Dump current state",
				.hotkey = 'D',
				.cb = cmd_dump_state
		},
		{
				.command = "rawconf",
				.help = "Dump config, raw bytes",
				.hotkey = 'W',
				.cb = cmd_dump_raw_config
		},

		{
				.command = "save",
				.help = "Save current configuration",
				.hotkey = 'V',
				.cb = cmd_save_config
		},
		{
				.command = "factoryreset",
				.help = "Factory reset configuration",
				.hotkey = 'F',
				.cb = cmd_factory_reset_config
		},
		{
				.command = "?",
				.help = "show this help",
				.hotkey = 'H',
				.cb = sui_command_show_help
		},
		// list terminator
		{
				.command = NULL,
				.help = NULL,
				.cb = NULL
		}

};



void sui_command_show_help(SUIInteractionFunctions * funcs) {
	uint8_t i = 0;
	const char * starsep =    " *************************************************************\r\n";
	const char * starspacer = " *                                                           *\r\n";
	BoardConfigPtrConst bc = boardconfig_get();
	char buf[2];
	buf[1] = '\0';

	size_t namelen = strlen(bc->board_name);
	uint8_t namespaces_prefix = (59 - namelen)/2;


	CDCWRITESTRING("\r\n\r\n");
	CDCWRITESTRING(starsep);
	CDCWRITESTRING(" *");
	for (uint8_t i=0; i<namespaces_prefix; i++) {
		CDCWRITECHAR(" ");
		funcs->wait();
	}
	CDCWRITESTRING(bc->board_name);
	for (uint8_t i=(namespaces_prefix + namelen); i<59; i++) {

		CDCWRITECHAR(" ");
		funcs->wait();
	}
	CDCWRITESTRING("*\r\n");

	        CDCWRITESTRING(" *                === Available Commands ===                 *\r\n");
	        CDCWRITESTRING(starspacer);
			CDCWRITEFLUSH();
			funcs->wait();
	        CDCWRITESTRING(" *    command         key                                    *\r\n");
	while (commands[i].command != NULL) {
		buf[0] = commands[i].hotkey;

		CDCWRITESTRING(" * ");
		CDCWRITESTRING(commands[i].command);

		size_t cmdlen = strlen(commands[i].command);
		size_t helplen = strlen(commands[i].help);

		if (cmdlen < 19) {
			for (uint8_t i=0; i< (19-cmdlen); i++) {
				CDCWRITECHAR(" ");
			}
		}

		CDCWRITECHAR("[");
		CDCWRITECHAR(buf);
		CDCWRITESTRING("]    ");
		CDCWRITESTRING(commands[i].help);

		CDCWRITEFLUSH();
		if (helplen+20+3 < 55) {

			for (uint8_t i=0; i<(55 - (helplen + 20 + 3)); i++) {
				CDCWRITECHAR(" ");
			}
		}

		CDCWRITESTRING("*\r\n");
		funcs->wait();
		CDCWRITEFLUSH();
		i++;

	}

	CDCWRITESTRING(starspacer);
    CDCWRITESTRING(" *                      RifFPGA v" );
    cdc_write_dec_u8(bc->version.major);
    CDCWRITECHAR(".");
    cdc_write_dec_u8(bc->version.minor);
    CDCWRITECHAR(".");
    cdc_write_dec_u8(bc->version.patchlevel);
    CDCWRITESTRING("                       *\r\n");

	funcs->wait();
	CDCWRITEFLUSH();

    CDCWRITESTRING(" *                   (C) 2025 Pat Deegan                     *\r\n");
	CDCWRITESTRING(starspacer);
	funcs->wait();
	CDCWRITEFLUSH();
	funcs->wait();

    CDCWRITESTRING(" *        Enter command (or key) to trigger function.        *\r\n");
	funcs->wait();
	CDCWRITESTRING(starspacer);
	funcs->wait();
	CDCWRITESTRING(starsep);
	funcs->wait();


}

CommandInfo * sui_command_by_name(const char * cmd) {
	uint8_t i = 0;
	static size_t command_max_len = 0;

	if (command_max_len == 0) {
		// max len unknown, figure it out
		while (commands[i].command != NULL) {
			size_t cmdlen = strlen(commands[i].command);
			if ( cmdlen > command_max_len) {
				command_max_len = cmdlen;
			}
			i++;
		}

		i = 0; // reset
	}

	size_t inlen = strlen(cmd);
	size_t complen = inlen;
	if (complen > command_max_len) {
		complen = command_max_len;
	}

	while (commands[i].command != NULL) {
		if (strncmp(commands[i].command, cmd, complen) == 0) {
			return &(commands[i]);
		}
		if (commands[i].hotkey == cmd[0]) {
			return &(commands[i]);
		}
		i++;
	}
	return NULL;

}




