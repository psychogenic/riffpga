/*
 * uart_bridge.c, part of the riffpga project
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

#include "board_config.h"
#include "uart_bridge.h"

typedef struct uartbridgestatestruct {
	bool is_init;
	uart_inst_t * uart;

} UartBridgeState;

static UartBridgeState ubridgestate = {0};

void uart_bridge_enable() {
	// bconf->uart_bridge.

	if (ubridgestate.is_init) {
		return;
	}

	BoardConfigPtrConst bc = boardconfig_get();
	if (bc->uart_bridge.uartnum == 0) {
		ubridgestate.uart = uart0;
	} else {
		ubridgestate.uart = uart1;
	}


	gpio_set_function(bc->uart_bridge.pin_tx,
						UART_FUNCSEL_NUM(ubridgestate.uart, bc->uart_bridge.pin_tx));

	gpio_set_function(bc->uart_bridge.pin_rx,
						UART_FUNCSEL_NUM(ubridgestate.uart, bc->uart_bridge.pin_rx));
	uart_init(ubridgestate.uart, bc->uart_bridge.baud);



	ubridgestate.is_init = true;
}
void uart_bridge_disable() {

	if (! ubridgestate.is_init ) {
		return ;
	}

	uart_deinit(ubridgestate.uart);
	ubridgestate.is_init = false;

}


bool uart_bridge_is_writable() {


	if (! ubridgestate.is_init ) {
		return false ;
	}

	return uart_is_writable(ubridgestate.uart);
}
void uart_bridge_tx_wait_blocking() {
	if (! ubridgestate.is_init ) {
		return ;
	}

	uart_tx_wait_blocking(ubridgestate.uart);

}
bool uart_bridge_is_readable() {
	if (! ubridgestate.is_init ) {
		return false ;
	}
	return uart_is_readable(ubridgestate.uart);

}
void uart_bridge_write_blocking(const uint8_t *src, size_t len) {
	if (! ubridgestate.is_init ) {
		return ;
	}

	uart_write_blocking(ubridgestate.uart, src, len);

}
void uart_bridge_read_blocking (uint8_t *dst, size_t len) {
	if (! ubridgestate.is_init ) {
		return ;
	}
	uart_read_blocking(ubridgestate.uart, dst, len);
}
void uart_bridge_putc_raw(char c) {
	if (! ubridgestate.is_init ) {
		return ;
	}
	uart_putc_raw(ubridgestate.uart, c);

}
void uart_bridge_putc(char c) {
	if (! ubridgestate.is_init ) {
		return ;
	}
	uart_putc(ubridgestate.uart, c);

}
void uart_bridge_puts(const char *s) {
	if (! ubridgestate.is_init ) {
		return ;
	}
	uart_puts(ubridgestate.uart, s);

}
char uart_bridge_getc() {
	if (! ubridgestate.is_init ) {
		return 0;
	}
	return uart_getc(ubridgestate.uart);

}
