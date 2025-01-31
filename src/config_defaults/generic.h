/*
 * board_config_defaults.h, part of the riffpga project
 *
 * *One* of the config_defaults/*.h files winds up being
 * included in the project and setting up the defaults for
 * the target board.  From that point on, these are in the
 * BoardConfig struct that stored in flash and may be edited
 * using commands etc.
 *
 * The contents here apply
 *   * on fresh chips
 *   * on factoryreset
 *   * on burning firmware with a higher revision (subject to
 *     SYSTEM_NEWER_VERSION_AUTO_RESET)
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

#ifndef SRC_BOARD_CONFIG_DEFAULTS_GENERIC_H_
#define SRC_BOARD_CONFIG_DEFAULTS_GENERIC_H_

#define BOARD_NAME          "UF2FPGA"
#define DRIVE_VOLUME_LABEL  "FPGAUPDATE"


// RP2 clock frequency
#define BOARD_SYSTEM_CLOCK_FREQ_HZ		120000000


// RP2 pins -- as GPIO #
#define PIN_RP_LED			25

#define PIN_FPGA_RESET		4
// SPI needs to be a valid set of SPI lines
#define PIN_FPGA_SPI_MISO	0
#define PIN_FPGA_SPI_CS		1
#define PIN_FPGA_SPI_SCK	2
#define PIN_FPGA_SPI_MOSI	3
// CDONE-type pin
#define PIN_FPGA_PROG_DONE	9

// UART -- rx/tx from RP2's point of view
#define PIN_UART_BRIDGE_TX	8 /* TODO:FIXME */
#define PIN_UART_BRIDGE_RX	9 /* TODO:FIXME */


// only autoclock1 is currently used
#define PIN_AUTOCLOCK1		5 /* TODO:FIXME */
#define PIN_AUTOCLOCK2		6 /* TODO:FIXME */


// freq is in Hz
#define AUTOCLOCK1_DEFAULT_FREQ		12000000UL
#define AUTOCLOCK1_DEFAULT_ENABLE	1

#define AUTOCLOCK2_DEFAULT_FREQ		1000000UL
#define AUTOCLOCK2_DEFAULT_ENABLE	0


/*
 * FPGA_RESET_EXTERNALLY_TRIGGERED
 * Set to 1 if FPGA may be reset externally
 * and this may be monitored on PIN_FPGA_RESET
 */
#define FPGA_RESET_EXTERNALLY_TRIGGERED		0
#define FPGA_PROG_DONE_LEVEL	1 /* 1==HIGH on PIN_FPGA_PROG_DONE means program success */

#define FPGA_RESET_INVERTED 	1
#define FLASH_SPI_CS_INVERTED 	1
#define FLASH_SPI_POLARITY		0
#define FLASH_SPI_PHASE 		0
#define FLASH_SPI_BAUDRATE 			4000000UL


/*
 * UF2 magic numbers and family -- let's us
 * ignore irrelevant data, can be any uint32_t
 *
 * BIN_UF2_* are for the bitstream binary file.
 */
#define BIN_UF2_MAGIC_START1    0x951C0634UL // Randomly selected
#define BIN_UF2_MAGIC_END       0x1C73C401UL // Ditto
#define BIN_UF2_FAMILY_ID		0x6e2e91c1UL


/*
 * The remaining UF2 magiks are used internally, you
 * needn't change them, but may.
 */
// size marker wrapped in UF2
#define BITSTREAM_UF2_MAGIC_START1		0x951C0512UL
#define BITSTREAM_UF2_MAGIC_END			0xE4D951C0UL
#define BITSTREAM_UF2_FAMILY_ID			0x951C0FA3UL



// board config wrapped in UF2
#define BOARDCONF_UF2_MAGIC_START1		0x951C057EUL
#define BOARDCONF_UF2_MAGIC_END			0xC401061EUL
#define BOARDCONF_UF2_FAMILY_ID			0xB0A2DC0FUL


#define UART_BRIDGE_BAUDRATE 	115200
#define UART_BRIDGE_ENABLED		0
#define UART_BRIDGE_DEVICEIDX 	1
#define UART_BRIDGE_PIN_TX	 	8
#define UART_BRIDGE_PIN_RX	 	9

/*
 * UART_BRIDGE_ESCAPE_*
 * We need a way to break out of the uart
 * bridge.  Getting a sequence of these chars
 * should do it. Choose something you are
 * unlikely to transmit through the bridge.
 */
#define UART_BRIDGE_ESCAPE_SEQUENCE0	0x1b
#define UART_BRIDGE_ESCAPE_SEQUENCE1	0x03
#define UART_BRIDGE_ESCAPE_SEQUENCE2	0x04

/*
 * We have 4 slots for switches on the PCB, connected
 * to the RP2 (other than the BOOT switch).
 *
 * Calling them RESET, CLOCKING, USR1/2 here for
 * convenience, but you can set the _FUNCTION to anything.
 *
 * If there's a clock in there (..._FUCTION == 2), then
 * it may be held down on boot to force "manual clocking"
 * mode, during which each click will clock once.
 *
 */
#define USER_SWITCH_IDX_RESET		0
#define USER_SWITCH_IDX_CLOCKING	1
#define USER_SWITCH_IDX_USR1		2
#define USER_SWITCH_IDX_USR2		3

#define USER_SWITCH_RESET_PIN		15
#define USER_SWITCH_RESET_FUNCTION	1 /* 1 == Reset */
#define USER_SWITCH_RESET_INVERT	1
#define USER_SWITCH_RESET_PULL		0
#define USER_SWITCH_RESET_EDGE		GPIO_IRQ_EDGE_FALL


#define USER_SWITCH_CLOCK_PIN		24
#define USER_SWITCH_CLOCK_FUNCTION	2 /* 2 == Clocking */
#define USER_SWITCH_CLOCK_INVERT	0
#define USER_SWITCH_CLOCK_PULL		0
#define USER_SWITCH_CLOCK_EDGE		GPIO_IRQ_EDGE_RISE


#define USER_SWITCH_USR1_PIN		0 /* DNE */
#define USER_SWITCH_USR1_FUNCTION	0 /* 0 == NOTSET */
#define USER_SWITCH_USR1_INVERT		0
#define USER_SWITCH_USR1_PULL		0
#define USER_SWITCH_USR1_EDGE		0
#define USER_SWITCH_USR2_PIN		0 /* DNE */
#define USER_SWITCH_USR2_FUNCTION	0 /* 0 == NOTSET */
#define USER_SWITCH_USR2_INVERT		0
#define USER_SWITCH_USR2_PULL		0
#define USER_SWITCH_USR2_EDGE		0


/*
 * SYSTEM_NEWER_VERSION_AUTO_RESET
 * If this is 1, then burning a version of this system
 * with a greater major/minor/patch will automatically
 * "factory reset" the config data, using whatever the
 * defaults are (as specified here)
 */
#define SYSTEM_NEWER_VERSION_AUTO_RESET		1


/*
 * SYSTEM_INPUTS_NUM/SYSTEM_INPUTS_IO_LIST
 * RP2 GPIO that's treated as a list of inputs which may be
 * read/acted upon as a single value
 */
#define SYSTEM_INPUTS_NUM					4
#define SYSTEM_INPUTS_IO_LIST				16,17,18,19


/*
 * Flash storage:
 * We leave the first 0.5M for RP code.
 * After that we have:
 *  2 pages/slot for markers (much more than we need but the space is reserved)
 *  then a 512k block of flash space/slot
 *
 * Mess with this if you dare, huh.
 */
#define BITSTREAM_MANAGEMENT_START 			(512 * 1024)
#define MARKER_RESERVED_SPACE				(4*2*1024)
#define MARKER_SLOT_ADDRESS(slotidx)		(BITSTREAM_MANAGEMENT_START + ( (slotidx) * MARKER_RESERVED_SPACE))
#define BOARD_CONFIG_FLASHADDRESS			MARKER_SLOT_ADDRESS(0) - MARKER_RESERVED_SPACE

// bitstream storage base address -- assumes 4 slots available, 0-3
#define FLASH_TARGET_OFFSETSTART			MARKER_SLOT_ADDRESS(4)
#define BITSTREAM_SLOT_RESERVED_SPACE		(512 * 1024)
#define FLASH_STORAGE_STARTADDRESS(slotidx)	(FLASH_TARGET_OFFSETSTART + ( (slotidx) * BITSTREAM_SLOT_RESERVED_SPACE))




#endif /* SRC_BOARD_CONFIG_DEFAULTS_H_ */
