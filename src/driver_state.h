/*
 * driver_state.h, part of the ASICSim2 project
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

#ifndef DRIVER_STATE_H_
#define DRIVER_STATE_H_

#include "board_includes.h"

typedef struct driverstatestruct {

	uint32_t blink_interval_ms;
	bool have_programmed;
	bool immediate_led_blink;
	bool clocking_manually;
} DriverState;

extern DriverState MainDriverState;




#endif /* DRIVER_STATE_H_ */
