#!/usr/bin/env python
'''
Created on Dec 17, 2024

@author: Pat Deegan
@copyright: Copyright (C) 2024 Pat Deegan, https://psychogenic.com
'''

import sys
import os.path
import argparse
import struct
UF2UtilsPresent = True
try:
    from uf2utils.file import UF2File
    from uf2utils.family import Family
    from uf2utils.block import Header, DataBlock
    from uf2utils.constants import Flags
except ModuleNotFoundError:
    UF2UtilsPresent = False
    
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

# these values must be in agreement with the 
# riffpga settings running on the target board
base_bitstream_storage_address_kb = 544
reserved_kb_for_bitstream_slot = 512

metadata_start1_offset  = 0x42
metadata_payload_header = "RFMETA"
metadata_payload_version = "01"
metadata_proj_name_maxlen = 23

factoryreset_start1_offset = 0xdead
factoryreset_payload_header = "RFRSET"



# derived values
page_blocks = 4 # 4 k per page, this has to align for flash reasons
reserved_pages_for_bitstream_slot = int(reserved_kb_for_bitstream_slot/page_blocks)
base_page = int(base_bitstream_storage_address_kb/page_blocks)


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
    parser.add_argument('--name', required=False, type=str,
                        default='',
                        help='Pretty name for bitstream')
    parser.add_argument('--autoclock', required=False, type=int,
                        default=0,
                        help='Auto-clock preference for project, in Hz [10-40M]')
                        
        
    parser.add_argument('--appendslot', required=False,
                        action='store_true',
                        help='Append to slot to output file name')
                        
    parser.add_argument('--factoryreset', required=False,
                        action='store_true',
                        help='Ignore other --args, just create a factory reset packet of death')
                        
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


def get_metadata_block(settings:UF2Settings, flash_address:int, bitstreamSize:int, autoclock:int, 
    filename:str, bitstreamName:str=None):
    if bitstreamName is None or not len(bitstreamName):
        extsplit = os.path.splitext(filename)
        if extsplit and len(extsplit) > 1:
            bitstreamName = os.path.basename(filename).replace(extsplit[1], '')
        else:
            bitstreamName = os.path.basename(filename)

    bsnamelenmax = metadata_proj_name_maxlen
    bsnamelen = len(bitstreamName)
    if bsnamelen > bsnamelenmax:
        bitstreamName = bitstreamName[:bsnamelenmax]
        bsnamelen = bsnamelenmax

    bsnameArray = bytes(bitstreamName, encoding='ascii')
    if bsnamelen < bsnamelenmax:
        bsnameArray += bytearray(bsnamelenmax - bsnamelen)
        
    metaheader = f'{metadata_payload_header}{metadata_payload_version}'
    
    # struct is 
    #  char HEADER[6]
    #  uint32 size
    #  uint8  namelen
    #  char name[metadata_proj_name_maxlen]
    #  uint32 clock_hz
    
    payload = bytes(metaheader, encoding='ascii')
    payload += struct.pack('<IB', bitstreamSize, bsnamelen) + bsnameArray
    payload += struct.pack('<I', autoclock)
    # print(payload)
    hdr = Header(Flags.FamilyIDPresent | Flags.NotMainFlash, flash_address, len(payload), 0, 1, settings.boardFamily)
    return DataBlock(payload, hdr, magic_start1=(settings.magicStart1+metadata_start1_offset),
                        magic_end=settings.magicEnd)


def gen_factory_reset(args):
    
    
    uf2sets = TargetOptions[args.target]
    uf2sets.magicStart1 += factoryreset_start1_offset
    
    uf2 = get_new_uf2(uf2sets)
    payload_bytes =  bytes(f'{factoryreset_payload_header}01', encoding='ascii')
    
    uf2.append_payload(payload_bytes, 
                       start_offset=base_bitstream_storage_address_kb*1024, 
                       block_payload_size=256)
                       
    
    uf2.to_file(args.outfile)
    print(f"\n\nGenerated FACTORY RESET UF2.")
    print(f"It now available at {args.outfile}\n")
                       
    
    

def main():
    
    args = get_args()
    
    if not UF2UtilsPresent:
        print()
        print("ERROR: You need uf2utils to run this program")
        print("Run `pip install uf2utils` or otherwise get it from pypi.org")
        print()
        return 
    
    if args.factoryreset:
        gen_factory_reset(args)
        return
    
    if args.slot < 1 or args.slot > 4:
        print("Select a slot between 1-4")
        sys.exit(-1)
        
    if len(args.name) > metadata_proj_name_maxlen:
        print(f'Name can only be up to {metadata_proj_name_maxlen} characters. Will truncate.')
        
    if args.autoclock:
        if args.autoclock < 10 or args.autoclock > 60e6:
            print("Auto-clocking only supports rates between 10Hz and 60MHz")
            sys.exit(-3)
        
    
    slotidx = args.slot - 1
    payload_bytes = get_payload_contents(args.infile)
    
    # stick it somewhere within its slot...
    
    
    
    uf2sets = TargetOptions[args.target]
    
    uf2 = get_new_uf2(uf2sets)
    
    
    # figure out a start address for the bitstream.
    # we have a little room to play, important thing is to page-align.
    
    # number of pages this infile requires, plus a teeny bit of slack
    pages_required = int(len(payload_bytes)/(4*1024)) + 4
    
    # base page for this slot
    lowest_page_for_slot = base_page + (reserved_pages_for_bitstream_slot * slotidx)
    # actual start page we'll use, randomized, with a teeny bit of slack on the front as well
    start_page = lowest_page_for_slot + random.randint(4, reserved_pages_for_bitstream_slot-pages_required)
    # actual start address, based on page
    start_offset = start_page*page_blocks*1024
    
    # append a data block for meta information
    uf2.append_datablock(get_metadata_block(uf2sets, start_offset, 
                        len(payload_bytes), args.autoclock, 
                        args.infile, args.name))
    uf2.append_payload(payload_bytes, 
                       start_offset=start_offset, 
                       block_payload_size=256)
                       
    if args.appendslot:
        fnameext = os.path.splitext(args.outfile)
        outfilename = f'{fnameext[0]}_{args.slot}{fnameext[1]}'
    else:
        outfilename = args.outfile
    
    uf2.to_file(outfilename)
    print(f"\n\nGenerated UF2 for slot {args.slot}, starting at address {hex(start_offset)} with size {len(payload_bytes)}")
    print(f"It now available at {outfilename}\n")
    

if __name__ == "__main__":
    main()
    
