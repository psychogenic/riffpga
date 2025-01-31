/*
 * io_inputs.c, part of the riffpga project
 *
 *  Created on: Jan 22, 2025
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

#include "io_inputs.h"
#include "board_config.h"
#include "debug.h"

typedef struct switchconfstruct {
	bool enabled;
	uint8_t function;
	uint8_t pin;
	uint8_t inverted;
} SwitchConfigCache;

static SwitchConfigCache _sw_config[BOARD_MAX_NUM_SWITCHES] = { 0 };
static uint8_t _sw_manual_clock_idx = 0;

static volatile bool _sw_interrupt[BOARD_MAX_NUM_SWITCHES] = { false };

// static char event_str[128];
static void sw_interrupt_triggered(void) {

	for (uint8_t i = 0; i < BOARD_MAX_NUM_SWITCHES; i++) {
		uint32_t irqmask = gpio_get_irq_event_mask(_sw_config[i].pin);

		gpio_acknowledge_irq(_sw_config[i].pin, irqmask);
		if (irqmask) {
			// handle
			if (irqmask & GPIO_IRQ_EDGE_RISE) {
				_sw_interrupt[i] = true;
			}
		}
	}
}

volatile bool io_switch_interrupted(uint8_t idx) {
	return _sw_interrupt[idx];
}
void io_switch_interrupt_clear(uint8_t idx) {
	_sw_interrupt[idx] = false;
}

bool io_manualclock_switch_interrupted() {
	return io_switch_interrupted(_sw_manual_clock_idx);

}
void io_manualclock_switch_interrupt_clear() {
	io_switch_interrupt_clear(_sw_manual_clock_idx);
}

bool io_switch_state(uint8_t idx) {
	if (!_sw_config[idx].inverted) {
		return gpio_get(_sw_config[idx].pin) ? true : false;
	}

	return gpio_get(_sw_config[idx].pin) ? false : true;

}
bool io_manualclock_switch_state() {
	if (_sw_config[_sw_manual_clock_idx].enabled == false) {
		DEBUG_LN("NOT ENABLED!");
		return false;
	}
	return io_switch_state(_sw_manual_clock_idx);
}

uint8_t io_switches_init(void) {

	BoardConfigPtrConst bconf = boardconfig_get();
	uint8_t num = 0;
	for (uint8_t i = 0; i < BOARD_MAX_NUM_SWITCHES; i++) {
		_sw_config[i].function = bconf->switches[i].function;
		if (bconf->switches[i].function == SwitchFunctionNOTSET) {
			_sw_config[i].enabled = false;
			continue;
		}

		if (bconf->switches[i].function == SwitchFunctionClocking) {
			_sw_manual_clock_idx = i;
		}
		_sw_config[i].inverted = bconf->switches[i].inverted;
		_sw_config[i].enabled = true;
		_sw_config[i].pin = bconf->switches[i].pin;

		gpio_init(_sw_config[i].pin);
		gpio_set_dir(_sw_config[i].pin, GPIO_IN);
		//gpio_set_irq_enabled_with_callback(_sw_config[i].pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
		// 		  true, sw_interrupt_triggered);

		gpio_add_raw_irq_handler(_sw_config[i].pin, sw_interrupt_triggered);

		gpio_set_irq_enabled(_sw_config[i].pin, GPIO_IRQ_EDGE_RISE, true);
		num++;
	}

	return num;

}

void io_inputs_init(void) {

	BoardConfigPtrConst bconf = boardconfig_get();

	if (!bconf->system.num_inputs) {

		return;
	}

	for (uint8_t i; i < bconf->system.num_inputs; i++) {
		gpio_init(bconf->system.input_io[i]);
		gpio_set_dir(bconf->system.input_io[i], GPIO_IN);
		gpio_pull_up(bconf->system.input_io[i]);
	}

}
uint16_t io_inputs_value(void) {
	uint16_t retVal = 0;
	BoardConfigPtrConst bconf = boardconfig_get();

	if (!bconf->system.num_inputs) {
		DEBUG_LN("No inputs configured!");
		return 0;
	}

	for (uint8_t i; i < bconf->system.num_inputs; i++) {
		if (gpio_get(bconf->system.input_io[i])) {
			retVal |= (1 << (i));
		}

	}

	return retVal;

}
