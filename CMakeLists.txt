cmake_minimum_required(VERSION 3.17)

# To build:
# mkdir build
# cd build
# cmake -DPICO_SDK_PATH=/path/to/pico-sdk -DTARGET_GENERIC=ON ..
#   note: valid targets are currently
#	 TARGET_GENERIC
#	 TARGET_PSYDMI and
# 	 TARGET_EFABLESS_EXPLAIN
#  ... probably a better way to do this.

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
OPTION(PICO_SDK_PATH "Path to pico SDK")



OPTION(TARGET_PSYDMI "Build for PsyDMI" OFF)
OPTION(TARGET_GENERIC "Build generic" OFF)
OPTION(TARGET_EFABLESS_EXPLAIN "Build for efab explain" OFF)


set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)




set(FAMILY rp2040)
set(BOARD pico_sdk)
set(TINYUSB_FAMILY_PROJECT_NAME_PREFIX "tinyusb_dev_")
# Provide an LWIP path for net_lwip_webserver in case the module does not exist in tinyusb
set(TINYUSB_LWIP_PATH ${PICO_LWIP_PATH})
# Some examples use this, and we need to set this here due to a bug in the TinyUSB CMake config
set(TOP ${PICO_TINYUSB_PATH})



set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

if (TARGET_EFABLESS_EXPLAIN)
	add_compile_definitions(TARGET_EFABLESS_EXPLAIN)
	set(PROJ_SUFFIX "efabless")
endif(TARGET_EFABLESS_EXPLAIN)

if (TARGET_GENERIC)
	add_compile_definitions(TARGET_GENERIC)
	set(PROJ_SUFFIX "generic")
endif(TARGET_GENERIC)

if (TARGET_PSYDMI)
	add_compile_definitions(TARGET_PSYDMI)
	set(PROJ_SUFFIX "psydmi")
endif(TARGET_PSYDMI)

set(PROJNAME "ASICSim_${PROJ_SUFFIX}")
set(EXENAME "ASICSim_${PROJ_SUFFIX}")
project(${PROJNAME} C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()



add_executable(${EXENAME})
target_include_directories(${EXENAME} PRIVATE src)
# Example source
target_sources(${EXENAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/msc_disk.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/usb_descriptors.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ghostfat.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/board.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/cdc_interface.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/bitstream.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/fpga.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/board_config.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/uart_bridge.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/clock_pwm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/io_inputs.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sui/sui_util.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sui/sui_command.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sui/sui_handler.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sui/commands/clocking.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sui/commands/config.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sui/commands/dump.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sui/commands/fpga.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sui/commands/io.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sui/commands/uart.c
  
  )

# In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_device
# for TinyUSB device support and tinyusb_board for the additional board support library used by the example
target_link_libraries(${EXENAME} PUBLIC pico_stdlib 
			pico_unique_id tinyusb_device tinyusb_board 
			hardware_clocks 
			hardware_pwm
			hardware_spi hardware_flash
			hardware_uart 
			hardware_watchdog
			)

# Uncomment this line to enable fix for Errata RP2040-E5 (the fix requires use of GPIO 15)
#target_compile_definitions(dev_hid_composite PUBLIC PICO_RP2040_USB_DEVICE_ENUMERATION_FIX=1)

pico_add_extra_outputs(${EXENAME})

