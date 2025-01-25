/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "board_config.h"
#include "board_includes.h"
#include "bsp/board_api.h"
#include "tusb.h"
#include "cdc_interface.h"
#include "uf2.h"
#include "debug.h"
#include "board.h"
#include "bitstream.h"
#include "fpga.h"
#include "board_config.h"
#include "clock_pwm.h"
#include "sui/sui_handler.h"
#include "uart_bridge.h"
#include "io_inputs.h"



//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+


#define SLOT_BLINKTIME_ON_MS 	5
#define SLOT_BLINKTIME_OFF_MS 	300
enum {
  BLINK_NOT_MOUNTED = 750,
  BLINK_MOUNTED = 2500,
  BLINK_SUSPENDED = 4000,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

static bool have_programmed = false;

void led_blinking_task(void);
void cdc_task(void);

void run_tasks(void) {

	  tud_task(); // tinyusb device task
	  cdc_task();
	  led_blinking_task();

}
/*------------- MAIN -------------*/
int main(void) {
  board_init();
  board_flash_init();
  uf2_init();


  boardconfig_init();
  if (boardconfig_version_mismatch() == true) {
	  boardconfig_factoryreset();
  }


  BoardConfigPtrConst bconf = boardconfig_get();
  boardconfig_set_systemclock_hz(bconf->system.clock_freq_hz);

  // init device stack on configured roothub port
  	tud_init(BOARD_TUD_RHPORT);
  	if (board_init_after_tusb) {
  		board_init_after_tusb();
  	}
  fpga_init();
  fpga_reset(true);
  io_inputs_init();

  if (bconf->clocking[0].enabled && bconf->clocking[0].freq_hz > 10) {
	  boardconfig_set_autoclock_hz(bconf->clocking[0].freq_hz);
  }

  while (1) {
	  if (fpga_external_reset()) {
		  if (have_programmed) {
			  CDCWRITESTRING("\r\nExternal RST detected\r\n");
		  }
		  have_programmed = false;
		  fpga_external_reset_handled();
	  }
	  run_tasks();
	  if (! have_programmed ) {
		  if (board_millis() > (50 + DEBUG_START_PROGRAM_DELAY_MS)) {
			  have_programmed = true;

			  if (! bs_have_checked_for_marker()) {
				  DEBUG("Checking for bitstream...");
				  if (bs_check_for_marker()) {
					  DEBUG_LN("Got it!");
				  } else {
					  DEBUG_LN("Not found.");
				  }
			  }


			  run_tasks();

			  if (bs_file_size()) {
			  	  DEBUG("Have bitstream, ");
			  	  DEBUG_U32(bs_file_size());
			  	  DEBUG_LN(" bytes. Program...");
			  	  bs_program_fpga(run_tasks);
			  	  DEBUG_LN("Done.");
			  } else {
				  DEBUG_LN("No bitstream found");
			  }

		  }
	  }
  }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
  (void) remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
  blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED;
}

//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+

void tud_and_blink_tasks(void) {
	tud_task();
	led_blinking_task();
	tud_task();
}


void cdc_task(void) {
	static uint8_t breakout_sequence_idx = 0;

	BoardConfigPtrConst bconf = boardconfig_get();
	if (! bconf->uart_bridge.enabled ) {
		sui_handle_request(tud_cdc_write, tud_cdc_read_char, tud_cdc_available, tud_and_blink_tasks);
		return;
	}

	// UART bridge is enabled
	while (tud_cdc_available()) {
		char c = tud_cdc_read_char();
		bool tx_char = true;
		if (c == bconf->uart_bridge.breakout_sequence[breakout_sequence_idx]) {
			breakout_sequence_idx++;
			if (! bconf->uart_bridge.breakout_sequence[breakout_sequence_idx]) {
				// we've matched the whole sequence!
				uart_bridge_disable();
				boardconfig_uartbridge_disable();
				CDCWRITESTRING("\r\nUART Bridge teardown\r\n");
				tx_char = false;
			}
		} else {
			breakout_sequence_idx = 0;
		}

		if (tx_char == true) {
			uart_bridge_putc_raw(c);
		}
	}

	while (uart_bridge_is_readable()) {
		char inc = uart_bridge_getc();
		tud_cdc_write(&inc, 1);

	}


}



// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
  (void) itf;
  (void) rts;

  // TODO set some indicator
  if (dtr) {
    // Terminal connected
  } else {
    // Terminal disconnected
  }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf) {
  (void) itf;
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+

void led_blinking_task(void) {
  static uint8_t led_blink_count = 0;
  static uint32_t next_blink_ms = 0;
  static bool led_state = false;

  BoardConfigPtrConst bconf = boardconfig_get();


  // Blink every interval ms
  uint32_t tnow = board_millis();
  if ( next_blink_ms > tnow) return; // not enough time

  if (led_blink_count < (2* (1 + boardconfig_selected_bitstream_slot()) )) {

	  if (led_blink_count == 0) {
		  led_state = 1;
	  }
	  led_blink_count++;
	  if (led_state == 0) {
		  next_blink_ms = SLOT_BLINKTIME_OFF_MS + tnow;
	  } else {
		  next_blink_ms = SLOT_BLINKTIME_ON_MS + tnow;
	  }
  } else {
	  led_blink_count = 0;
	  led_state = 0;
	  next_blink_ms = blink_interval_ms + tnow;
  }

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}
