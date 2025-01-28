/*
 * sui_command.h, part of the riffpga project
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

#ifndef SUI_SUI_COMMAND_H_
#define SUI_SUI_COMMAND_H_

#include "sui_util.h"

typedef void(*commandcallback)(SUIInteractionFunctions * funcs);

typedef struct commandinfostruct {
	const char * command;
	const char * help;
	const char hotkey;
	bool needs_confirmation;
	commandcallback cb;
} CommandInfo;

void sui_command_show_help(SUIInteractionFunctions * funcs);

CommandInfo * sui_command_by_name(const char * cmd);



#endif /* SUI_SUI_COMMAND_H_ */
