/*
 * board.h, part of the riffpga project
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

#ifndef BOARDSPECIFIC_FUNCTIONS_H
#define BOARDSPECIFIC_FUNCTIONS_H 1

#include "board_includes.h"
#include "board_defs.h"

void board_gpio_init(void);



// Initialize flash for DFU
void board_flash_init(void);

// Get size of flash
uint32_t board_flash_size(void);

uint32_t board_size_written(void);
void board_size_written_clear(void);
void board_flash_pages_erased_clear(void);

// returns first written page address, or -1 if none
int16_t board_first_written_page(void);
uint32_t page_address_from_index(uint16_t page_index);
uint32_t board_first_written_address(void);



// Read from flash
void board_flash_read (uint32_t addr, void* buffer, uint32_t len);

// Write to flash, len is uf2's payload size (often 256 bytes)
bool board_flash_write(uint32_t addr, void const* data, uint32_t len);

// Flush/Sync flash contents
void board_flash_flush(void);

void board_reboot(void);





#endif
