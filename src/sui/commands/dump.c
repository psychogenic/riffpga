/*
 * dump.c, part of the riffpga project
 *
 *  Created on: Jan 23, 2025
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

#include "sui/commands/dump.h"
#include "sui/commands/io.h"
#include "board_config_defaults.h"
#include "cdc_interface.h"
#include "bitstream.h"

static void dump_clocks(BoardConfigPtrConst bc, SUIInteractionFunctions *funcs) {

	CDCWRITESTRING(" Auto-clocking: ");
	if (bc->clocking[0].enabled) {
		CDCWRITESTRING("ENABLED\r\n");
	} else {
		CDCWRITESTRING("DISABLED\r\n");
	}

	CDCWRITEFLUSH();
	CDCWRITESTRING("\tpin: ");
	cdc_write_dec_u32(bc->clocking[0].pin);
	CDCWRITEFLUSH();
	CDCWRITESTRING(", freq: ");
	cdc_write_dec_u32_ln(bc->clocking[0].freq_hz);

	uint32_t sysclkhz = clock_get_hz(clk_sys);
	if (sysclkhz != bc->system.clock_freq_hz) {
		CDCWRITESTRING(" Sys Clock requested ");
		cdc_write_dec_u32(bc->system.clock_freq_hz);
		CDCWRITESTRING(", actual: ");
	} else {
		CDCWRITESTRING(" Sys Clock: ");
	}

	cdc_write_dec_u32_ln(sysclkhz);
	CDCWRITEFLUSH();

}
static void dump_cram_conf(BoardConfigPtrConst bc,
		SUIInteractionFunctions *funcs) {
	CDCWRITESTRING(" FPGA/CRAM pins: \r\n\tRESET: ");
	cdc_write_dec_u8(bc->fpga_cram.pin_reset);
	CDCWRITESTRING(", CDONE: ");
#ifdef FPGA_PROG_DONE_LEVEL
	cdc_write_dec_u8_ln(bc->fpga_cram.pin_done);
#else
	CDCWRITESTRING("n/a\r\n");
#endif
	CDCWRITESTRING("\tSPI CS: ");
	cdc_write_dec_u8(bc->fpga_cram.spi.pin_cs);
	CDCWRITEFLUSH();
	CDCWRITESTRING(" SCK: ");
	cdc_write_dec_u8(bc->fpga_cram.spi.pin_sck);
	CDCWRITESTRING(" MISO: ");
	cdc_write_dec_u8(bc->fpga_cram.spi.pin_miso);
	CDCWRITESTRING(" MOSI: ");
	cdc_write_dec_u8(bc->fpga_cram.spi.pin_mosi);
	CDCWRITESTRING(" RATE: ");
	cdc_write_dec_u32_ln(bc->fpga_cram.spi.rate);
	CDCWRITEFLUSH();
}
static void dump_uart_conf(BoardConfigPtrConst bc,
		SUIInteractionFunctions *funcs) {

	CDCWRITESTRING(" UART bridge: ");
	if (bc->uart_bridge.enabled) {
		CDCWRITESTRING("ENABLED\r\n");
	} else {
		CDCWRITESTRING("DISABLED\r\n");
	}
	CDCWRITEFLUSH();
	CDCWRITESTRING("\tbaud: ");
	cdc_write_dec_u32(bc->uart_bridge.baud);
	CDCWRITESTRING(", rx: ");
	cdc_write_dec_u8(bc->uart_bridge.pin_rx);
	CDCWRITESTRING(", tx: ");
	cdc_write_dec_u8(bc->uart_bridge.pin_tx);
	CDCWRITESTRING(", breakout: 0x");
	cdc_write_u8(bc->uart_bridge.breakout_sequence[0]);
	CDCWRITESTRING(" 0x");
	cdc_write_u8(bc->uart_bridge.breakout_sequence[1]);
	CDCWRITESTRING(" 0x");
	cdc_write_u8_ln(bc->uart_bridge.breakout_sequence[2]);

}
static void dump_switches_conf(BoardConfigPtrConst bc,
		SUIInteractionFunctions *funcs) {

	for (uint8_t i = 0; i < BOARD_MAX_NUM_SWITCHES; i++) {
		if (bc->switches[i].function != (uint8_t) SwitchFunctionNOTSET) {
			CDCWRITESTRING(" Switch ");
			cdc_write_dec_u8(i);
			CDCWRITESTRING(": ");
			CDCWRITEFLUSH();
			switch (bc->switches[i].function) {
			case (uint8_t) SwitchFunctionReset:
				CDCWRITESTRING("Reset");
				break;
			case (uint8_t) SwitchFunctionClocking:
				CDCWRITESTRING("Clocking");
				break;
			case (uint8_t) SwitchFunctionUser:
				CDCWRITESTRING("User");
				break;
			default:
				break;
			}
			CDCWRITESTRING("\r\n\tpin: ");
			cdc_write_dec_u8(bc->switches[i].pin);
			CDCWRITESTRING(", pull: ");
			cdc_write_dec_u8(bc->switches[i].pull);
			CDCWRITESTRING(", invert: ");
			cdc_write_dec_u8(bc->switches[i].inverted);
			CDCWRITEFLUSH();

			CDCWRITESTRING(", state: ");
			if (gpio_get(bc->switches[i].pin)) {
				CDCWRITESTRING("HIGH\r\n");
			} else {

				CDCWRITESTRING("LOW\r\n");
			}
			CDCWRITEFLUSH();
		}
	}
}
static void dump_fpga_resetprog_state(BoardConfigPtrConst bc,
		SUIInteractionFunctions *funcs) {

	if (gpio_get(bc->fpga_cram.pin_reset)) {
		if (bc->fpga_cram.reset_inverted) {
			CDCWRITESTRING(" FPGA enabled (not in reset), ");
		} else {

			CDCWRITESTRING(" FPGA in RESET, ");
		}
	} else {
		if (bc->fpga_cram.reset_inverted) {
			CDCWRITESTRING(" FPGA in RESET, ");
		} else {
			CDCWRITESTRING(" FPGA enabled (not in reset), ");
		}
	}
	funcs->wait();

	CDCWRITESTRING("cdone is: ");
#ifdef FPGA_PROG_DONE_LEVEL
	if (gpio_get(bc->fpga_cram.pin_done)) {
		CDCWRITESTRING("HIGH, ");
	} else {
		CDCWRITESTRING("LOW, ");
	}
#else
	CDCWRITESTRING("n/a, ");
#endif
	CDCWRITEFLUSH();

	if (bc->clocking[0].enabled) {
		CDCWRITESTRING("AUTOCLOCK ON.\r\n");
	} else {
		CDCWRITESTRING("autoclock OFF.\r\n");
	}
}
static void dump_gp_inputs(BoardConfigPtrConst bc,
		SUIInteractionFunctions *funcs) {

	CDCWRITESTRING(" GP Inputs: ");
	if (!bc->system.num_inputs) {
		CDCWRITESTRING("None\r\n");
	} else {
		for (uint8_t i = 0; i < bc->system.num_inputs; i++) {
			cdc_write_dec_u8(bc->system.input_io[i]);
			CDCWRITECHAR(' ');
		}
		cmd_read_io_inputs(funcs);
		CDCWRITEFLUSH();

	}
}


static void uf2_magic_info(BoardConfigPtrConst bc,
		SUIInteractionFunctions *funcs) {

	CDCWRITESTRING(" UF2 Magic\r\n  Bounds: 0x");
	cdc_write_u32(bc->bin_download.magic_start);
	CDCWRITESTRING("/0x");
	cdc_write_u32_ln(bc->bin_download.magic_end);
	CDCWRITESTRING("  Family: 0x");
	cdc_write_u32_ln(bc->bin_download.family_id);
	funcs->wait();
	CDCWRITEFLUSH();
}


static void dump_bitstream_info(BoardConfigPtrConst bc,
		SUIInteractionFunctions *funcs) {

	Bitstream_Slot_Content slot_contents[POSITION_SLOTS_ALLOWED];
	const Bitstream_Settings *bssets = bs_settings_get();

	uint8_t num_found = bs_slot_contents(slot_contents);
	CDCWRITESTRING(" Slot Contents\r\n");
	for (uint8_t i = 0; i < POSITION_SLOTS_ALLOWED; i++) {
		CDCWRITESTRING("  ");
		cdc_write_dec_u8((i + 1));
		CDCWRITECHAR(':');
		if (slot_contents[i].found == true) {
			if (slot_contents[i].info.namelen) {
				cdc_write(slot_contents[i].info.name, slot_contents[i].info.namelen);
			} else {
				CDCWRITESTRING("unnamed");
			}
		} else {
			CDCWRITESTRING("empty");
		}
		funcs->wait();
		CDCWRITEFLUSH();
	}

	CDCWRITESTRING("\r\n Using bitstream slot ");
	cdc_write_dec_u8(1 + boardconfig_selected_bitstream_slot());
	if (bssets->size) {
		CDCWRITESTRING(", have config of size ");
		cdc_write_dec_u32(bssets->size);
		CDCWRITESTRING(" @ 0x");
		cdc_write_u32_ln(bssets->start_address);
	} else {

		CDCWRITESTRING(", but no valid stream present.\r\n");
	}
}
void cmd_dump_state(SUIInteractionFunctions *funcs) {

	BoardConfigPtrConst bc = boardconfig_get();
	const char *header =
			"\r\n\r\n*********************** State/Config **************************\r\n";
	const char *footer =
			"***************************************************************\r\n";
	CDCWRITESTRING(header);
	CDCWRITESTRING(" Board: ");
	CDCWRITEFLUSH();
	funcs->wait();

	CDCWRITESTRING(bc->board_name);
	CDCWRITESTRING("\r\n RifFPGA Version: ");
	cdc_write_dec_u8(bc->version.major);
	CDCWRITECHAR('.');
	cdc_write_dec_u8(bc->version.minor);
	CDCWRITECHAR('.');
	cdc_write_dec_u8_ln(bc->version.patchlevel);
	CDCWRITESTRING("\r\n");
	CDCWRITEFLUSH();


	funcs->wait();
	dump_clocks(bc, funcs);
	dump_cram_conf(bc, funcs);
	dump_uart_conf(bc, funcs);

	CDCWRITEFLUSH();
	funcs->wait();

	dump_switches_conf(bc, funcs);
	uf2_magic_info(bc, funcs);
	const Bitstream_Marker_State *bsmark = bs_marker_get();
	if (bsmark->have_checked && bsmark->settings.size) {

		CDCWRITESTRING(" Project bitstream: ");
		cdc_write_dec_u32(bsmark->settings.size);
		CDCWRITESTRING(" bytes @ 0x");
		cdc_write_u32_ln(bsmark->settings.start_address);
	}
	funcs->wait();

	dump_gp_inputs(bc, funcs);

	dump_bitstream_info(bc, funcs);

	CDCWRITESTRING("\r\n");
	dump_fpga_resetprog_state(bc, funcs);

	CDCWRITESTRING(footer);

}

void cmd_dump_raw_config(SUIInteractionFunctions *funcs) {

	BoardConfigPtrConst bc = boardconfig_get();

	uint8_t *v = (uint8_t*) bc;

	CDCWRITESTRING("\r\nBoardConfig Raw Bytes:\r\n");
	for (uint32_t i = 0; i < sizeof(BoardConfig); i++) {
		if (i % 32 == 0) {
			CDCWRITESTRING("\r\n");
			CDCWRITEFLUSH();
		}
		cdc_write_u8_leadingzeros(v[i]);
		CDCWRITECHAR(' ');
	}
	CDCWRITESTRING("\r\n\r\n");
	CDCWRITEFLUSH();
}

void cmd_dump_raw_slot(SUIInteractionFunctions *funcs) {
	UF2_Block* raw_bs_block = bs_info();

	uint8_t *v = (uint8_t*) raw_bs_block;

	CDCWRITESTRING("\r\nCurrent slot raw bytes:\r\n");
	for (uint32_t i = 0; i < sizeof(UF2_Block); i++) {
		if (i % 32 == 0) {
			CDCWRITESTRING("\r\n");
			CDCWRITEFLUSH();
		}
		cdc_write_u8_leadingzeros(v[i]);
		CDCWRITECHAR(' ');
	}
	CDCWRITESTRING("\r\n\r\n");
	CDCWRITEFLUSH();
}


