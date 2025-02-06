#!/usr/bin/env python
'''
    If you setup the system with a "weird" breakout for the 
    UART bridge, rather than the default "ESCAPE ESCAPE ESCAPE", 
    this script will shoot over the write bytes to get out of 
    bridge-mode.
'''
# serial port
serial_port = '/dev/ttyACM0'
# breakout sequence
breakout = b'\x1b\x03\x04\x0a'

import serial
import time

def do_breakout():
    try:
        ser = serial.Serial(serial_port)
    except:
        print(f"Could not open serial port {serial_port}? Aborting.")
        return
    print(f"Writing breakout sequence ({breakout}) to {serial_port}")
    ser.write(breakout)
    time.sleep(0.5)
    
if __name__ == '__main__':
    do_breakout()

