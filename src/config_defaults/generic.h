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
 * All the pin numbers within are RP2 GPIO numbers, so e.g. setting
 *  #define PIN_WHATEVER 10
 * would mean GPIO10 on an RP2040 (physical pin 13 on the QFN)
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

/*
 * The name of this board
 */
#define BOARD_NAME          "rif-generic"

/*
 * DRIVE_VOLUME_LABEL: what gets mounted as
 * the drive name
 */
#define DRIVE_VOLUME_LABEL  "FPGAUPDATE"


// Remember: RP2 pins -- as GPIO#

/*
 * Some LED connected to the RP--this is useful to
 * provide info about state program slot/unprogrammed.
 */
#define PIN_RP_LED			25

/*
 * Something we use to reset the FPGA.
 * Should have pull-up/pull-down as appropriate to
 * allow operation by default
 */
#define PIN_FPGA_RESET		4
/*
 * Assuming Lattice like CRAM that's basically
 * just SPI, here.
 * SPI needs to be a valid set of SPI lines
 * for the RP2.  See 1.4.3. GPIO Functions
 * in https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf
 *
 * The CS pin, in this case, or whatever the mechanism is
 * to tell the FPGA that it is a *slave* should have pull-up/pull-down
 * as appropriate to keep it in this mode by default.
 */
#define PIN_FPGA_SPI_MISO	0
#define PIN_FPGA_SPI_CS		1
#define PIN_FPGA_SPI_SCK	2
#define PIN_FPGA_SPI_MOSI	3
// CDONE-type pin, see FPGA_PROG_DONE_LEVEL below
#define PIN_FPGA_PROG_DONE	9



// only autoclock1 is currently used
#define PIN_AUTOCLOCK1		5 /* the RP2 pin we use for clocking the FPGA  */
#define PIN_AUTOCLOCK2		6 /* reserved for future, ignored for now */


// freq is in Hz
#define AUTOCLOCK1_DEFAULT_FREQ		12000000UL
#define AUTOCLOCK1_DEFAULT_ENABLE	1

#define AUTOCLOCK2_DEFAULT_FREQ		1000000UL
#define AUTOCLOCK2_DEFAULT_ENABLE	0

// RP2 clock frequency
// TinyUSB wants it to be 120M but it's
// worked fine from 100 to 126 so far... still
#define BOARD_SYSTEM_CLOCK_FREQ_HZ		120000000UL



/*
 * FPGA_RESET_EXTERNALLY_TRIGGERED
 * Set to 1 if FPGA may be reset externally
 * and this may be monitored on PIN_FPGA_RESET
 */
#define FPGA_RESET_EXTERNALLY_TRIGGERED		0
#define FPGA_RESET_INVERTED 	1

#define FLASH_SPI_CS_INVERTED 	1 /* 1==CS LOW means selected */
#define FLASH_SPI_POLARITY		0
#define FLASH_SPI_PHASE 		0
#define FLASH_SPI_BAUDRATE 			4000000UL


/*
 * If you have a "Done" pin hooked-up,
 * FPGA_PROG_DONE_LEVEL lets the system know what means "programmed".
 * If you don't, and want to ignore all that, comment out the #define
 * and the code will just assume the FPGA's been programmed as
 * appropriate.
 */
#define FPGA_PROG_DONE_LEVEL	1 /* 1==HIGH on PIN_FPGA_PROG_DONE means program success */

/*
 * UART bridge defaults.
 * The UART bridge, when enabled, will shuttle all the
 * data coming in over to the FPGA through UART_BRIDGE_PIN_TX.
 * Anything coming to it over UART_BRIDGE_PIN_RX gets sent back
 * over the USB serial connection.
 * The way to get out of this mode: see UART_BRIDGE_ESCAPE_SEQUENCE*
 */
#define UART_BRIDGE_BAUDRATE 	115200
#define UART_BRIDGE_ENABLED		0 /* probably best to default to 0==off */

// The TX/RX pin, as seen from the RP2's point of view
#define UART_BRIDGE_PIN_TX	 	8 /* an RP2 pin you can use as a UART TX */
#define UART_BRIDGE_PIN_RX	 	9 /* an RP2 pin you can use as a UART RX */

// UART_BRIDGE_DEVICEIDX -- the RP uart0/uart1 index we are
// using.  Set the index according to the selected pins
#define UART_BRIDGE_DEVICEIDX 	1

/*
 * UART_BRIDGE_ESCAPE_*
 * We need a way to break out of the uart
 * bridge.  Getting a sequence of these chars
 * should do it. Choose something you are
 * unlikely to transmit through the bridge.
 */
#define UART_BRIDGE_ESCAPE_SEQUENCE0	0x1b
#define UART_BRIDGE_ESCAPE_SEQUENCE1	0x1b
#define UART_BRIDGE_ESCAPE_SEQUENCE2	0x1b




/*
 * UF2 magic numbers and family -- let's us
 * ignore irrelevant data, can be any uint32_t
 *
 * BIN_UF2_* are for the bitstream binary file,
 * the thing that gets sent over to configure the
 * FPGA.
 */
#define BIN_UF2_MAGIC_START1    0x951C0634UL // Randomly selected
#define BIN_UF2_MAGIC_END       0x1C73C401UL // Ditto
#define BIN_UF2_FAMILY_ID		0x6e2e91c1UL




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
 * The remaining UF2 magiks are used internally, you
 * needn't change them, but may if you please.
 */
// size marker wrapped in UF2
#define BITSTREAM_UF2_MAGIC_START1		0x951C0512UL
#define BITSTREAM_UF2_MAGIC_END			0xE4D951C0UL
#define BITSTREAM_UF2_FAMILY_ID			0x951C0FA3UL



// board config wrapped in UF2
#define BOARDCONF_UF2_MAGIC_START1		0x951C057EUL
#define BOARDCONF_UF2_MAGIC_END			0xC401061EUL
#define BOARDCONF_UF2_FAMILY_ID			0xB0A2DC0FUL



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
