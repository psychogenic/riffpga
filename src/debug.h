/*
 * debug.h, part of the riffpga project
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

#ifndef ASICSIM_DEBUG_H_
#define ASICSIM_DEBUG_H_

#include "cdc_interface.h"

//define DEBUG_OUTPUT_ENABLED

// define FORCE_DEBUG_BUF(b, len) cdc_write(b, len)
#ifdef DEBUG_OUTPUT_ENABLED
#define DEBUG_ENDLN()	cdc_write("\n",1)
#define DEBUG_BUF(b, len) cdc_write(b, len)
#define DEBUG(s)	cdc_write(s, sizeof(s))
#define DEBUG_LN(s)	cdc_write(s, sizeof(s)) ; DEBUG_ENDLN()

#define DEBUG_U32(v) cdc_write_u32(v)
#define DEBUG_U32_LN(v) DEBUG_U32(v); DEBUG_ENDLN()
#define DEBUG_U16(v) cdc_write_u16(v)
#define DEBUG_U16_LN(v) DEBUG_U16(v); DEBUG_ENDLN()
#define DEBUG_U8(v) cdc_write_u8(v)
#define DEBUG_U8_LN(v) DEBUG_U8(v); DEBUG_ENDLN()

#else

#define DEBUG_ENDLN()
#define DEBUG_BUF(b, len)
#define DEBUG(s)
#define DEBUG_LN(s)

#define DEBUG_U32(v)
#define DEBUG_U32_LN(v)
#define DEBUG_U16(v)
#define DEBUG_U16_LN(v)
#define DEBUG_U8(v)
#define DEBUG_U8_LN(v)

#endif /* DEBUG_OUTPUT_ENABLED */


#endif
