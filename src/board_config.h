/*
 * board_config.h, part of the riffpga project
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

#ifndef SRC_BOARD_CONFIG_STRUCTS_H_
#define SRC_BOARD_CONFIG_STRUCTS_H_




#include "board_includes.h"

#define RIF_PACKED_STRUCT __attribute__((packed))
// #define RIF_PACKED_STRUCT


#define BOARD_NAME_CHARS	23
#define POSITION_SLOTS_NUM		4
#define POSITION_SLOTS_ALLOWED	3

// 4*4 = 16 bytes
typedef struct RIF_PACKED_STRUCT uf2_magic_struct {

	uint32_t magic_start; // start1
	uint32_t magic_end;
	uint32_t family_id;
	uint32_t res1; // reserved

} UF2_MagicInfo;


// 5*4 = 20 bytes
typedef struct RIF_PACKED_STRUCT storage_slot_struct {
	uint32_t slot_start_address[POSITION_SLOTS_NUM];
	uint16_t res1;
	uint8_t  selected_slot;
	uint8_t  res2;
} StorageSlotInfo;

typedef struct RIF_PACKED_STRUCT version_info_struct {
	uint8_t conf_struct; // 4 bytes.
	uint8_t major;
	uint8_t minor;
	uint8_t patchlevel;
} VersionInfo ;

// 4*4 = 16 bytes
typedef struct RIF_PACKED_STRUCT fpga_pwm_drive_struct {
	uint32_t freq_hz;
	uint32_t top;
	uint32_t div;
	uint16_t duty;
	uint8_t enabled;
	uint8_t pin;
} FPGA_PWM;

// 12 bytes
#define UART_BREAKOUT_SEQUENCE_CHARS_MAX	3

typedef struct RIF_PACKED_STRUCT uart_config_struct {
	uint32_t baud;
	uint8_t enabled;
	uint8_t uartnum;
	uint8_t pin_tx;
	uint8_t pin_rx;
	uint8_t breakout_sequence[UART_BREAKOUT_SEQUENCE_CHARS_MAX + 1];
} UART_Bridge;
// 12 bytes
typedef struct RIF_PACKED_STRUCT spi_config_struct {
	uint32_t rate;
	uint8_t cs_inverted;
	uint8_t phase;
	uint8_t polarity;
	uint8_t order;
	uint8_t pin_cs;
	uint8_t pin_sck;
	uint8_t pin_miso;
	uint8_t pin_mosi;
} SPIConfig;

// 16 bytes
typedef struct RIF_PACKED_STRUCT fpga_cram_config {
	SPIConfig spi; // 12
	uint8_t res1;
	uint8_t pin_done;
	uint8_t pin_reset;
	uint8_t reset_inverted;
} FPGACRAMConfig;

typedef enum switchfunctionenum {
	SwitchFunctionNOTSET=0,
	SwitchFunctionReset=1,
	SwitchFunctionClocking=2,
	SwitchFunctionUser=3
} SwitchFunction;

typedef struct RIF_PACKED_STRUCT userswitchinfostruct {
	uint8_t function;
	uint8_t pin;
	uint8_t pull;
	uint8_t inverted;
	uint8_t irq_edge;
	uint8_t res1;
	uint16_t res2;
} UserSwitch;

#define BOARD_MAX_NUM_SWITCHES	4
#define BOARD_MAX_NUM_INPUTS	16

// 24 bytes
typedef struct RIF_PACKED_STRUCT syssettingsstruct {
	uint32_t 	clock_freq_hz;
	uint8_t		auto_reset_on_newer_version;
	uint8_t		fpga_reset_external_trigger;
	uint8_t 	reserved;
	uint8_t		num_inputs;
	uint8_t		input_io[BOARD_MAX_NUM_INPUTS];
} SystemSettings;


typedef struct RIF_PACKED_STRUCT   board_config_struct {
	char board_name[BOARD_NAME_CHARS+1]; // 24 bytes

	VersionInfo version; 				// 4 bytes

	SystemSettings 	system; 			// 24 bytes

	UF2_MagicInfo bin_download; 		// 16
	StorageSlotInfo bin_position; 		// 20
	UF2_MagicInfo bs_marker; 			// 16
	StorageSlotInfo bs_marker_position; // 20
	FPGACRAMConfig	fpga_cram;			// 16
	FPGA_PWM clocking[2];				// 16*2 = 32

	UART_Bridge uart_bridge; 			// 12

	UserSwitch switches[BOARD_MAX_NUM_SWITCHES];// 8*4 = 32
	uint8_t user_app_data[8]; 			// 8, free to use by applications, won't be touched by low-level
	uint8_t reserved[64];				// 64 for future expansions, without impact to user payload below
										// -----
										// 264, so 476 - 264 = 212 free bytes in payload
} BoardConfig ;

typedef const BoardConfig const * BoardConfigPtrConst;

void boardconfig_init(void);
void boardconfig_factoryreset(void);

BoardConfigPtrConst boardconfig_get(void);

bool boardconfig_version_outdated(void);
bool boardconfig_version_mismatch(void);





uint32_t boardconfig_bin_startoffset(void);

uint32_t boardconfig_bin_startoffset(void);
uint32_t boardconfig_bs_marker_address(void);
uint32_t boardconfig_bs_marker_address_for(uint8_t slotidx);

void boardconfig_set_systemclock_hz(uint32_t v);
void boardconfig_autoclock_enable();
void boardconfig_autoclock_disable();
void boardconfig_set_autoclock_hz(uint32_t v);


uint8_t boardconfig_selected_bitstream_slot(void);
void boardconfig_set_bitstream_slot(uint8_t s);

void boardconfig_uartbridge_enable();
void boardconfig_uartbridge_disable();
void boardconfig_set_uartbridge_baudrate(uint32_t v);


void boardconfig_write(void);
void boardconfig_dump(void);



FPGA_PWM * boardconfig_autoclocking(uint8_t idx);

#endif /* SRC_BOARD_CONFIG_H_ */
