/*
 * uart_bridge.h, part of the riffpga project
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

#ifndef UART_BRIDGE_H_
#define UART_BRIDGE_H_



void uart_bridge_enable();
void uart_bridge_disable();

void uart_bridge_tx_wait_blocking();
bool uart_bridge_is_readable();
void uart_bridge_write_blocking(const uint8_t *src, size_t len);
void uart_bridge_read_blocking (uint8_t *dst, size_t len);
void uart_bridge_putc_raw(char c);
void uart_bridge_putc(char c);
void uart_bridge_puts(const char *s);
char uart_bridge_getc();
bool uart_bridge_is_writable();




#endif /* UART_BRIDGE_H_ */
