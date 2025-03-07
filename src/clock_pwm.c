/*
 * clock_pwm.c, part of the riffpga project
 *
 *  Created on: Jan 6, 2025
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

#include "hardware/pwm.h"
#include "board_includes.h"
#include "debug.h"
#include "clock_pwm.h"

// Returns: floor((16*F + offset) / div16)
// Avoids overflow in the numerator that would occur if
//   16*F + offset > 2**32
//   F + offset/16 > 2**28 = 268435456 (approximately, due to flooring)
uint32_t get_slice_hz(uint32_t offset, uint32_t div16) {
	uint32_t source_hz = clock_get_hz(clk_sys);
	if (source_hz + offset / 16 > 268000000) {
		return (16 * (uint64_t) source_hz + offset) / div16;
	} else {
		return (16 * source_hz + offset) / div16;
	}
}

// Returns 16*F / denom, rounded.
uint32_t get_slice_hz_round(uint32_t div16) {
	return get_slice_hz(div16 / 2, div16);
}

// Returns ceil(16*F / denom).
uint32_t get_slice_hz_ceil(uint32_t div16) {
	return get_slice_hz(div16 - 1, div16);
}

void clock_once(FPGA_PWM *pwmconf) {
	DEBUG_LN("clock-once");
	if (pwmconf->enabled) {
		DEBUG_LN("clock-once called but pwm enabled... disabling.");
		clock_pwm_disable(pwmconf);
		gpio_set_function(pwmconf->pin, GPIO_FUNC_NULL);
		gpio_set_dir(pwmconf->pin, GPIO_OUT);

		gpio_put(pwmconf->pin, 0);

	}
	gpio_put(pwmconf->pin, 1);
	sleep_us(50);
	gpio_put(pwmconf->pin, 0);
	sleep_us(50);

}
bool clock_pwm_enable(FPGA_PWM *pwmconf) {

	uint slice = pwm_gpio_to_slice_num(pwmconf->pin);
	uint8_t channel = pwm_gpio_to_channel(pwmconf->pin);
	// Select PWM function for given GPIO.
	gpio_set_function(pwmconf->pin, GPIO_FUNC_PWM);
	pwm_config defconf = pwm_get_default_config();

	pwm_init(slice, &defconf, true);
	// pwm_set_enabled(slice, true);

	pwmconf->enabled = 1;
	DEBUG_LN("PWM enable");
	return true;
}
void clock_pwm_disable(FPGA_PWM *pwmconf) {

	uint slice = pwm_gpio_to_slice_num(pwmconf->pin);
	// gpio_set_function(gpio, GPIO_FUNC_PWM);
	pwm_set_enabled(slice, false);
	pwmconf->enabled = 0;
	return;

}

bool clock_pwm_set_freq3(uint32_t freq_hz, FPGA_PWM *pwmconf) {
	uint slice_num = pwm_gpio_to_slice_num(pwmconf->pin);
	uint chan = pwm_gpio_to_channel(pwmconf->pin);
	uint32_t clock = clock_get_hz(clk_sys);
	uint32_t divider16 = clock / freq_hz / 4096 + (clock % (freq_hz * 4096) != 0);



	if (divider16 < 16) {
		divider16 = 16;
	}


	if (divider16 >= 256 * 16) {

		DEBUG_LN("freq too small");
		return false;
	}

	uint32_t wrap = clock * 16 / divider16 / freq_hz - 1;



	pwmconf->top = wrap;
	pwmconf->div = divider16;
	pwmconf->freq_hz = freq_hz;



	pwm_set_clkdiv_int_frac(slice_num, pwmconf->div / 16, pwmconf->div & 0xF);
	pwm_set_wrap(slice_num, pwmconf->top);
	pwm_set_chan_level(slice_num, chan, pwmconf->top / 2);
	pwm_set_enabled(slice_num, true);


#ifdef DEBUG_OUTPUT_ENABLED
	DEBUG("PWM configured with:\r\n\t top:");
	cdc_write_dec_u32_ln(pwmconf->top);
	DEBUG("\t div:");
	cdc_write_dec_u32_ln(pwmconf->div);
	DEBUG("\t level:");
	cdc_write_dec_u32_ln(wrap/2);
	DEBUG("\t freq:");
	cdc_write_dec_u32(pwmconf->freq_hz);
	DEBUG(" at sysclk ");
	cdc_write_dec_u32_ln(clock);
	CDCWRITEFLUSH();

#endif
	if (wrap) {
		return true;
	}
	return false;
}


#define PWM_TOP_MAX 65534
bool clock_pwm_set_freq(uint32_t freq_hz, FPGA_PWM *pwmconf) {
	uint slice_num = pwm_gpio_to_slice_num(pwmconf->pin);
	uint chan = pwm_gpio_to_channel(pwmconf->pin);

	uint32_t source_hz = clock_get_hz(clk_sys);
	uint32_t divider16;
	uint32_t wrap = 0;
	if ((source_hz + freq_hz / 2) / freq_hz < PWM_TOP_MAX) {
		// If possible (based on the formula for TOP below), use a DIV of 1.
		// This also prevents overflow in the DIV calculation.
		divider16 = 16;

		// Same as get_slice_hz_round() below but canceling the 16s
		// to avoid overflow for high freq.
		wrap = (source_hz + freq_hz / 2) / freq_hz - 1;
	} else {
		// Otherwise, choose the smallest possible DIV for maximum
		// duty cycle resolution.
		// Constraint: 16*F/(div16*freq) < TOP_MAX
		// So:
		divider16 = get_slice_hz_ceil(PWM_TOP_MAX * freq_hz);

		// Set TOP as accurately as possible using rounding.
		wrap = get_slice_hz_round(divider16 * freq_hz) - 1;
	}


	if (divider16 < 16) {
		divider16 = 16;
	}


	if (divider16 >= 256 * 16) {

		DEBUG_LN("freq too small");
		return false;
	}

	if (wrap < 2) {
		if (divider16 == 16) {
			CDCWRITESTRING("Frequency too high -- setting to max");
			wrap = 2;
		}
	}


	pwmconf->top = wrap;
	pwmconf->div = divider16;
	pwmconf->freq_hz = freq_hz;



	pwm_set_clkdiv_int_frac(slice_num, pwmconf->div / 16, pwmconf->div & 0xF);
	pwm_set_wrap(slice_num, pwmconf->top);
	pwm_set_chan_level(slice_num, chan, pwmconf->top / 2);
	pwm_set_enabled(slice_num, true);


#ifdef DEBUG_OUTPUT_ENABLED
	DEBUG("PWM configured with:\r\n\t top:");
	cdc_write_dec_u32_ln(pwmconf->top);
	DEBUG("\t div:");
	cdc_write_dec_u32_ln(pwmconf->div);
	DEBUG("\t level:");
	cdc_write_dec_u32_ln(wrap/2);
	DEBUG("\t freq:");
	cdc_write_dec_u32(pwmconf->freq_hz);
	DEBUG(" at sysclk ");
	cdc_write_dec_u32_ln(source_hz);
	CDCWRITEFLUSH();

#endif

	return true;
}


float clock_pwm_freq_achieved(FPGA_PWM * pwmconf) {
	float fact = pwmconf->top * ((pwmconf->div/16.00f) + ((1.0 * (pwmconf->div & 0xF))/16.00));
	return clock_get_hz(clk_sys)/fact;

}


bool clock_pwm_set_freq2(uint32_t freq_hz, FPGA_PWM *pwmconf) {
	// Set the frequency, making "top" as large as possible for maximum resolution.
	// Maximum "top" is set at 65534 to be able to achieve 100% duty with 65535.
#define TOP_MAX 65534

	uint slice = pwm_gpio_to_slice_num(pwmconf->pin);
	uint8_t channel = pwm_gpio_to_channel(pwmconf->pin);

	uint32_t source_hz = clock_get_hz(clk_sys);
	uint32_t div16;
	uint32_t top;

	if ((source_hz + freq_hz / 2) / freq_hz < TOP_MAX) {
		// If possible (based on the formula for TOP below), use a DIV of 1.
		// This also prevents overflow in the DIV calculation.
		div16 = 16;

		// Same as get_slice_hz_round() below but canceling the 16s
		// to avoid overflow for high freq.
		top = (source_hz + freq_hz / 2) / freq_hz - 1;
	} else {
		// Otherwise, choose the smallest possible DIV for maximum
		// duty cycle resolution.
		// Constraint: 16*F/(div16*freq) < TOP_MAX
		// So:
		div16 = get_slice_hz_ceil(TOP_MAX * freq_hz);

		// Set TOP as accurately as possible using rounding.
		top = get_slice_hz_round(div16 * freq_hz) - 1;
	}

	if (div16 < 16) {
		DEBUG_LN("freq too large");
		return false;
	} else if (div16 >= 256 * 16) {

		DEBUG_LN("freq too small");
		return false;
	}

	pwmconf->div = div16;
	pwmconf->top = top;
	pwmconf->enabled = 1;
	pwmconf->freq_hz = freq_hz;
	uint16_t duty_u16 = pwmconf->duty;
	uint32_t cc = (duty_u16 * (top + 1) + 0xffff / 2) / 0xffff;
	pwm_config config = pwm_get_default_config();



	pwm_set_wrap(slice, top);
	pwm_set_chan_level(slice, channel, cc);
	pwm_set_enabled(slice, true);

#ifdef DEBUG_OUTPUT_ENABLED
	DEBUG("PWM configured with:\r\n\t top:");
	cdc_write_dec_u32_ln(pwmconf->top);
	DEBUG("\t div:");
	cdc_write_dec_u32_ln(pwmconf->div);
	DEBUG("\t level:");
	cdc_write_dec_u32_ln(cc);
	DEBUG("\t div:");
	cdc_write_dec_u32_ln(pwmconf->div);
	DEBUG("\t freq:");
	cdc_write_dec_u32(pwmconf->freq_hz);
	DEBUG(" at sysclk ");
	cdc_write_dec_u32_ln(source_hz);

#endif


	return true;

}
