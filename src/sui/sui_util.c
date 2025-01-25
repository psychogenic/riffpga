/*
 * sui_util.c, part of the riffpga project
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

#include "board_includes.h"
#include "sui_util.h"
#include "debug.h"
#include "cdc_interface.h"

uint32_t sui_prompt_for_integer(const char * prompt, uint32_t len,
		writestring_func wr, readchar_func rd, charavail_func avail, bgwaittask waittask) {
	wr(prompt, len);
	return sui_read_integer(rd, avail, waittask);

}


uint8_t sui_read_string(readchar_func rd, charavail_func avail, bgwaittask waittask, char * buffer, uint8_t maxlen) {

	bool have_val = false;
	uint8_t charcount = 0;
	while ((have_val == false) && (charcount < maxlen)) {
		if (! avail() ) {
			waittask();
		} else {
			char c = rd();
			if (c == 0x08 /* backspace */ ){
				if (charcount) {
					charcount -= 1;
				}
			} else if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c < ' ' || c > 'z') {
				have_val = true;
				cdc_write("\r\n", 2);
				CDCWRITEFLUSH();

			} else {
				buffer[charcount] = c;
				cdc_write(&c, 1);
				CDCWRITEFLUSH();
				charcount++;
			}
		}
	}
	buffer[charcount] = '\0';

	return charcount;

}

uint32_t sui_read_integer(readchar_func rd, charavail_func avail, bgwaittask waittask) {

	bool have_val = false;
	uint32_t v = 0;
	uint8_t digits[11] = {0};
	uint8_t dig_count = 0;
	while (have_val == false) {
		while (! avail() ) {
			waittask();
		}
		char c = rd();
		if (c == 0x08 /* backspace */ ){
			if (dig_count) {
				dig_count -= 1;
			}
		} else if (c < '0' || c > '9') {
			have_val = true;
		} else {
			digits[dig_count] = c - '0';
			dig_count++;
		}
		cdc_write(&c, 1);
		CDCWRITEFLUSH();
		// tud_cdc_write_flush();
	}
	for (uint8_t i=0 ; i<dig_count; i++) {
		v = v * 10;
		v += digits[i];
	}
	// DEBUG("\r\n\r\nGOT VAL: ");
	// DEBUG_U32_LN(v);

	return v;
}
