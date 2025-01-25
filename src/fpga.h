/*
 * fpga.h, part of the riffpga project
 *
 *  Created on: Dec 18, 2024
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

#ifndef SRC_FPGA_H_
#define SRC_FPGA_H_

#include "board_includes.h"

void fpga_init(void);
bool fpga_is_init(void);

bool fpga_is_in_reset(void);

volatile bool fpga_external_reset(void);
void fpga_external_reset_handled(void);



void fpga_debug_spi_pins(void);

void fpga_enter_programming_mode(void);
void fpga_exit_programming_mode(void);

bool fpga_is_programmed(void);
void fpga_set_programmed(bool set_to);
void fpga_reset(bool set_to); /* true==in reset */
bool fpga_in_reset(void);

void fpga_spi_transaction_begin(void);
void fpga_spi_transaction_end(void);
void fpga_spi_write(uint8_t * bts, size_t len);



#endif /* SRC_FPGA_H_ */
