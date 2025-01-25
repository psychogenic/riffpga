/*
 * sui_handler.h, part of the riffpga project
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

#ifndef SUI_SUI_HANDLER_H_
#define SUI_SUI_HANDLER_H_
#include "sui_util.h"
#include "sui_command.h"

#define SUI_COMMAND_MAXLEN	30
void sui_handle_request(writestring_func wr, readchar_func rd, charavail_func avail, bgwaittask waittask);



#endif /* SUI_SUI_HANDLER_H_ */
