#!/usr/bin/env python

'''
Created on Dec 17, 2024

@author: Pat Deegan
@copyright: Copyright (C) 2024 Pat Deegan, https://psychogenic.com
'''


import argparse
from uf2utils.file import UF2File
from uf2utils.family import Family
import random

class UF2Settings:
    
    def __init__(self, name:str, 
                 familyDesc:str, 
                 familyId:int, 
                 magicstart:int, 
                 magicend:int):
        self.name = name
        self.description = familyDesc
        self.boardFamily = familyId
        self.magicStart1 = magicstart
        self.magicEnd = magicend

TargetOptions = {
    
    'generic': UF2Settings(
                    name='Generic',
                    familyDesc='Generic/Sample build',
                    familyId=0x6e2e91c1,
                    magicstart=0x951C0634,
                    magicend= 0x1C73C401),

    'efabless': UF2Settings(
                    name='EfabExplain',
                    familyDesc='Efabless ASIC Sim 2',
                    familyId=0xefab1e55,
                    magicstart=0x951C0634,
                    magicend= 0x1C73C401),
    
    'psydmi': UF2Settings(
            name='PsyDMI',
            familyDesc='PsyDMI driver',
            familyId=0xD31B02AD,
            magicstart=0x951C0634,
            magicend=0x1C73C401)
}


base_bitstream_storage_page = 544
page_blocks = 4 # 4 k per page
reserved_pages_for_bitstream_slot = int(512/page_blocks)
max_pages_for_bitstream = int(128/page_blocks)
base_page = int(544/page_blocks)

def get_args():
    parser = argparse.ArgumentParser(
                    description='Convert bitstream .bin to .uf2 file to use with riffpga',
                    epilog='Copy the resulting UF2 over to the mounted FPGAUpdate drive')
    
    targetList = list(TargetOptions.keys())
                      
    parser.add_argument('--target', required=False, type=str, 
                        choices=targetList,
                        default=targetList[0],
                        help=f'Target board [{targetList[0]}]')
    parser.add_argument('--slot', required=False, type=int, 
                        default=1,
                        help='Slot (1-3) [1]')
    parser.add_argument('infile',
                        help='input bitstream')
    parser.add_argument('outfile', help='output UF2 file')
    
    return parser.parse_args()
def get_payload_contents(infilepath:str):
    
    # return whatever you want in here
    with open(infilepath, 'rb') as infile:
        bts = infile.read()
    
    return bts

def get_new_uf2(settings:UF2Settings):
    
    myBoard = Family(id=settings.boardFamily, name=settings.name, description=settings.description)
    uf2 = UF2File(board_family=myBoard.id, fill_gaps=False, magic_start=settings.magicStart1, 
                  magic_end=settings.magicEnd)
    # uf2.header.flags is already Flags.FamilyIDPresent
    # you may want to add more, but if you do, probably good 
    # to preserve the FamilyIDPresent bit
    
    return uf2


def main():
    
    args = get_args()
    
    if args.slot < 1 or args.slot > 4:
        print("Select a slot between 1-4")
        return 
    
    slotidx = args.slot - 1
    
    # stick it somewhere within its slot
    page = random.randint(0, reserved_pages_for_bitstream_slot-max_pages_for_bitstream)
    page += reserved_pages_for_bitstream_slot * slotidx
    page += base_page
    
    uf2sets = TargetOptions[args.target]
    
    uf2 = get_new_uf2(uf2sets)
    payload_bytes = get_payload_contents(args.infile)
    
    start_offset = page*page_blocks*1024
    uf2.append_payload(payload_bytes, 
                       start_offset=start_offset, 
                       block_payload_size=256)
                       
    
    uf2.to_file(args.outfile)
    print(f"Generated UF2 for slot {args.slot}, starting at address {hex(start_offset)}")
    print(f"It now available at {args.outfile}")
    

if __name__ == "__main__":
    main()
    
