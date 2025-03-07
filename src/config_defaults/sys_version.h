 /*
 * sys_version.h, part of the riffpga project
 *
 * Incrementing this version will automatically overwrite the config
 * blob on targets with the set of defaults.
 *
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

#ifndef CONFIG_DEFAULTS_SYSVERSION_H_
#define CONFIG_DEFAULTS_SYSVERSION_H_


#define BOARD_VERSION_MAJOR		1
#define BOARD_VERSION_MINOR		1
#define BOARD_VERSION_PATCH		5

#define BRD_STR_HELPER(x) #x
#define BRD_STR(x) BRD_STR_HELPER(x)

#define BOARD_VERSION_STR		BRD_STR(BOARD_VERSION_MAJOR) "." \
								BRD_STR(BOARD_VERSION_MINOR) "." \
								BRD_STR(BOARD_VERSION_PATCH)


#endif
