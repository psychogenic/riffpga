/*
 * board_config.c, part of the riffpga project
 *
 *  Created on: Dec 20, 2024
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

#include "config_defaults/sys_version.h"
#include "board_includes.h"
#include <stdlib.h>
#include "board_config.h"
#include "board_config_defaults.h"
#include "debug.h"
#include "board.h"
#include "uf2.h"
#include "clock_pwm.h"
#include "bitstream.h"

static  BoardConfig _board_conf_singleton_obj = {0};

static BoardConfig * _board_conf_singleton_ptr = &_board_conf_singleton_obj;
static bool _boardconf_is_init = false;


static void board_config_reinit(void);

static bool version_mismatch(const VersionInfo const * v1, const VersionInfo const * v2);


void boardconfig_init(void) {
	BoardConfig testBC;
	VersionInfo curVer = {
			.major = BOARD_VERSION_MAJOR,
			.minor = BOARD_VERSION_MINOR,
			.patchlevel = BOARD_VERSION_PATCH
	};
	if (_boardconf_is_init) {
		DEBUG_LN("bc already init");
		return;
	}
	_board_conf_singleton_ptr = &_board_conf_singleton_obj;

	_boardconf_is_init = true;
	UF2_Block config_block;
	board_flash_read(BOARD_CONFIG_FLASHADDRESS, &config_block,
			sizeof(UF2_Block));

	if (config_block.magicStart0 == UF2_MAGIC_START0
			&& config_block.magicStart1 == BOARDCONF_UF2_MAGIC_START1
			&& config_block.magicEnd == BOARDCONF_UF2_MAGIC_END
			&& config_block.familyID == BOARDCONF_UF2_FAMILY_ID) {
		DEBUG_LN("Have valid board config!");
		memcpy(&testBC, config_block.data, sizeof(BoardConfig));
		if (version_mismatch(&testBC.version, &curVer) == true) {
			// use something valid
			board_config_reinit();
			// but note we are out of date
			memcpy(&(_board_conf_singleton_obj.version), &(testBC.version),
					sizeof(VersionInfo));
		} else {
			// flash data all good
			memcpy(&_board_conf_singleton_obj, config_block.data,
					sizeof(BoardConfig));
		}
	} else {
		DEBUG_LN("No config block--initializing");
		boardconfig_factoryreset(false);
	}
}

void boardconfig_factoryreset(bool erase_bitstreams) {

	//DEBUG_LN("bc fact reset");
	board_config_reinit();
	boardconfig_write();
	if (erase_bitstreams) {
		bs_erase_all();
	}
	// boardconfig_write();

}

BoardConfigPtrConst boardconfig_get(void) {
	if (!_boardconf_is_init) {
		boardconfig_init();
	}

	// CDCWRITESTRING("\r\nPTR ");
	// cdc_write_dec_u32_ln((uint32_t)&_board_conf_singleton_ptr);
	return _board_conf_singleton_ptr;

}

uint32_t boardconfig_autoclocking_achieved(uint8_t idx) {
	return (uint32_t)clock_pwm_freq_achieved(boardconfig_autoclocking(idx));
}
FPGA_PWM* boardconfig_autoclocking(uint8_t idx) {
	if (idx > 1) {
		return NULL;
	}

	return &(_board_conf_singleton_ptr->clocking[idx]);
}

uint32_t boardconfig_bin_startoffset(void) {
	BoardConfigPtrConst bc = boardconfig_get();

	return bc->bin_position.slot_start_address[bc->bin_position.selected_slot];
}

uint32_t boardconfig_bs_marker_address(void) {

	BoardConfigPtrConst bc = boardconfig_get();
	return bc->bs_marker_position.slot_start_address[bc->bs_marker_position.selected_slot];
}

uint32_t boardconfig_bs_marker_address_for(uint8_t slotidx) {

	BoardConfigPtrConst bc = boardconfig_get();
	return bc->bs_marker_position.slot_start_address[slotidx];
}

bool boardconfig_version_outdated(void) {
	BoardConfigPtrConst bc = boardconfig_get();
	if ((bc->version.patchlevel < BOARD_VERSION_PATCH)
			|| (bc->version.minor < BOARD_VERSION_MINOR)
			|| (bc->version.major < BOARD_VERSION_MAJOR)

			) {
		return true;
	}

	return false;
}

static bool version_mismatch(const VersionInfo const *v1,
		const VersionInfo const *v2) {
	if ((v1->patchlevel != v2->patchlevel) || (v1->minor != v2->minor)
			|| (v1->major != v2->major)

			) {
		return true;
	}

	return false;

}
bool boardconfig_version_mismatch(void) {
	BoardConfigPtrConst bc = boardconfig_get();
	if ((bc->version.patchlevel != BOARD_VERSION_PATCH)
			|| (bc->version.minor != BOARD_VERSION_MINOR)
			|| (bc->version.major != BOARD_VERSION_MAJOR)

			) {
		return true;
	}

	return false;

}

void board_config_reinit(void) {
	/* do full reinit of board conf */
	BoardConfig bc = {
			.version  = {
					.major = BOARD_VERSION_MAJOR,
					.minor = BOARD_VERSION_MINOR,
					.patchlevel = BOARD_VERSION_PATCH
			},
			.system = {
					.clock_freq_hz = BOARD_SYSTEM_CLOCK_FREQ_HZ,
					.auto_reset_on_newer_version = SYSTEM_NEWER_VERSION_AUTO_RESET,
					.fpga_reset_external_trigger = FPGA_RESET_EXTERNALLY_TRIGGERED,
					.num_inputs = SYSTEM_INPUTS_NUM
			},
			.bin_download = {
					.magic_start = BIN_UF2_MAGIC_START1,
					.magic_end = BIN_UF2_MAGIC_END,
					.family_id = BIN_UF2_FAMILY_ID
			},

			.bin_position = {
					.selected_slot = 0,
			},


			.bs_marker = {
					.magic_start = BITSTREAM_UF2_MAGIC_START1,
					.magic_end = BITSTREAM_UF2_MAGIC_END,
					.family_id = BITSTREAM_UF2_FAMILY_ID
			},

			.bs_marker_position = {
					.selected_slot = 0
			},

			.fpga_cram = {
					.spi = {
							.rate = FLASH_SPI_BAUDRATE,
							.cs_inverted = FLASH_SPI_CS_INVERTED,
							.phase = FLASH_SPI_PHASE,
							.polarity = FLASH_SPI_POLARITY,
							.order = 0, /* unused */
							.pin_cs = PIN_FPGA_SPI_CS,
							.pin_sck = PIN_FPGA_SPI_SCK,
							.pin_miso = PIN_FPGA_SPI_MISO,
							.pin_mosi = PIN_FPGA_SPI_MOSI
					},
#ifdef FPGA_PROG_DONE_LEVEL
					.pin_done = PIN_FPGA_PROG_DONE,
#else
					.pin_done = 0,
#endif
					.pin_reset = PIN_FPGA_RESET,
					.reset_inverted = FPGA_RESET_INVERTED,
			},

			.uart_bridge = {
					.baud = UART_BRIDGE_BAUDRATE,
					.enabled = UART_BRIDGE_ENABLED,
					.uartnum = UART_BRIDGE_DEVICEIDX,
					.pin_tx = UART_BRIDGE_PIN_TX,
					.pin_rx = UART_BRIDGE_PIN_RX
			}

	};

	memset(bc.system.input_io, 0xff, BOARD_MAX_NUM_INPUTS);
#if (SYSTEM_INPUTS_NUM > 0)
	uint8_t pins[SYSTEM_INPUTS_NUM] = {SYSTEM_INPUTS_IO_LIST};
	memcpy(bc.system.input_io, pins, SYSTEM_INPUTS_NUM);
#endif

	// board name
	memset(bc.board_name, 0, (BOARD_NAME_CHARS+1));
	memcpy(bc.board_name, BOARD_NAME, sizeof(BOARD_NAME));
	memset(bc.uart_bridge.breakout_sequence, 0, (UART_BREAKOUT_SEQUENCE_CHARS_MAX + 1));
	bc.uart_bridge.breakout_sequence[0] = UART_BRIDGE_ESCAPE_SEQUENCE0;
	bc.uart_bridge.breakout_sequence[1] = UART_BRIDGE_ESCAPE_SEQUENCE1;
	bc.uart_bridge.breakout_sequence[2] = UART_BRIDGE_ESCAPE_SEQUENCE2;


	// slot addresses
	for (uint8_t i=0; i<POSITION_SLOTS_NUM; i++) {
		bc.bin_position.slot_start_address[i] = FLASH_STORAGE_STARTADDRESS(i);
		bc.bs_marker_position.slot_start_address[i] = MARKER_SLOT_ADDRESS(i);

	}

	// clocking
	bc.clocking[0].enabled = AUTOCLOCK1_DEFAULT_ENABLE;
	bc.clocking[0].freq_hz = AUTOCLOCK1_DEFAULT_FREQ;
	bc.clocking[0].duty = (0xffff/2);
	bc.clocking[0].div = 1;  // arbitrary
	bc.clocking[0].top = 0xff; // arbitrary
	bc.clocking[0].pin = PIN_AUTOCLOCK1;

	bc.clocking[1].enabled = AUTOCLOCK2_DEFAULT_ENABLE;
	bc.clocking[1].freq_hz = AUTOCLOCK2_DEFAULT_FREQ;
	bc.clocking[1].duty = (0xffff/2);
	bc.clocking[1].div = 1;  // arbitrary
	bc.clocking[1].top = 0xff; // arbitrary
	bc.clocking[1].pin = PIN_AUTOCLOCK2;

	bc.switches[USER_SWITCH_IDX_RESET].function = USER_SWITCH_RESET_FUNCTION;
	bc.switches[USER_SWITCH_IDX_RESET].pin = USER_SWITCH_RESET_PIN;
	bc.switches[USER_SWITCH_IDX_RESET].pull = USER_SWITCH_RESET_PULL;
	bc.switches[USER_SWITCH_IDX_RESET].inverted = USER_SWITCH_RESET_INVERT;
	bc.switches[USER_SWITCH_IDX_RESET].irq_edge = USER_SWITCH_RESET_EDGE;

	bc.switches[USER_SWITCH_IDX_CLOCKING].function = USER_SWITCH_CLOCK_FUNCTION;
	bc.switches[USER_SWITCH_IDX_CLOCKING].pin = USER_SWITCH_CLOCK_PIN;
	bc.switches[USER_SWITCH_IDX_CLOCKING].pull = USER_SWITCH_CLOCK_PULL;
	bc.switches[USER_SWITCH_IDX_CLOCKING].inverted = USER_SWITCH_CLOCK_INVERT;
	bc.switches[USER_SWITCH_IDX_CLOCKING].irq_edge = USER_SWITCH_CLOCK_EDGE;

	bc.switches[USER_SWITCH_IDX_USR1].function = USER_SWITCH_USR1_FUNCTION;
	bc.switches[USER_SWITCH_IDX_USR1].pin = USER_SWITCH_USR1_PIN;
	bc.switches[USER_SWITCH_IDX_USR1].pull = USER_SWITCH_USR1_PULL;
	bc.switches[USER_SWITCH_IDX_USR1].inverted = USER_SWITCH_USR1_INVERT;
	bc.switches[USER_SWITCH_IDX_USR1].irq_edge = USER_SWITCH_USR1_EDGE;

	bc.switches[USER_SWITCH_IDX_USR2].function = USER_SWITCH_USR2_FUNCTION;
	bc.switches[USER_SWITCH_IDX_USR2].pin = USER_SWITCH_USR2_PIN;
	bc.switches[USER_SWITCH_IDX_USR2].pull = USER_SWITCH_USR2_PULL;
	bc.switches[USER_SWITCH_IDX_USR2].inverted = USER_SWITCH_USR2_INVERT;
	bc.switches[USER_SWITCH_IDX_USR2].irq_edge = USER_SWITCH_USR2_EDGE;




	memcpy(&_board_conf_singleton_obj, &bc, sizeof(BoardConfig));
	_board_conf_singleton_ptr = &_board_conf_singleton_obj; // (BoardConfig*)malloc(sizeof(BoardConfig));


}


void boardconfig_dump(void) {
	DEBUG_BUF(_board_conf_singleton_ptr->board_name, 19);
	DEBUG_U8_LN(_board_conf_singleton_ptr->bs_marker_position.selected_slot);
	DEBUG_U32_LN(_board_conf_singleton_ptr->bs_marker_position.slot_start_address[0]);
}

void boardconfig_write(void) {
	UF2_Block config_block;
    config_block.magicStart0 = UF2_MAGIC_START0;
    config_block.magicStart1 = BOARDCONF_UF2_MAGIC_START1;
    config_block.magicEnd = BOARDCONF_UF2_MAGIC_END;
    config_block.familyID = BOARDCONF_UF2_FAMILY_ID;
    config_block.flags = UF2_FLAG_FAMILYID | UF2_FLAG_NOFLASH;
    config_block.blockNo = 1;
    config_block.numBlocks = 1;
    config_block.payloadSize = sizeof(BoardConfig);

    memcpy(config_block.data, _board_conf_singleton_ptr, sizeof(BoardConfig));


    DEBUG_LN("Writing config");
    board_flash_write(BOARD_CONFIG_FLASHADDRESS, &config_block, sizeof(config_block));
    board_flash_pages_erased_clear();
    sleep_ms(10);
}


void boardconfig_set_systemclock_hz(uint32_t v) {

	if (set_sys_clock_khz(v / 1000, false)) {

		_board_conf_singleton_ptr->system.clock_freq_hz = v;
	} else {
		  DEBUG_LN("Could not set requested clock");
	}
}


void boardconfig_autoclock_enable() {

	FPGA_PWM * pwmconf = boardconfig_autoclocking(0);
	clock_pwm_enable(pwmconf);
	clock_pwm_set_freq(pwmconf->freq_hz, pwmconf);
}
void boardconfig_autoclock_disable() {

	FPGA_PWM * pwmconf = boardconfig_autoclocking(0);
	clock_pwm_disable(pwmconf);
}
void boardconfig_set_autoclock_hz(uint32_t v) {

	FPGA_PWM * pwmconf = boardconfig_autoclocking(0);
	if (v == 0) {
		boardconfig_autoclock_disable();
	} else {
		clock_pwm_enable(pwmconf);
		clock_pwm_set_freq(v, pwmconf);
	}
}

uint8_t boardconfig_selected_bitstream_slot(void) {
	return _board_conf_singleton_ptr->bin_position.selected_slot;
}

void boardconfig_set_bitstream_slot(uint8_t s) {
	if (s < POSITION_SLOTS_ALLOWED) {
		/* keep both in sync -- this is redundant, but
		 * they're both StorageSlotInfo so have the selected_slot
		 */
		_board_conf_singleton_ptr->bs_marker_position.selected_slot = s;
		_board_conf_singleton_ptr->bin_position.selected_slot = s;
	} else {

		CDCWRITESTRING("Invalid bs slot: ");
		cdc_write_dec_u8_ln(s);
	}

}

void boardconfig_uartbridge_enable() {
	_board_conf_singleton_ptr->uart_bridge.enabled = 1;
}
void boardconfig_uartbridge_disable() {
	_board_conf_singleton_ptr->uart_bridge.enabled = 0;
}

void boardconfig_set_uartbridge_baudrate(uint32_t v) {
	_board_conf_singleton_ptr->uart_bridge.baud = v;
}
