/*
 * clocking.h, part of the riffpga project
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

#ifndef SUI_COMMANDS_CLOCKING_H_
#define SUI_COMMANDS_CLOCKING_H_

#include "sui/sui_util.h"

void cmd_set_sys_clock_hz(SUIInteractionFunctions * funcs);
void cmd_set_autoclock_hz(SUIInteractionFunctions * funcs);
void cmd_set_autoclock_manual(SUIInteractionFunctions * funcs);

#endif /* SUI_COMMANDS_CLOCKING_H_ */

