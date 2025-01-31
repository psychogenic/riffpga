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

const uint8_t *flash_read_access = (const uint8_t*) (XIP_BASE);
#define MAX_NUM_PAGES 128

typedef struct page_state_struct {
	int16_t index;
	uint8_t erased;
	uint16_t blocks_written;
} PageState;
static PageState pages_erased[MAX_NUM_PAGES] = { 0 };
static uint32_t size_uf2_written = 0;
static uint32_t uf2_start_address = 0;

//define BRD_DEBUG_ENABLE
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
	watchdog_enable(250, 1);
	// while(1) {}

}

void board_gpio_init(void) {

}

// Initialize flash for DFU
void board_flash_init(void) {
	board_size_written_clear();

}
static uint8_t pages_erased_cache_index_for(uint16_t page) {

	for (uint8_t i = 0; i < MAX_NUM_PAGES; i++) {
		if (pages_erased[i].index < 0) {
			// shouldn't happen
			return MAX_NUM_PAGES - 1;
		}
		if (pages_erased[i].index == page) {
			return i;
		}
	}
	// shouldn't happen
	return MAX_NUM_PAGES - 1;
}
static bool page_was_erased(uint16_t page) {

	for (uint8_t i = 0; i < MAX_NUM_PAGES; i++) {
		if (pages_erased[i].index < 0) {
			return 0;
		}
		if (pages_erased[i].index == page) {
			return pages_erased[i].erased;
		}
	}
	return 0;
}

static uint16_t page_for(uint32_t addr) {
	uint16_t pid = (uint16_t) (addr >> 12);
	return pid;
}
static uint16_t page_blockmask_for(uint32_t req_addr, uint32_t len) {
	uint16_t pid = page_for(req_addr);

	uint32_t page_start_address = (((uint32_t)pid) << 12);

	uint32_t addr = req_addr;
	uint16_t write_block_size = 256;
	uint16_t mask = 0;
	while (addr < (req_addr + len)) {

		mask |= 1 << (uint8_t)((addr - page_start_address)/write_block_size);

		addr += write_block_size;

	}
	/*
	BRD_DEBUG("BMsk: ");
	BRD_DEBUG_U32(addr);
	BRD_DEBUG("/");
	BRD_DEBUG_U32(page_start_address);
	BRD_DEBUG(", ");
	BRD_DEBUG_U32(len);
	BRD_DEBUG(": ");
	BRD_DEBUG_U16_LN(mask);
	*/
	return mask;

}

static bool has_been_programmed(uint32_t req_addr, uint32_t len) {
	uint16_t page = page_for(req_addr);
	uint16_t blocks_to_write = page_blockmask_for(req_addr, len);

	uint8_t idx = pages_erased_cache_index_for(page);
	if (pages_erased[idx].blocks_written & blocks_to_write) {
		return true;
	}
	return false;
}

static void register_programmed(uint32_t req_addr, uint32_t len) {
	uint16_t page = page_for(req_addr);
	uint16_t blocks_written = page_blockmask_for(req_addr, len);

	uint8_t idx = pages_erased_cache_index_for(page);
	pages_erased[idx].blocks_written |= blocks_written;
	BRD_DEBUG("Pg ");
	BRD_DEBUG_U16(page);
	BRD_DEBUG(" wrt ");
	BRD_DEBUG_U16_LN(pages_erased[idx].blocks_written);


	if (pages_erased[idx].blocks_written == 0xffff) {
		// we *had* erased, but now everything's been written over
		// this is no longer to be considered erased.
		pages_erased[idx].erased = 0;
		BRD_DEBUG("All filled!");
	}

}

uint32_t board_first_written_address(void) {
	return uf2_start_address;
}
int16_t board_first_written_page(void) {
	int16_t first_page = 0x7fff;
	bool found_pages = false;
	for (uint8_t i = 0; i < MAX_NUM_PAGES; i++) {
		if (pages_erased[i].index >= 0) {
			if (pages_erased[i].index < first_page) {
				first_page = pages_erased[i].index;
				found_pages = true;
			}
		}
	}

	if (found_pages) {
		return first_page;
	}
	return -1;
}

uint32_t page_address_from_index(uint16_t page) {
	uint32_t addr = ((uint32_t) page) << 12;
	return addr;
}

static void mark_as_erased(uint16_t page) {

	for (uint8_t i = 0; i < MAX_NUM_PAGES; i++) {
		if (pages_erased[i].index < 0) {
			pages_erased[i].index = page;
			pages_erased[i].erased = 1;
			pages_erased[i].blocks_written = 0;
			BRD_DEBUG("Mark page "); BRD_DEBUG_U16(page); BRD_DEBUG(" erased @ "); BRD_DEBUG_U8_LN(i);
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
void board_flash_read(uint32_t addr, void *buffer, uint32_t len) {
	// BRD_DEBUG("flash read: ");
	// BRD_DEBUG_U32_LN(addr);
	uint32_t offset = addr;
	memcpy(buffer, (void const*) &(flash_read_access[offset]), len);

}

static void call_flash_page_erase(void *param) {
	uint16_t *page = (uint16_t*) param;

	uint32_t offset = page_address_from_index(*page);
	BRD_DEBUG("Erasing @"); BRD_DEBUG_U32_LN(offset);
	flash_range_erase(offset, FLASH_SECTOR_SIZE);
}

static void call_flash_range_program(void *param) {
	uint32_t offset = ((uintptr_t*) param)[0];
	const uint8_t *data = (const uint8_t*) ((uintptr_t*) param)[1];
	const uint32_t *size = (const uint32_t*) ((uintptr_t*) param)[2];
	// BRD_DEBUG("Range prog @");
	// BRD_DEBUG_U32(offset);
	// BRD_DEBUG(" sz:");
	// BRD_DEBUG_U32_LN(*size);

	flash_range_program(offset, data, *size);
}

bool board_flash_write(uint32_t addr, void const *data, uint32_t len) {
	(void) data;
	(void) len;
	// BRD_DEBUG("flash write: ");
	// BRD_DEBUG_U32_LN(addr);
	uint32_t *lenptr = &len;
	int rc;
	uint16_t page_index = page_for(addr);
	if (!page_was_erased(page_index)) {
		rc = flash_safe_execute(call_flash_page_erase, (void*) (&page_index),
				UINT32_MAX);
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

	if (has_been_programmed(addr, len)) {
		BRD_DEBUG("Has already been written! ");
		BRD_DEBUG_U32(addr);
		BRD_DEBUG(" len ");
		BRD_DEBUG_U32_LN(len);

	}


	uintptr_t params[] = { addr, (uintptr_t) data, (uintptr_t) lenptr };
	rc = flash_safe_execute(call_flash_range_program, params, UINT32_MAX);
	if (rc == PICO_OK) {
		// BRD_DEBUG_LN("Wrote!");
		register_programmed(addr, len);
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

	for (uint8_t i = 0; i < MAX_NUM_PAGES; i++) {
		pages_erased[i].index = -1;
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
