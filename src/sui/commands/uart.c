/*
 * uart.c, part of the riffpga project
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

#include "sui/commands/uart.h"

#include "board_config.h"
#include "cdc_interface.h"
#include "bitstream.h"
#include "uart_bridge.h"

void cmd_uartbridge_enable(SUIInteractionFunctions *funcs) {
	CDCWRITESTRING("\r\nEnabling UART bridge\r\n");
	uart_bridge_enable();
	boardconfig_uartbridge_enable();
}

void cmd_uartbridge_baudrate(SUIInteractionFunctions *funcs) {
	const char *prompt = "\r\nEnter value [baud]: ";

	BoardConfigPtrConst bc = boardconfig_get();

	CDCWRITESTRING("\r\nUART bridge baudrate now: ");
	cdc_write_dec_u32_ln(bc->uart_bridge.baud);
	uint32_t setting = sui_prompt_for_integer(prompt, strlen(prompt),
			funcs->write, funcs->read, funcs->avail, funcs->wait);
	if (setting < 9600) {
		CDCWRITESTRING("\r\ncancelled.");
	} else {
		boardconfig_set_uartbridge_baudrate(setting);
		CDCWRITESTRING("\r\nBaud rate set to ");
		cdc_write_dec_u32_ln(setting);
	}
}
