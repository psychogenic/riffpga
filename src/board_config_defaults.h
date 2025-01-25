/*
 * board_config_defaults.h, part of the riffpga project
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

#ifndef BOARD_CONFIG_DEFAULTS_H_
#define BOARD_CONFIG_DEFAULTS_H_


#ifdef TARGET_GENERIC
#include "config_defaults/generic.h"
#elif defined(TARGET_EFABLESS_EXPLAIN)
#include "config_defaults/efab_explain.h"
#elif defined(TARGET_PSYDMI)
#include "config_defaults/psydmi.h"
#else
#error "provide a TARGET_* for config defaults"
#endif


#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN	PIN_RP_LED
#endif

#ifndef CFG_TUD_MAX_SPEED
#define CFG_TUD_MAX_SPEED
#endif
#endif /* BOARD_CONFIG_DEFAULTS_H_ */
