/*
 * clocking.c, part of the riffpga project
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

#include "sui/commands/clocking.h"

#include "board_config.h"
#include "cdc_interface.h"
#include "bitstream.h"
#include "driver_state.h"

void cmd_set_sys_clock_hz(SUIInteractionFunctions *funcs) {
	const char *prompt = "\r\nEnter value [Hz]: ";
	CDCWRITESTRING("Sys Clock now: ");
	cdc_write_dec_u32_ln(clock_get_hz(clk_sys));
	CDCWRITEFLUSH();

	uint32_t setting = sui_prompt_for_integer(prompt, strlen(prompt),
			funcs->write, funcs->read, funcs->avail, funcs->wait);

	if (setting > 1000) {
		boardconfig_set_systemclock_hz(setting);
	}
}

void cmd_set_autoclock_hz(SUIInteractionFunctions *funcs) {
	const char *prompt = "\r\nEnter value [Hz]: ";
	CDCWRITESTRING("Auto clock now: ");

	BoardConfigPtrConst bc = boardconfig_get();

	if (bc->clocking[0].enabled) {
		CDCWRITESTRING("ENABLED @ ");
		cdc_write_dec_u32(bc->clocking[0].freq_hz);
		CDCWRITESTRING(" Hz");

	} else {
		CDCWRITESTRING("DISABLED ");
	}

	uint32_t setting = sui_prompt_for_integer(prompt, strlen(prompt),
			funcs->write, funcs->read, funcs->avail, funcs->wait);

	if (setting < 10) {
		CDCWRITESTRING("Values < 10, use manual.\r\n");
	} else {
		boardconfig_set_autoclock_hz(setting);
		CDCWRITESTRING("Setting auto-clocking\r\n");

	}
	/* this isn't working quite right...
	CDCWRITEFLUSH();
	CDCWRITESTRING("Achieved: ");
	cdc_write_dec_u32( (uint32_t)boardconfig_autoclocking_achieved(0));
	CDCWRITESTRING(" Hz\r\n");
	*/

}
void cmd_set_autoclock_manual(SUIInteractionFunctions *funcs) {

	CDCWRITESTRING("Auto clock was: ");

	BoardConfigPtrConst bc = boardconfig_get();

	if (bc->clocking[0].enabled) {
		CDCWRITESTRING("ENABLED @ ");
		cdc_write_dec_u32(bc->clocking[0].freq_hz);
		CDCWRITESTRING(" Hz.  Disabling.\r\n");
		boardconfig_autoclock_disable();

	} else {
		CDCWRITESTRING("already disabled.\r\n");
	}

	CDCWRITESTRING("Now clocking manually.\r\n");
	MainDriverState.clocking_manually = true;
}
