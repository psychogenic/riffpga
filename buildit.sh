#!/bin/bash
PICO_SDK_PATH=/path/to/pico-sdk
TARG=TARGET_GENERIC
# TARG=TARGET_EFABLESS_EXPLAIN
#TARG=TARGET_PSYDMI
mkdir build
cd build

# cmake -DPICO_SDK_PATH=/storage/malcalypse/dev/pico/pico-sdk -DTARGET_PSYDMI=ON .. 
cmake -DPICO_SDK_PATH=$PICO_SDK_PATH -D${TARG}=ON .. 
make
