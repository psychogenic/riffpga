/*
 * io_inputs.h, part of the riffpga project
 *
 *  Created on: Jan 22, 2025
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

#ifndef IO_INPUTS_H_
#define IO_INPUTS_H_

#include "board_includes.h"
void io_irq_handler(void);

// returns the number of switches
// available
uint8_t io_switches_init(void);
bool io_switch_state(uint8_t idx);
bool io_manualclock_switch_state();
volatile bool io_manualclock_switch_interrupted();
void io_manualclock_switch_interrupt_clear();

volatile bool io_switch_interrupted(uint8_t idx);
void io_switch_interrupt_clear(uint8_t idx);




void io_inputs_init(void);
uint16_t io_inputs_value(void);



#endif /* IO_INPUTS_H_ */
