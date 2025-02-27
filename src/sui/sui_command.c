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
#include "debug.h"
#include "sui/commands/clocking.h"
#include "sui/commands/config.h"
#include "sui/commands/dump.h"
#include "sui/commands/fpga.h"
#include "sui/commands/io.h"
#include "sui/commands/uart.h"
#include "sui/commands/rp_system.h"



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


/*
 * List of available commands.  To add a command:
 *
 *  - add a CommandInfo entry in this list, somewhere
 *    before the last entry
 *
 *  - the .cb member will be a callback, the actual command
 *    that is run.
 *
 *  - implement that callback and do whatever you want to do
 *
 * Note: try to have a command who's first letters don't conflict
 * so you can use them easily with shortened strings, e.g.
 * 'uar' rather than having to type 'uartbridge'
 * Also, don't conflict with the hotkey
 */
static CommandInfo commands[] = {
		{
				.command = "slot",
				.help = "Select FPGA bitstream slot",
				.hotkey = 'S',
				.needs_confirmation = false,
				.cb = cmd_select_active_slot
		},
		{
				.command = "projclock",
				.help = "Project Auto-Clocking",
				.hotkey = 'A',
				.needs_confirmation = false,
				.cb = cmd_set_autoclock_hz
		},
		{
				.command = "manualclock",
				.help = "Stop Auto-Clocking",
				.hotkey = 'M',
				.needs_confirmation = true,
				.cb = cmd_set_autoclock_manual
		},
		{
				.command = "fpgareset",
				.help = "Toggle FPGA reset",
				.hotkey = 'R',
				.needs_confirmation = true,
				.cb = cmd_fpga_reset
		},
		{
				.command = "fpgaprogram",
				.help = "Program FPGA",
				.hotkey = 'P',
				.needs_confirmation = false,
				.cb = cmd_fpga_prog
		},

		{
				.command = "fpgaerase",
				.help = "Erase all FPGA slots and reset",
				.hotkey = 'E',
				.needs_confirmation = true,
				.cb = cmd_fpga_erase
		},

		{
				.command = "sysclock",
				.help = "RP2 System Clock",
				.hotkey = 'C',
				.needs_confirmation = false,
				.cb = cmd_set_sys_clock_hz
		},
		{
				.command = "uartbridge",
				.help = "Enable UART bridge",
				.hotkey = 'U',
				.needs_confirmation = true,
				.cb = cmd_uartbridge_enable
		},
		{
				.command = "baudrate",
				.help = "UART bridge baudrate",
				.hotkey = 'B',
				.needs_confirmation = false,
				.cb = cmd_uartbridge_baudrate
		},
#if SYSTEM_INPUTS_NUM > 0
		{
				.command = "readinputs",
				.help = "Read Inputs",
				.hotkey = 'I',
				.needs_confirmation = false,
				.cb = cmd_read_io_inputs
		},
#endif

		{
				.command = "dumpstate",
				.help = "Dump current state",
				.hotkey = 'D',
				.needs_confirmation = false,
				.cb = cmd_dump_state
		},

		{
				.command = "save",
				.help = "Save current configuration",
				.hotkey = 'V',
				.needs_confirmation = true,
				.cb = cmd_save_config
		},
		{
				.command = "factoryreset",
				.help = "Factory reset configuration",
				.hotkey = 'F',
				.needs_confirmation = true,
				.cb = cmd_factory_reset_config
		},
#ifdef DEBUG_OUTPUT_ENABLED
		{
				.command = "rawconf",
				.help = "Dump config, raw bytes",
				.hotkey = 'W',
				.needs_confirmation = false,
				.cb = cmd_dump_raw_config
		},
		{
				.command = "rawslot",
				.help = "Dump slot conf, raw bytes",
				.hotkey = 'Y',
				.needs_confirmation = false,
				.cb = cmd_dump_raw_slot
		},

#endif
		{
				.command = "reboot",
				.help = "Reboot the RP2 chip",
				.hotkey = 'T',
				.needs_confirmation = true,
				.cb = cmd_rp2_reboot
		},

		{
				.command = "?",
				.help = "Show this help",
				.hotkey = 'H',
				.needs_confirmation = false,
				.cb = sui_command_show_help
		},
		// list terminator
		{
				.command = NULL,
				.help = NULL,
				.cb = NULL
		}

};

CommandInfo* sui_command_by_name(const char *cmd) {
	uint8_t i = 0;
	static size_t command_max_len = 0;

	if (command_max_len == 0) {
		// max len unknown, figure it out
		while (commands[i].command != NULL) {
			size_t cmdlen = strlen(commands[i].command);
			if (cmdlen > command_max_len) {
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
		if (
		// only check cmd name substring for entered strings that have more than one
		// char--except in the case of commands that are 1 char long.
		(inlen > 1 || strlen(commands[i].command) == 1)
				&& strncmp(commands[i].command, cmd, complen) == 0) {
			return &(commands[i]);
		}
		if (inlen == 1 && (commands[i].hotkey == cmd[0])) {
			return &(commands[i]);
		}
		i++;
	}
	return NULL;

}

void sui_command_show_help(SUIInteractionFunctions *funcs) {
	uint8_t i = 0;
	const char *starsep =
			" *************************************************************\r\n";
	const char *starspacer =
			" *                                                           *\r\n";
	BoardConfigPtrConst bc = boardconfig_get();
	// char buf[2];
	// buf[1] = '\0';

	size_t namelen = strlen(bc->board_name);
	uint8_t namespaces_prefix = (59 - namelen) / 2;

	CDCWRITESTRING("\r\n\r\n");
	CDCWRITESTRING(starsep);
	CDCWRITESTRING(" *");
	for (uint8_t i = 0; i < namespaces_prefix; i++) {
		CDCWRITECHAR(' ');
		funcs->wait();
	}
	CDCWRITESTRING(bc->board_name);
	for (uint8_t i = (namespaces_prefix + namelen); i < 59; i++) {

		CDCWRITECHAR(' ');
		funcs->wait();
	}
	CDCWRITESTRING("*\r\n");

	CDCWRITESTRING(
			" *                === Available Commands ===                 *\r\n");
	CDCWRITESTRING(starspacer);
	CDCWRITEFLUSH();
	funcs->wait();
	CDCWRITESTRING(
			" *     command        key                                    *\r\n");
	while (commands[i].command != NULL) {
		// buf[0] = commands[i].hotkey;

		CDCWRITESTRING(" * ");
		CDCWRITESTRING(commands[i].command);

		size_t cmdlen = strlen(commands[i].command);
		size_t helplen = strlen(commands[i].help);

		if (cmdlen < 19) {
			for (uint8_t i = 0; i < (19 - cmdlen); i++) {
				CDCWRITECHAR(' ');
			}
		}

		CDCWRITECHAR('[');
		CDCWRITECHAR(commands[i].hotkey);
		CDCWRITESTRING("]    ");
		CDCWRITESTRING(commands[i].help);

		CDCWRITEFLUSH();
		if (helplen + 20 + 3 < 55) {

			for (uint8_t i = 0; i < (55 - (helplen + 20 + 3)); i++) {
				CDCWRITECHAR(' ');
			}
		}

		CDCWRITESTRING("*\r\n");
		funcs->wait();
		CDCWRITEFLUSH();
		i++;

	}

	CDCWRITESTRING(starspacer);
	CDCWRITESTRING(" *                      RifFPGA v");
	cdc_write_dec_u8(bc->version.major);
	CDCWRITECHAR('.');
	cdc_write_dec_u8(bc->version.minor);
	CDCWRITECHAR('.');
	cdc_write_dec_u8(bc->version.patchlevel);
	CDCWRITESTRING("                       *\r\n");

	funcs->wait();
	CDCWRITEFLUSH();

	CDCWRITESTRING(
			" *                   (C) 2025 Pat Deegan                     *\r\n");
	CDCWRITESTRING(starspacer);
	funcs->wait();
	CDCWRITEFLUSH();
	funcs->wait();

	CDCWRITESTRING(
			" *        Enter command (or key) to trigger function.        *\r\n");
	funcs->wait();
	CDCWRITESTRING(starspacer);
	funcs->wait();
	CDCWRITESTRING(starsep);
	funcs->wait();

}



