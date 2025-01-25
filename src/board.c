/*
 * board.c, part of the riffpga project
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
#include "board.h"
#include "debug.h"
#include "cdc_interface.h"

const uint8_t *flash_read_access = (const uint8_t *) (XIP_BASE);
#define MAX_NUM_PAGES 128


static int16_t pages_erased[MAX_NUM_PAGES] = {-1};
static uint32_t size_uf2_written = 0;
static uint32_t uf2_start_address = 0;





// define BRD_DEBUG_ENABLE
#ifdef BRD_DEBUG_ENABLE
#define BRD_DEBUG_ENDLN()	DEBUG_ENDLN()
#define BRD_DEBUG_BUF(b, len) DEBUG_BUF(b, len)
#define BRD_DEBUG(s) DEBUG(s)
#define BRD_DEBUG_LN(s)	DEBUG_LN(s)

#define BRD_DEBUG_U32(v) DEBUG_U32(v)
#define BRD_DEBUG_U32_LN(v) DEBUG_U32_LN(v)
#define BRD_DEBUG_U16(v) DEBUG_U16(v)

#define BRD_DEBUG_U16_LN(v) DEBUG_U16_LN(v)
#define BRD_DEBUG_U8(v) DEBUG_U8(v)
#define BRD_DEBUG_U8_LN(v) DEBUG_U8_LN(v)
#else

#define BRD_DEBUG_ENDLN()
#define BRD_DEBUG_BUF(b, len)
#define BRD_DEBUG(s)
#define BRD_DEBUG_LN(s)

#define BRD_DEBUG_U32(v)
#define BRD_DEBUG_U32_LN(v)
#define BRD_DEBUG_U16(v)

#define BRD_DEBUG_U16_LN(v)
#define BRD_DEBUG_U8(v)
#define BRD_DEBUG_U8_LN(v)
#endif



void board_reboot(void) {
	watchdog_enable(100, 1);
	while(1) {}

}

void board_gpio_init(void) {

}


// Initialize flash for DFU
void board_flash_init(void) {
	board_size_written_clear();

}



static bool page_was_erased(uint16_t idx) {

	for (uint8_t i=0; i<MAX_NUM_PAGES; i++) {
		if (pages_erased[i] < 0) {
			return 0;
		}
		if (pages_erased[i] == idx) {
			return 1;
		}
	}
	return 0;
}

static uint16_t page_index_for(uint32_t addr) {
	uint16_t pid = (uint16_t)(addr >> 12);
	return pid;
}


uint32_t board_first_written_address(void) {
	return uf2_start_address;
}
int16_t board_first_written_page(void) {
	int16_t first_page = 0x7fff;
	bool found_pages = false;
	for (uint8_t i=0; i<MAX_NUM_PAGES; i++) {
		if (pages_erased[i] >= 0) {
			if (pages_erased[i] < first_page) {
				first_page = pages_erased[i];
				found_pages = true;
			}
		}
	}

	if (found_pages) {
		return first_page;
	}
	return -1;
}


uint32_t page_address_from_index(uint16_t page_index) {
	uint32_t addr = ((uint32_t)page_index) << 12;
	return addr;
}

static void mark_as_erased(uint16_t page_index) {

	for (uint8_t i=0; i<MAX_NUM_PAGES; i++) {
		if (pages_erased[i] < 0) {
			pages_erased[i] = page_index;
			BRD_DEBUG("Mark page ");
			BRD_DEBUG_U16(page_index);
			BRD_DEBUG(" erased @ ");
			BRD_DEBUG_U8_LN(i);
			CDCWRITEFLUSH();

			return;
		}
	}
}

// Get size of flash
uint32_t board_flash_size(void) {
	return BOARD_FLASH_SIZE;
}

// Read from flash
void board_flash_read (uint32_t addr, void* buffer, uint32_t len) {
	// BRD_DEBUG("flash read: ");
	// BRD_DEBUG_U32_LN(addr);
	uint32_t offset = addr;
	memcpy(buffer, (void const *)&(flash_read_access[offset]), len);

}

static void call_flash_page_erase(void *param) {
	uint16_t* page_index = (uint16_t*)param;

	uint32_t offset = page_address_from_index(*page_index);
	BRD_DEBUG("Erasing @");
	BRD_DEBUG_U32_LN(offset);
	flash_range_erase(offset, FLASH_SECTOR_SIZE);
}

static void call_flash_range_program(void *param) {
	uint32_t offset = ((uintptr_t*)param)[0];
	const uint8_t * data = (const uint8_t *)((uintptr_t*)param)[1];
	const uint32_t * size = (const uint32_t*)((uintptr_t*)param)[2];
	// BRD_DEBUG("Range prog @");
	// BRD_DEBUG_U32(offset);
	// BRD_DEBUG(" sz:");
	// BRD_DEBUG_U32_LN(*size);

	flash_range_program(offset, data, *size);
}

bool board_flash_write(uint32_t addr, void const* data, uint32_t len) {
	(void)data;
	(void)len;
	// BRD_DEBUG("flash write: ");
	// BRD_DEBUG_U32_LN(addr);
	uint32_t* lenptr = &len;
	int rc;
	uint16_t page_index = page_index_for(addr);
	if (! page_was_erased(page_index) ) {
		rc = flash_safe_execute(call_flash_page_erase, (void*)(&page_index), UINT32_MAX);
		if (rc == PICO_OK) {
			mark_as_erased(page_index);
		} else {
			BRD_DEBUG_LN("ERR ERASE");
			CDCWRITESTRING("\r\nFlash Page errase error!\r\n");
			return 0;
		}
	}
	if (addr < uf2_start_address) {
		uf2_start_address = addr;
	}
		uintptr_t params[] = { addr, (uintptr_t)data, (uintptr_t)lenptr};
		rc = flash_safe_execute(call_flash_range_program, params, UINT32_MAX);
		if (rc == PICO_OK) {
			// BRD_DEBUG_LN("Wrote!");
			size_uf2_written += len;
		} else {
			CDCWRITESTRING("\r\nWrite fail!!\r\n");
		}


	return 1;

}

uint32_t board_size_written(void) {
	return size_uf2_written;
}

void board_flash_pages_erased_clear(void) {

	for (uint8_t i=0; i<MAX_NUM_PAGES; i++) {
		pages_erased[i] = -1;
	}

}
void board_size_written_clear(void) {
	size_uf2_written = 0;
	// set to highest possible
	uf2_start_address = 0xffffffff;

	// clear out pages erased cache
	board_flash_pages_erased_clear();

}

// Flush/Sync flash contents
void board_flash_flush(void) {

	BRD_DEBUG_LN("FLUSH CALLED!!!! WE DONE");
	board_flash_pages_erased_clear();
}
