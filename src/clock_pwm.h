/*
 * clock_pwm.h, part of the riffpga project
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

#ifndef CLOCK_PWM_H_
#define CLOCK_PWM_H_


#include "board_config.h"

bool clock_pwm_enable(FPGA_PWM * pwmconf);
void clock_pwm_disable(FPGA_PWM * pwmconf);
bool clock_pwm_set_freq(uint32_t freq_hz, FPGA_PWM * pwmconf);



#endif /* CLOCK_PWM_H_ */
