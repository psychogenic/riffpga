/*
 * cdc_interface.c, part of the riffpga project
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
#include "cdc_interface.h"

#include "tusb.h"
#include "bsp/board_api.h"
volatile bool wbusy = 0;

static char digits_buffer[12];

int32_t cdc_read_char(void) {
	return tud_cdc_read_char();
}


uint32_t cdc_available(void) {
	return tud_cdc_available();
}

void cdc_write_char(char c) {
	if (! tud_cdc_ready())
	{
		return;
	}

	tud_cdc_write_char(c);
}

void cdc_write(const char * s, uint32_t len) {
	if (! tud_cdc_ready())
	{
		return;
	}
	while (wbusy) {}
	wbusy = 1;
	tud_cdc_write_flush(); // empty any pending
    tud_cdc_write(s, len);

    if (len >= CDC_THROTTLE_WRITES_OVER) {
    	uint32_t tnowTarg = board_millis() + (len/CDC_THROTTLE_RATIO_MS);
    	while (tnowTarg > board_millis()) {

    	}

    }
    wbusy = 0;
}

void cdc_write_debug(const char * s, uint32_t len) {
	cdc_write(s, len);
}

bool cdc_write_busy() {
	return wbusy;

}


void cdc_write_u32(uint32_t v) {
	uint8_t nc = u32_to_hexstr(v, digits_buffer);
	cdc_write(digits_buffer, nc);
}
void cdc_write_u16(uint16_t v) {
	uint8_t nc = u16_to_hexstr(v, digits_buffer);
	cdc_write(digits_buffer, nc);

}
void cdc_write_u8(uint8_t v) {
	uint8_t nc = u8_to_hexstr(v, digits_buffer);
	cdc_write(digits_buffer, nc);

}

void cdc_write_u8_leadingzeros(uint8_t v) {
	uint8_t nc = u8_to_hexstr(v, digits_buffer);
	if (nc < 2) {
		cdc_write("0", 1);
	}
	cdc_write(digits_buffer, nc);

}


void cdc_write_u32_ln(uint32_t v) {

	uint8_t nc = u32_to_hexstr(v, digits_buffer);
	digits_buffer[nc] = '\r';
	digits_buffer[nc+1] = '\n';
	cdc_write(digits_buffer, nc+2);
}
void cdc_write_u16_ln(uint16_t v) {

	uint8_t nc = u16_to_hexstr(v, digits_buffer);
	digits_buffer[nc] = '\r';
	digits_buffer[nc+1] = '\n';
	cdc_write(digits_buffer, nc+2);
}
void cdc_write_u8_ln(uint8_t v) {
	uint8_t nc = u8_to_hexstr(v, digits_buffer);
	digits_buffer[nc] = '\r';
	digits_buffer[nc+1] = '\n';
	cdc_write(digits_buffer, nc+2);


}


static uint8_t u_to_decstr(uint32_t value, char* buffer) {
  const char decDigits[] = "0123456789";
  char tmp_buffer[12] = {0};
  if (value == 0) {
	  buffer[0] = '0';
	  buffer[1] = '\0';
	  return 1;
  }
  uint8_t charidx = 0;
  while (value > 0) {
	  tmp_buffer[charidx] = decDigits[value % 10];
	  value = value / 10;
	  charidx ++;
  }

  uint8_t i = 0;
  for (int8_t d=charidx - 1; d>=0; d--) {
	  buffer[i] = tmp_buffer[d];
	  i++;
  }
  buffer[i] = '\0';
  return i;
}

static uint8_t u_to_hexstr(uint32_t value, char* buffer, size_t len) {
  const char hexDigits[] = "0123456789ABCDEF";
  size_t i;
  bool started_digits = 0;
  uint8_t charidx = 0;
  for (i = 0; i < len; i++) {
    buffer[charidx] = hexDigits[(value >> (((len*4)-4) - (i * 4))) & 0xF];

    if (! started_digits ) {
    	if (buffer[charidx] != '0') {
    		started_digits = 1;
    	}
    }
    if (started_digits) {
        charidx++;
    }
  }
  if (! charidx ) {
	  // never got anything but 0
	  charidx++;
  }
  buffer[charidx] = '\0';
  return charidx;
}


void cdc_write_dec_u32(uint32_t v) {
	uint8_t nc = u_to_decstr(v, digits_buffer);
	cdc_write(digits_buffer, nc);
}
void cdc_write_dec_u16(uint16_t v){
	uint8_t nc = u_to_decstr(v, digits_buffer);
	cdc_write(digits_buffer, nc);
}
void cdc_write_dec_u8(uint8_t v){
	uint8_t nc = u_to_decstr(v, digits_buffer);
	cdc_write(digits_buffer, nc);
}

void cdc_write_dec_u32_ln(uint32_t v){
	uint8_t nc = u_to_decstr(v, digits_buffer);
	digits_buffer[nc] = '\r';
	digits_buffer[nc+1] = '\n';
	cdc_write(digits_buffer, nc+2);
}
void cdc_write_dec_u16_ln(uint16_t v) {

	uint8_t nc = u_to_decstr(v, digits_buffer);
	digits_buffer[nc] = '\r';
	digits_buffer[nc+1] = '\n';
	cdc_write(digits_buffer, nc+2);
}
void cdc_write_dec_u8_ln(uint8_t v) {

	uint8_t nc = u_to_decstr(v, digits_buffer);
	digits_buffer[nc] = '\r';
	digits_buffer[nc+1] = '\n';
	cdc_write(digits_buffer, nc+2);
}


uint8_t u32_to_hexstr(uint32_t value, char* buffer) {
	return u_to_hexstr(value, buffer, 8);
}
uint8_t u16_to_hexstr(uint16_t value, char* buffer) {
	return u_to_hexstr((uint32_t)value, buffer, 4);
}
uint8_t u8_to_hexstr(uint8_t value, char* buffer) {
	return u_to_hexstr((uint32_t)value, buffer, 2);
}
