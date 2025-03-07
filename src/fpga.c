/*
 * fpga.c, part of the riffpga project
 *
 *  Created on: Dec 18, 2024
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

#include "board_defs.h"
#include "board_config.h"
#include "board_config_defaults.h"
#include "fpga.h"
#include "debug.h"

typedef struct fpga_state_struct {

	bool is_init;
	bool is_programmed;
	bool in_reset;
	uint8_t pin_reset;
	uint8_t pin_reset_dir;
	bool reset_switch_enabled;

	bool spi_tx_started;
	uint8_t spi_idx;

} FPGA_State;

static volatile FPGA_State fpgastate = { 0 };

#define SPIDEVICE(fpgastate) (fpgastate.spi_idx == 0 ? spi0 : spi1)

#define FPGA_DEBUG_ENABLE
#ifdef FPGA_DEBUG_ENABLE
#define FPGA_DEBUG_ENDLN()	DEBUG_ENDLN()
#define FPGA_DEBUG_BUF(b, len) DEBUG_BUF(b, len)
#define FPGA_DEBUG(s) DEBUG(s)
#define FPGA_DEBUG_LN(s)	DEBUG_LN(s)

#define FPGA_DEBUG_U32(v) DEBUG_U32(v)
#define FPGA_DEBUG_U32_LN(v) DEBUG_U32_LN(v)
#define FPGA_DEBUG_U16(v) DEBUG_U16(v)

#define FPGA_DEBUG_U16_LN(v) DEBUG_U16_LN(v)
#define FPGA_DEBUG_U8(v) DEBUG_U8(v)
#define FPGA_DEBUG_U8_LN(v) DEBUG_U8_LN(v)
#else

#define FPGA_DEBUG_ENDLN()
#define FPGA_DEBUG_BUF(b, len)
#define FPGA_DEBUG(s)
#define FPGA_DEBUG_LN(s)

#define FPGA_DEBUG_U32(v)
#define FPGA_DEBUG_U32_LN(v)
#define FPGA_DEBUG_U16(v)

#define FPGA_DEBUG_U16_LN(v)
#define FPGA_DEBUG_U8(v)
#define FPGA_DEBUG_U8_LN(v)
#endif

volatile bool _fpga_extrst_done = false;

volatile bool fpga_external_reset(void) {
	return _fpga_extrst_done;
}

void fpga_external_reset_handled(void) {
	_fpga_extrst_done = false;
}

void gpio_int_callback(uint gpio, uint32_t event_mask) {
	// single, useless, centralized callback
	// using gpio_add_raw_irq_handler to have a more
	// fine-grained system

}

void reset_button_release(void) {
	uint32_t irqmask = gpio_get_irq_event_mask(fpgastate.pin_reset);
	if (irqmask) {
		gpio_acknowledge_irq(fpgastate.pin_reset, irqmask);
		if (fpgastate.is_init && fpgastate.reset_switch_enabled
				&& !fpgastate.in_reset) {
			CDCWRITESTRING("\r\nRESET switch");
			_fpga_extrst_done = true;
		}
		fpgastate.reset_switch_enabled = true;
	}
}
static void fpga_set_reset_pin_dir(uint8_t dir) {
	fpgastate.pin_reset_dir = dir;
	gpio_set_dir(fpgastate.pin_reset, dir);
}
static void fpga_reset_monitor_enable(BoardConfigPtrConst bc, bool do_enable) {

	if (do_enable == true) {
		fpga_set_reset_pin_dir(GPIO_IN);
	}

	fpgastate.reset_switch_enabled = false;
	if (bc->fpga_cram.reset_inverted) {

		gpio_set_irq_enabled(bc->fpga_cram.pin_reset, GPIO_IRQ_EDGE_RISE,
				do_enable);
	} else {
		gpio_set_irq_enabled(bc->fpga_cram.pin_reset, GPIO_IRQ_EDGE_FALL,
				do_enable);

	}

}
void fpga_init(void) {

	uint8_t spi0_scks[] = { 2, 6, 18, 22, 0xff };

	BoardConfigPtrConst bc = boardconfig_get();

	uint8_t i = 0;
	fpgastate.spi_idx = 1;
	while (spi0_scks[i] != 0xff) {
		if (bc->fpga_cram.spi.pin_sck == spi0_scks[i]) {
			fpgastate.spi_idx = 0;
		}
		i++;
	}

	spi_init(SPIDEVICE(fpgastate), bc->fpga_cram.spi.rate);
	// NO: do it manual style gpio_set_function(PIN_FPGA_SPI_CS, GPIO_FUNC_SPI);
	gpio_set_function(bc->fpga_cram.spi.pin_miso, GPIO_FUNC_SPI);
	gpio_set_function(bc->fpga_cram.spi.pin_mosi, GPIO_FUNC_SPI);
	gpio_set_function(bc->fpga_cram.spi.pin_sck, GPIO_FUNC_SPI);

	spi_set_format(SPIDEVICE(fpgastate), 8, bc->fpga_cram.spi.polarity,
			bc->fpga_cram.spi.phase,
			bc->fpga_cram.spi.order /* unused... must be MSB */);

	fpgastate.pin_reset = bc->fpga_cram.pin_reset;
	gpio_init(bc->fpga_cram.pin_reset);

	fpga_reset(true);

	if (bc->system.fpga_reset_external_trigger) {
		// setup the IRQ handler
		gpio_set_irq_enabled_with_callback(bc->fpga_cram.pin_reset,
				GPIO_IRQ_EDGE_RISE, true, gpio_int_callback);
		gpio_add_raw_irq_handler(bc->fpga_cram.pin_reset, reset_button_release);

	} else {
		fpga_set_reset_pin_dir(GPIO_OUT);

	}

#ifdef FPGA_PROG_DONE_LEVEL
    gpio_init(bc->fpga_cram.pin_done);
    gpio_set_dir(bc->fpga_cram.pin_done, GPIO_IN);
#endif

	// NO: do it manual style gpio_set_function(PIN_FPGA_SPI_CS, GPIO_FUNC_SPI);

	gpio_init(bc->fpga_cram.spi.pin_cs);
	gpio_set_dir(bc->fpga_cram.spi.pin_cs, GPIO_OUT);
	gpio_put(bc->fpga_cram.spi.pin_cs, bc->fpga_cram.spi.cs_inverted);

	fpgastate.is_init = true;

}

void fpga_FPGA_DEBUG_spi_pins(void) {

	uint8_t dummyData[5] = { 0xaa, 0x55, 0x12, 0x33, 0xaa };
	BoardConfigPtrConst bc = boardconfig_get();
	spi_deinit(SPIDEVICE(fpgastate));

	// NO: do it manual style gpio_set_function(PIN_FPGA_SPI_CS, GPIO_FUNC_SPI);
	gpio_set_function(bc->fpga_cram.spi.pin_miso, GPIO_FUNC_NULL);
	gpio_set_function(bc->fpga_cram.spi.pin_mosi, GPIO_FUNC_NULL);
	gpio_set_function(bc->fpga_cram.spi.pin_sck, GPIO_FUNC_NULL);

	gpio_init(bc->fpga_cram.spi.pin_sck);
	gpio_set_dir(bc->fpga_cram.spi.pin_sck, GPIO_OUT);

	gpio_init(bc->fpga_cram.spi.pin_mosi);
	gpio_set_dir(bc->fpga_cram.spi.pin_mosi, GPIO_OUT);

	for (uint8_t i = 0; i < 3; i++) {
		gpio_put(bc->fpga_cram.spi.pin_cs, 0);
		gpio_put(bc->fpga_cram.spi.pin_sck, 1);
		gpio_put(bc->fpga_cram.spi.pin_mosi, 1);
		gpio_put(bc->fpga_cram.spi.pin_sck, 0);
		gpio_put(bc->fpga_cram.spi.pin_cs, 1);
		gpio_put(bc->fpga_cram.spi.pin_mosi, 0);

	}

	fpga_init();

	fpga_enter_programming_mode();

	fpga_spi_write(dummyData, 5); // DUMMY BYTE
	fpga_exit_programming_mode();

}

bool fpga_external_reset_applied(void) {

	if (fpgastate.pin_reset_dir == GPIO_IN) {

		BoardConfigPtrConst bc = boardconfig_get();
		if (bc->fpga_cram.reset_inverted) {
			return gpio_get(bc->fpga_cram.pin_reset) == 0;
		} else {
			return gpio_get(bc->fpga_cram.pin_reset) == 1;
		}
	}

	return false;
}

bool fpga_is_in_reset(void) {

	if (fpgastate.in_reset == true) {
		return true;
	}

	if (fpga_external_reset_applied()) {
		return true;
	}

	return false;
}
void fpga_reset(bool set_to) {
	BoardConfigPtrConst bc = boardconfig_get();
	if (set_to) {
		FPGA_DEBUG_LN("Resetting FPGA");
		fpgastate.reset_switch_enabled = false;

		fpgastate.in_reset = true;
		fpgastate.is_programmed = false;
		if (bc->system.fpga_reset_external_trigger) {
			fpga_reset_monitor_enable(bc, false); // disable IRQ
			fpga_set_reset_pin_dir(GPIO_OUT);
		}
		gpio_put(bc->fpga_cram.pin_reset, !bc->fpga_cram.reset_inverted);
	} else {

		FPGA_DEBUG_LN("FPGA Reset RELEASE");

		// about to release from reset, ensure we tell it it's in slave mode
		gpio_put(bc->fpga_cram.spi.pin_cs, !bc->fpga_cram.spi.cs_inverted);
		// now release
		gpio_put(bc->fpga_cram.pin_reset, bc->fpga_cram.reset_inverted);
		if (bc->system.fpga_reset_external_trigger) {
			fpga_reset_monitor_enable(bc, true); // set to input and enable IRQ
		}

		fpgastate.in_reset = false;
	}

	sleep_ms(FLASH_RESET_DELAY_MS);
}
bool fpga_in_reset(void) {
	return fpgastate.in_reset;
}

bool fpga_is_init(void) {
	return fpgastate.is_init;
}
bool fpga_is_programmed(void) {

#ifdef FPGA_PROG_DONE_LEVEL
	BoardConfigPtrConst bc = boardconfig_get();
	if (fpgastate.is_programmed == false) {
			return false;
	}

	if (gpio_get(bc->fpga_cram.pin_done) == FPGA_PROG_DONE_LEVEL) {
		return true;
	}

	return false;

#else
	return fpgastate.is_programmed;
#endif
}

void fpga_set_programmed(bool set_to) {
	fpgastate.is_programmed = set_to;
}

void fpga_enter_programming_mode(void) {
	BoardConfigPtrConst bc = boardconfig_get();
	uint8_t dummy_byte = 0;
	fpga_reset(true);

	fpga_spi_transaction_begin(); // cs goes low
	FPGA_DEBUG_LN("doing progmode reset ");
	sleep_ms(8);
	fpga_reset(false); // we disable reset, fpga acts as slave
	sleep_ms(6); // minimum of 1200us here!
	// send 8 dummy clocks
	gpio_put(bc->fpga_cram.spi.pin_cs, bc->fpga_cram.spi.cs_inverted); // release

	fpga_spi_write(&dummy_byte, 1); // DUMMY BYTE
	// back to low
	gpio_put(bc->fpga_cram.spi.pin_cs, !bc->fpga_cram.spi.cs_inverted); // select
}

void fpga_exit_programming_mode(void) {
	uint8_t dummy_bytes[6] = { 0 };
	fpga_spi_write(dummy_bytes, sizeof(dummy_bytes));

	fpga_spi_transaction_end();
}

void fpga_spi_transaction_begin(void) {
	BoardConfigPtrConst bc = boardconfig_get();
	gpio_put(bc->fpga_cram.spi.pin_cs, !bc->fpga_cram.spi.cs_inverted); // select

	fpgastate.spi_tx_started = true;
	asm volatile("nop \n nop \n nop");
}
void fpga_spi_transaction_end(void) {

	BoardConfigPtrConst bc = boardconfig_get();
	gpio_put(bc->fpga_cram.spi.pin_cs, bc->fpga_cram.spi.cs_inverted); // release

	fpgastate.spi_tx_started = false;
}

void fpga_spi_write(uint8_t *bts, size_t len) {
	bool end_tx = false;
	if (!fpgastate.spi_tx_started) {
		end_tx = true;
		fpga_spi_transaction_begin();
	}

	while (spi_is_busy(SPIDEVICE(fpgastate))) {
		FPGA_DEBUG_LN("spi bzzy");
	}
	spi_write_blocking(SPIDEVICE(fpgastate), bts, len);

	if (end_tx) {
		fpga_spi_transaction_end();
	}
}
