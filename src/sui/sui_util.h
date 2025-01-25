/*
 * sui_util.h, part of the riffpga project
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

#ifndef SUI_SUI_UTIL_H_
#define SUI_SUI_UTIL_H_

#include "board_config.h"
#include "board_includes.h"

typedef void(*bgwaittask)(void);
typedef int32_t(*readchar_func) (void);
typedef uint32_t(*charavail_func) (void);
typedef uint32_t(*writestring_func)(const char*s, uint32_t len);

typedef struct suiinteractionstruct {

	bgwaittask wait;
	readchar_func read;
	charavail_func avail;
	writestring_func write;
} SUIInteractionFunctions;


uint8_t sui_read_string(readchar_func rd, charavail_func avail, bgwaittask waittask, char * buffer, uint8_t maxlen);

uint32_t sui_read_integer(readchar_func rd, charavail_func avail, bgwaittask waittask);

uint32_t sui_prompt_for_integer(const char * prompt, uint32_t len,
		writestring_func wr, readchar_func rd, charavail_func avail, bgwaittask waittask);


#endif /* SUI_SUI_UTIL_H_ */
