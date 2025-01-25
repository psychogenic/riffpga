/*
 * io_inputs.c, part of the riffpga project
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


#include "io_inputs.h"
#include "board_config.h"
#include "debug.h"


void io_inputs_init(void) {

	BoardConfigPtrConst bconf = boardconfig_get();

	if (! bconf->system.num_inputs) {

		return;
	}

	for (uint8_t i; i<bconf->system.num_inputs; i++) {
		gpio_init(bconf->system.input_io[i]);
		gpio_set_dir(bconf->system.input_io[i], GPIO_IN);
		gpio_pull_up(bconf->system.input_io[i]);
	}


}
uint16_t io_inputs_value(void) {
	uint16_t retVal = 0;
	BoardConfigPtrConst bconf = boardconfig_get();

	if (! bconf->system.num_inputs) {
		DEBUG_LN("No inputs configured!");
		return 0;
	}

	for (uint8_t i; i<bconf->system.num_inputs; i++) {
		if (gpio_get(bconf->system.input_io[i])) {
			retVal |= ( 1 << (i));
		}

	}

	return retVal;

}
