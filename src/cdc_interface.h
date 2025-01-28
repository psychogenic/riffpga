/*
 * cdc_interface.h, part of the riffpga project
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

#ifndef CDC_INTERFACE_H
#define CDC_INTERFACE_H 1

#include "board_includes.h"

#define CDCWRITESTRING(s)	cdc_write(s, strlen(s))
#define CDCWRITECHAR(c)		cdc_write_char(c);
#define CDCWRITEFLUSH()     tud_cdc_n_write_flush(0)


#define CDC_THROTTLE_WRITES_OVER	10
#define CDC_THROTTLE_RATIO_MS		6
void cdc_write_char(char c);

void cdc_write(const char * s, uint32_t len);
int32_t cdc_read_char(void);
uint32_t cdc_available(void);


void cdc_write_u32(uint32_t v);
void cdc_write_u16(uint16_t v);
void cdc_write_u8(uint8_t v);
void cdc_write_u8_leadingzeros(uint8_t v);

void cdc_write_u32_ln(uint32_t v);
void cdc_write_u16_ln(uint16_t v);
void cdc_write_u8_ln(uint8_t v);


void cdc_write_dec_u32(uint32_t v);
void cdc_write_dec_u16(uint16_t v);
void cdc_write_dec_u8(uint8_t v);

void cdc_write_dec_u32_ln(uint32_t v);
void cdc_write_dec_u16_ln(uint16_t v);
void cdc_write_dec_u8_ln(uint8_t v);

bool cdc_write_busy();
void cdc_write_debug(const char * s, uint32_t len);


uint8_t u32_to_hexstr(uint32_t value, char* buffer);
uint8_t u16_to_hexstr(uint16_t value, char* buffer);
uint8_t u8_to_hexstr(uint8_t value, char* buffer);

#endif
