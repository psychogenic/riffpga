/*
 * rp_system.c, part of the ASICSim2 project
 *
 *  Created on: Jan 30, 2025
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

#include "sui/commands/rp_system.h"
#include "cdc_interface.h"
#include "board.h"
#include "debug.h"

void cmd_rp2_reboot(SUIInteractionFunctions *funcs) {
	CDCWRITESTRING("\r\nRebooting!  Virtual drive will dismount\r\n");
	funcs->wait();
	sleep_ms(250);
	CDCWRITEFLUSH();
	funcs->wait();
	board_reboot();
}

