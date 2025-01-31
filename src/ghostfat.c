/*
 * The MIT License (MIT)
 *
 * Copyright (c) Microsoft Corporation
 * Copyright (c) Ha Thach for Adafruit Industries
 * Copyright (c) Henry Gabryjelski
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "board_includes.h"

#include "compile_date.h"
#include "uf2.h"
#include "board.h"
#include "debug.h"
#include "cdc_interface.h"
#include "bitstream.h"
#include "board_config.h"
#include "board_config_defaults.h"

//--------------------------------------------------------------------+
//
//--------------------------------------------------------------------+

// ota0 partition size
static uint32_t _flash_size;

#define STATIC_ASSERT(_exp) _Static_assert(_exp, "static assert failed")

#define STR0(x) #x
#define STR(x) STR0(x)

#define UF2_ARRAY_SIZE(_arr)    ( sizeof(_arr) / sizeof(_arr[0]) )
#define UF2_DIV_CEIL(_v, _d)    ( ((_v) / (_d)) + ((_v) % (_d) ? 1 : 0) )

typedef struct {
    uint8_t JumpInstruction[3];
    uint8_t OEMInfo[8];
    uint16_t SectorSize;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t FATCopies;
    uint16_t RootDirectoryEntries;
    uint16_t TotalSectors16;
    uint8_t MediaDescriptor;
    uint16_t SectorsPerFAT;
    uint16_t SectorsPerTrack;
    uint16_t Heads;
    uint32_t HiddenSectors;
    uint32_t TotalSectors32;
    uint8_t PhysicalDriveNum;
    uint8_t Reserved;
    uint8_t ExtendedBootSig;
    uint32_t VolumeSerialNumber;
    uint8_t VolumeLabel[11];
    uint8_t FilesystemIdentifier[8];
} __attribute__((packed)) FAT_BootBlock;

typedef struct {
    char name[8];
    char ext[3];
    uint8_t attrs;
    uint8_t reserved;
    uint8_t createTimeFine;
    uint16_t createTime;
    uint16_t createDate;
    uint16_t lastAccessDate;
    uint16_t highStartCluster;
    uint16_t updateTime;
    uint16_t updateDate;
    uint16_t startCluster;
    uint32_t size;
} __attribute__((packed)) DirEntry;
STATIC_ASSERT(sizeof(DirEntry) == 32);

typedef struct FileContent {
  char const name[11];
  void const * content;
  uint32_t size;       // OK to use uint32_T b/c FAT32 limits filesize to (4GiB - 2)

  // computing fields based on index and size
  uint16_t cluster_start;
  uint16_t cluster_end;
} FileContent_t;

//--------------------------------------------------------------------+
//
//--------------------------------------------------------------------+

#define BPB_SECTOR_SIZE           ( 512)
#define BPB_SECTORS_PER_CLUSTER   (CFG_UF2_SECTORS_PER_CLUSTER)
#define BPB_RESERVED_SECTORS      (   1)
#define BPB_NUMBER_OF_FATS        (   2)
#define BPB_ROOT_DIR_ENTRIES      (  64)
#define BPB_TOTAL_SECTORS         CFG_UF2_NUM_BLOCKS
#define BPB_MEDIA_DESCRIPTOR_BYTE (0xF8)
#define FAT_ENTRY_SIZE            (2)
#define FAT_ENTRIES_PER_SECTOR    (BPB_SECTOR_SIZE / FAT_ENTRY_SIZE)
#define FAT_END_OF_CHAIN          (0xFFFF)

// NOTE: MS specification explicitly allows FAT to be larger than necessary
#define TOTAL_CLUSTERS_ROUND_UP   UF2_DIV_CEIL(BPB_TOTAL_SECTORS, BPB_SECTORS_PER_CLUSTER)
#define BPB_SECTORS_PER_FAT       UF2_DIV_CEIL(TOTAL_CLUSTERS_ROUND_UP, FAT_ENTRIES_PER_SECTOR)
#define DIRENTRIES_PER_SECTOR     (BPB_SECTOR_SIZE/sizeof(DirEntry))
#define ROOT_DIR_SECTOR_COUNT     UF2_DIV_CEIL(BPB_ROOT_DIR_ENTRIES, DIRENTRIES_PER_SECTOR)
#define BPB_BYTES_PER_CLUSTER     (BPB_SECTOR_SIZE * BPB_SECTORS_PER_CLUSTER)


STATIC_ASSERT((BPB_SECTORS_PER_CLUSTER & (BPB_SECTORS_PER_CLUSTER-1)) == 0); // sectors per cluster must be power of two
STATIC_ASSERT(BPB_SECTOR_SIZE                              ==       512); // GhostFAT does not support other sector sizes (currently)
STATIC_ASSERT(BPB_NUMBER_OF_FATS                           ==         2); // FAT highest compatibility
STATIC_ASSERT(sizeof(DirEntry)                             ==        32); // FAT requirement
STATIC_ASSERT(BPB_SECTOR_SIZE % sizeof(DirEntry)           ==         0); // FAT requirement
STATIC_ASSERT(BPB_ROOT_DIR_ENTRIES % DIRENTRIES_PER_SECTOR ==         0); // FAT requirement
STATIC_ASSERT(BPB_BYTES_PER_CLUSTER                        <= (32*1024)); // FAT requirement (64k+ has known compatibility problems)
STATIC_ASSERT(FAT_ENTRIES_PER_SECTOR                       ==       256); // FAT requirement


#define UF2_FIRMWARE_BYTES_PER_SECTOR   256
#define UF2_SECTOR_COUNT                (_flash_size / UF2_FIRMWARE_BYTES_PER_SECTOR)
#define UF2_BYTE_COUNT                  (UF2_SECTOR_COUNT * BPB_SECTOR_SIZE) // always a multiple of sector size, per UF2 spec



//define UF2_WRITE_DEBUG_NOT_UF2_DUMP
// define UF2_WRITE_DEBUG_UF2_METAINFO_DUMP

#define GF_DEBUG_ENABLE 0
#if GF_DEBUG_ENABLE
#define GF_DEBUG_ENDLN()	DEBUG_ENDLN()
#define GF_DEBUG_BUF(b, len) DEBUG_BUF(b, len)
#define GF_DEBUG(s) 		DEBUG(s)
#define GF_DEBUG_LN(s)		DEBUG_LN(s)

#define GF_DEBUG_U32(v) 	DEBUG_U32(v)
#define GF_DEBUG_U32_LN(v) 	DEBUG_U32_LN(v)
#define GF_DEBUG_U16(v) 	DEBUG_U16(v)

#define GF_DEBUG_U16_LN(v) 	DEBUG_U16_LN(v)
#define GF_DEBUG_U8(v) 		DEBUG_U8(v)
#define GF_DEBUG_U8_LN(v) 	DEBUG_U8_LN(v)

#if GF_DEBUG_ENABLE > 1
#define GF_DEBUG_VERBOSE(s)			GF_DEBUG(s)
#define GF_DEBUG_VERBOSE_LN(s)		GF_DEBUG_LN(s)
#else
#define GF_DEBUG_VERBOSE(s)
#define GF_DEBUG_VERBOSE_LN(s)
#endif
#else

#define GF_DEBUG_ENDLN()
#define GF_DEBUG_BUF(b, len)
#define GF_DEBUG(s)
#define GF_DEBUG_LN(s)

#define GF_DEBUG_U32(v)
#define GF_DEBUG_U32_LN(v)
#define GF_DEBUG_U16(v)

#define GF_DEBUG_U16_LN(v)
#define GF_DEBUG_U8(v)
#define GF_DEBUG_U8_LN(v)
#define GF_DEBUG_VERBOSE(s)
#define GF_DEBUG_VERBOSE_LN(s)
#endif




char infoUf2File[128*3] =
    "Board: " UF2_BOARD_ID "\r\n"
    "Firmware: " UF2_PRODUCT_NAME "\r\n"
    "Version: "  UF2_VERSION "\r\n"
    "Compiled: " COMPILE_DATE "\r\n"
    "Flash Size: 0x";

TINYUF2_CONST char indexFile[] =
    "<!doctype html>\n"
    "<html>"
    "<body>"
    "<script>\n"
    "location.replace(\"" UF2_INDEX_URL "\");\n"
    "</script>"
    "</body>"
    "</html>\n";

TINYUF2_CONST char howtoFile[] =
	"Writing a UF2 file to this 'drive' will "
	"replace the contents of the bitstream for "
	"the FPGA.\r\n\r\n"
	"The FPGA will reset after writes, and I/O may be "
	"accessed via the headers/PMODs.\r\n\r\n"
	"There is also a serial device present to "
	"allow for configuration and interaction with "
	"the project, to set the clocking, select the slot or, "
	"for advanced users, via the uart bridge or microcotb (https://microcotb.org).\r\n\r\n"
	"Source code for this system is available at https://github.com/psychogenic/riffpga\r\n"
		;

#ifdef TINYUF2_FAVICON_HEADER
#include TINYUF2_FAVICON_HEADER
const char autorunFile[] = "[Autorun]\r\nIcon=FAVICON.ICO\r\n";
#endif

// size of CURRENT.UF2:
static FileContent_t info[] = {
    {.name = "INFO    TXT", .content = infoUf2File , .size = sizeof(infoUf2File) - 1},
    {.name = "README  TXT", .content = howtoFile   , .size = sizeof(howtoFile  ) - 1},
    {.name = "INDEX   HTM", .content = indexFile   , .size = sizeof(indexFile  ) - 1},
#ifdef TINYUF2_FAVICON_HEADER
    {.name = "AUTORUN INF", .content = autorunFile , .size = sizeof(autorunFile) - 1},
    {.name = "FAVICON ICO", .content = favicon_data, .size = favicon_len            },
#endif
    // current.uf2 must be the last element and its content must be NULL
    {.name = "CURRENT UF2", .content = NULL       , .size = 0                      },
};

enum {
  NUM_FILES = sizeof(info) / sizeof(info[0]),
  NUM_DIRENTRIES = NUM_FILES + 1 // including volume label as first root directory entry
};

enum {
  FID_INFO = 0,
  FID_INDEX = 1,
  FID_UF2 = NUM_FILES - 1,
};

STATIC_ASSERT(NUM_DIRENTRIES < BPB_ROOT_DIR_ENTRIES);  // FAT requirement -- Ensures BPB reserves sufficient entries for all files
STATIC_ASSERT(NUM_DIRENTRIES < DIRENTRIES_PER_SECTOR); // GhostFAT bug workaround -- else, code overflows buffer


#define NUM_SECTORS_IN_DATA_REGION (BPB_TOTAL_SECTORS - BPB_RESERVED_SECTORS - (BPB_NUMBER_OF_FATS * BPB_SECTORS_PER_FAT) - ROOT_DIR_SECTOR_COUNT)
#define CLUSTER_COUNT              (NUM_SECTORS_IN_DATA_REGION / BPB_SECTORS_PER_CLUSTER)

// Ensure cluster count results in a valid FAT16 volume!
STATIC_ASSERT( CLUSTER_COUNT >= 0x0FF5 && CLUSTER_COUNT < 0xFFF5 );

// Many existing FAT implementations have small (1-16) off-by-one style errors
// So, avoid being within 32 of those limits for even greater compatibility.

STATIC_ASSERT( CLUSTER_COUNT >= 0x1015 && CLUSTER_COUNT < 0xFFD5 );

#define FS_START_FAT0_SECTOR      BPB_RESERVED_SECTORS
#define FS_START_FAT1_SECTOR      (FS_START_FAT0_SECTOR + BPB_SECTORS_PER_FAT)
#define FS_START_ROOTDIR_SECTOR   (FS_START_FAT1_SECTOR + BPB_SECTORS_PER_FAT)
#define FS_START_CLUSTERS_SECTOR  (FS_START_ROOTDIR_SECTOR + ROOT_DIR_SECTOR_COUNT)

//--------------------------------------------------------------------+
//
//--------------------------------------------------------------------+

static Bitstream_MetaInfo bs_write_metainfo = {0};

static FAT_BootBlock TINYUF2_CONST BootBlock = {
    .JumpInstruction      = {0xeb, 0x3c, 0x90},
    .OEMInfo              = "UF2 UF2 ",
    .SectorSize           = BPB_SECTOR_SIZE,
    .SectorsPerCluster    = BPB_SECTORS_PER_CLUSTER,
    .ReservedSectors      = BPB_RESERVED_SECTORS,
    .FATCopies            = BPB_NUMBER_OF_FATS,
    .RootDirectoryEntries = BPB_ROOT_DIR_ENTRIES,
    .TotalSectors16       = (BPB_TOTAL_SECTORS > 0xFFFF) ? 0 : BPB_TOTAL_SECTORS,
    .MediaDescriptor      = BPB_MEDIA_DESCRIPTOR_BYTE,
    .SectorsPerFAT        = BPB_SECTORS_PER_FAT,
    .SectorsPerTrack      = 1,
    .Heads                = 1,
    .TotalSectors32       = (BPB_TOTAL_SECTORS > 0xFFFF) ? BPB_TOTAL_SECTORS : 0,
    .PhysicalDriveNum     = 0x80, // to match MediaDescriptor of 0xF8
    .ExtendedBootSig      = 0x29,
    .VolumeSerialNumber   = 0x00420042,
    .VolumeLabel          = UF2_VOLUME_LABEL,
    .FilesystemIdentifier = "FAT16   ",
};

//--------------------------------------------------------------------+
//
//--------------------------------------------------------------------+

static inline bool is_uf2_binfactreset_block(UF2_Block const *bl, BoardConfigPtrConst bc) {
	static const char payloadHeader[6] = BIN_UF2_FACTORYRESET_PAYLOADHEADER;
	if  (! ( (bl->magicStart0 == UF2_MAGIC_START0) &&
	         (bl->magicStart1 == bc->bin_download.magic_start + BIN_UF2_FACTORYRESET_START1DELTA) &&
	         (bl->magicEnd == bc->bin_download.magic_end)
		 ) ) {
		return false;
	}
	// UF2 header is good
	// check payload
	for (uint8_t i=0; i<6; i++) {
		if (bl->data[i] != payloadHeader[i]) {
			return false;
		}
	}

	return true;
}

static inline bool is_uf2_binmeta_block(UF2_Block const *bl, BoardConfigPtrConst bc) {
	static const char payloadHeader[6] = BIN_UF2_METABLOCK_PAYLOADHEADER;
	if  (! ( (bl->magicStart0 == UF2_MAGIC_START0) &&
	         (bl->magicStart1 == bc->bin_download.magic_start + BIN_UF2_METABLOCK_START1DELTA) &&
	         (bl->magicEnd == bc->bin_download.magic_end)
		 ) ) {
		return false;
	}
	// UF2 header is good
	// check payload
	for (uint8_t i=0; i<6; i++) {
		if (bl->data[i] != payloadHeader[i]) {
			return false;
		}
	}

	return true;

}


static inline bool is_uf2_bin_block (UF2_Block const *bl, BoardConfigPtrConst bc) {
#ifdef GF_DEBUG_OUTPUT_ENABLED
	if (bl->magicStart0 != UF2_MAGIC_START0) {
		GF_DEBUG("no magic start0");
		GF_DEBUG_U32_LN(bl->magicStart0);
		return 0;
	}


	if (bl->magicStart1 != bc->bin_download.magic_start) {
		GF_DEBUG_LN("bad magic start1");
		return 0;
	}

	if (bl->magicEnd != bc->bin_download.magic_end) {
		GF_DEBUG_LN("bad magic end");
		return 0;
	}


	if (bl->flags & UF2_FLAG_NOFLASH) {
		GF_DEBUG_LN("no flash");
		return 0;
	}

	GF_DEBUG_LN("uf2block!");
	return 1;


#endif
  return (bl->magicStart0 == UF2_MAGIC_START0) &&
         (bl->magicStart1 == bc->bin_download.magic_start) &&
         (bl->magicEnd == bc->bin_download.magic_end) &&
         !(bl->flags & UF2_FLAG_NOFLASH);
}



// cache the cluster start offset for each file
// this allows more flexible algorithms w/o O(n) time
static void init_starting_clusters(void) {
  // +2 because FAT decided first data sector would be in cluster number 2, rather than zero
  uint16_t start_cluster = 2;

  for (uint16_t i = 0; i < NUM_FILES; i++) {
    info[i].cluster_start = start_cluster;
    info[i].cluster_end = start_cluster + (uint16_t)
    		(UF2_DIV_CEIL(info[i].size, BPB_SECTOR_SIZE*BPB_SECTORS_PER_CLUSTER) - 1);

    start_cluster = info[i].cluster_end + 1;
  }
}

// get file index for file that uses the cluster
// if cluster is past last file, returns ( NUM_FILES-1 ).
//
// Caller must still check if a particular *sector*
// contains data from the file's contents, as there
// are often padding sectors, including all the unused
// sectors past the end of the media.
static uint32_t info_index_of(uint32_t cluster) {
  // default results for invalid requests is the index of the last file (CURRENT.UF2)


  if (cluster >= 0xFFF0) {
	  return FID_UF2;
  }

  for (uint32_t i = 0; i < NUM_FILES; i++) {
	// GF_DEBUG_U32_LN(info[i].cluster_start);
    if ( (info[i].cluster_start <= cluster) && (cluster <= info[i].cluster_end) ) {
      return i;
    }
  }

  return FID_UF2;
}


void uf2_init(void) {
  // TODO maybe limit to application size only if possible board_flash_app_size()
  _flash_size = board_flash_size();
  info[FID_UF2].size = 0;
  // update CURRENT.UF2 file size
  if (bs_check_for_marker()) {
	  info[FID_UF2].size = bs_uf2_file_size();
  }


  if (! info[FID_UF2].size ) {
	  bs_clear_size_check_flag();
	  // set to max
	  info[FID_UF2].size = UF2_BYTE_COUNT;
  }

  // update INFO_UF2.TXT with flash size if having enough space (8 bytes)
  size_t txt_len = strlen(infoUf2File);
  size_t const max_len = sizeof(infoUf2File) - 1;
  if ( max_len - txt_len > 8) {
    txt_len += u32_to_hexstr(_flash_size, infoUf2File + txt_len);

    if (max_len - txt_len > 6) {
      strcpy(infoUf2File + txt_len, " bytes\r\n");
      txt_len += 8;
    }
  }
  info[FID_INFO].size = txt_len;

  init_starting_clusters();

}

/*------------------------------------------------------------------*/
/* Read CURRENT.UF2
 *------------------------------------------------------------------*/
void padded_memcpy (char *dst, char const *src, int len) {
  for ( int i = 0; i < len; ++i ) {
    if ( *src ) {
      *dst = *src++;
    } else {
      *dst = ' ';
    }
    dst++;
  }
}


void uf2_read_block (uint32_t block_no, uint8_t *data) {
  memset(data, 0, BPB_SECTOR_SIZE);
  uint32_t sectionRelativeSector = block_no;

  if (! bs_have_checked_for_marker() ) {
	  uint32_t found_size = bs_check_for_marker();
	  GF_DEBUG("FOUND A SIZE MARKER!!!");
	  GF_DEBUG_U32_LN(found_size);
	  if (found_size) {
	  		  info[FID_UF2].size = found_size;
	  }
	  // re-init these
	  init_starting_clusters();
  }



  // GF_DEBUG("uf2_read_block "); GF_DEBUG_U32_LN(block_no);
  if ( block_no == 0 ) {
    // Request was for the Boot block
    memcpy(data, &BootBlock, sizeof(BootBlock));
    data[510] = 0x55;    // Always at offsets 510/511, even when BPB_SECTOR_SIZE is larger
    data[511] = 0xaa;    // Always at offsets 510/511, even when BPB_SECTOR_SIZE is larger
  }
  else if ( block_no < FS_START_ROOTDIR_SECTOR ) {
    // Request was for a FAT table sector

	// GF_DEBUG_LN("Root dir");
    sectionRelativeSector -= FS_START_FAT0_SECTOR;

    // second FAT is same as the first... use sectionRelativeSector to write data
    if ( sectionRelativeSector >= BPB_SECTORS_PER_FAT ) {
      sectionRelativeSector -= BPB_SECTORS_PER_FAT;
    }

    uint16_t* data16 = (uint16_t*) (void*) data;
    uint32_t sectorFirstCluster = sectionRelativeSector * FAT_ENTRIES_PER_SECTOR;
    uint32_t firstUnusedCluster = info[FID_UF2].cluster_end + 1;

    // OPTIMIZATION:
    // Because all files are contiguous, the FAT CHAIN entries
    // are all set to (cluster+1) to point to the next cluster.
    // All clusters past the last used cluster of the last file
    // are set to zero.
    //
    // EXCEPTIONS:
    // 1. Clusters 0 and 1 require special handling
    // 2. Final cluster of each file must be set to END_OF_CHAIN
    //

    // Set default FAT values first.
    for (uint16_t i = 0; i < FAT_ENTRIES_PER_SECTOR; i++) {
      uint32_t cluster = i + sectorFirstCluster;
      if (cluster >= firstUnusedCluster) {
        data16[i] = 0;
      }
      else {
        data16[i] = (uint16_t) (cluster + 1);
      }
    }

    // Exception #1: clusters 0 and 1 need special handling
    if (sectionRelativeSector == 0) {
      data[0] = BPB_MEDIA_DESCRIPTOR_BYTE;
      data[1] = 0xff;
      data16[1] = FAT_END_OF_CHAIN; // cluster 1 is reserved
    }

    // Exception #2: the final cluster of each file must be set to END_OF_CHAIN
    for (uint32_t i = 0; i < NUM_FILES; i++) {
      uint32_t lastClusterOfFile = info[i].cluster_end;
      if (lastClusterOfFile >= sectorFirstCluster) {
        uint32_t idx = lastClusterOfFile - sectorFirstCluster;
        if (idx < FAT_ENTRIES_PER_SECTOR) {
          // that last cluster of the file is in this sector
          data16[idx] = FAT_END_OF_CHAIN;
        }
      }
    }

  }
  else if ( block_no < FS_START_CLUSTERS_SECTOR ) {
    // Request was for a (root) directory sector .. root because not supporting subdirectories (yet)
    sectionRelativeSector -= FS_START_ROOTDIR_SECTOR;

    DirEntry *d = (void*) data;                   // pointer to next free DirEntry this sector
    int remainingEntries = DIRENTRIES_PER_SECTOR; // remaining count of DirEntries this sector

    uint32_t startingFileIndex;

    if ( sectionRelativeSector == 0 ) {
      // volume label is first directory entry
      padded_memcpy(d->name, (char const*) BootBlock.VolumeLabel, 11);
      d->attrs = 0x28;
      d++;
      remainingEntries--;

      startingFileIndex = 0;
    }else {
      // -1 to account for volume label in first sector
      startingFileIndex = DIRENTRIES_PER_SECTOR * sectionRelativeSector - 1;
    }

    for ( uint32_t fileIndex = startingFileIndex;
          remainingEntries > 0 && fileIndex < NUM_FILES; // while space remains in buffer and more files to add...
          fileIndex++, d++ ) {
      // WARNING -- code presumes all files take exactly one directory entry (no long file names!)
      uint32_t const startCluster = info[fileIndex].cluster_start;

      FileContent_t const *inf = &info[fileIndex];
      padded_memcpy(d->name, inf->name, 11);
      d->createTimeFine   = COMPILE_SECONDS_INT % 2 * 100;
      d->createTime       = (uint16_t)COMPILE_DOS_TIME;
      d->createDate       = (uint16_t)COMPILE_DOS_DATE;
      d->lastAccessDate   = (uint16_t)COMPILE_DOS_DATE;
      d->highStartCluster = (uint16_t)(startCluster >> 16);
      d->updateTime       = (uint16_t)COMPILE_DOS_TIME;
      d->updateDate       = (uint16_t)COMPILE_DOS_DATE;
      d->startCluster     = (uint16_t)(startCluster & 0xFFFF);
      d->size             = (inf->size ? inf->size : UF2_BYTE_COUNT);
    }
  }
  else if ( block_no < BPB_TOTAL_SECTORS ) {
    // Request was to read from the data area (files, unused space, ...)
    sectionRelativeSector -= FS_START_CLUSTERS_SECTOR;

    // plus 2 for first data cluster offset
    uint32_t fid = info_index_of(2 + sectionRelativeSector / BPB_SECTORS_PER_CLUSTER);
    FileContent_t const * inf = &info[fid];

    uint32_t fileRelativeSector = sectionRelativeSector - (uint32_t)(info[fid].cluster_start-2) * BPB_SECTORS_PER_CLUSTER;

    if ( fid != FID_UF2 ) {
      // Handle all files other than CURRENT.UF2
      size_t fileContentStartOffset = fileRelativeSector * BPB_SECTOR_SIZE;
      size_t fileContentLength = inf->size;
      // nothing to copy if already past the end of the file (only when >1 sector per cluster)
      if (fileContentLength > fileContentStartOffset) {
        // obviously, 2nd and later sectors should not copy data from the start
        const void * dataStart = (inf->content) + fileContentStartOffset;
        // limit number of bytes of data to be copied to remaining valid bytes
        size_t bytesToCopy = fileContentLength - fileContentStartOffset;
        // and further limit that to a single sector at a time
        if (bytesToCopy > BPB_SECTOR_SIZE) {
          bytesToCopy = BPB_SECTOR_SIZE;
        }
        memcpy(data, dataStart, bytesToCopy);
      }
    }
    else {
      // CURRENT.UF2: generate data on-the-fly

      BoardConfigPtrConst bc = boardconfig_get();
      uint32_t start_offset = boardconfig_bin_startoffset();
      uint32_t addr = start_offset + (fileRelativeSector * UF2_FIRMWARE_BYTES_PER_SECTOR);

      GF_DEBUG_VERBOSE("read UF2 @");
#if GF_DEBUG_ENABLE > 1
      GF_DEBUG_U32_LN(addr);
#endif
      if ( (addr - start_offset) < (BOARD_FLASH_ADDR_ZERO + _flash_size) ) {
        UF2_Block *bl = (void*) data;
        bl->magicStart0 = UF2_MAGIC_START0;
        bl->magicStart1 = bc->bin_download.magic_start;
        bl->magicEnd = bc->bin_download.magic_end;
        bl->blockNo = fileRelativeSector;
        bl->numBlocks = UF2_SECTOR_COUNT;
        bl->targetAddr = addr;
        bl->payloadSize = UF2_FIRMWARE_BYTES_PER_SECTOR;
        bl->flags = UF2_FLAG_FAMILYID;
        bl->familyID = bc->bin_download.family_id;

        board_flash_read(addr, bl->data, bl->payloadSize);
      }

    }

  }
}

static void uf2_write_complete(void) {
	GF_DEBUG_LN("UF2 write complete!");
	sleep_ms(20);
	CDCWRITEFLUSH();

	board_flash_flush();
	board_reboot();
}



/*------------------------------------------------------------------*/
/* Write UF2
 *------------------------------------------------------------------*/
#ifdef GF_DEBUG_OUTPUT_ENABLED
static void dump_uf2_block(UF2_Block * bl) {
	GF_DEBUG("uf2block: ");
	GF_DEBUG_U32(bl->magicStart0);
	GF_DEBUG(" srt1: ");
	GF_DEBUG_U32_LN(bl->magicStart1);

}

#define DUMP_UF2BLOCK(b) dump_uf2_block(b)

#else
#define DUMP_UF2BLOCK(b)
#endif

static void debug_dump_datablock(uint8_t *data, uint16_t len) {

	  char ascii_contents[20] = {0};
	  ascii_contents[16] = '\r';
	  ascii_contents[17] = '\n';
	  bool flushed = true;
	  for (uint16_t i=0; i<len; i++) {

		  if (i % 16 == 0) {

			  CDCWRITESTRING("  ");
			  if (i > 0) {
				  CDCWRITESTRING(ascii_contents);
			  } else {
				  CDCWRITESTRING("\r\n");
			  }
			  CDCWRITEFLUSH();
			  // reset
			  for (uint8_t j=0; j<16; j++) {
				  ascii_contents[j] = '.';
			  }
			  flushed = true;
		  } else {
			  flushed = false;
		  }
		  cdc_write_u8_leadingzeros(data[i]);
		  cdc_write_char(' ');
		  if (data[i]>=0x20 && data[i] <=0x7e) {
			  ascii_contents[i%16] = data[i];
		  } else {
			  ascii_contents[i%16] = '.';
		  }
	  }

	  if (flushed == false) {
		  CDCWRITESTRING(ascii_contents);
	  }
}





/**
 * Write an uf2 block wrapped by 512 sector.
 * @return number of bytes processed, only 3 following values
 *  -1 : if not an uf2 block
 * 512 : write is successful (BPB_SECTOR_SIZE == 512)
 *   0 : is busy with flashing, tinyusb stack will call write_block again with the same parameters later on
 */


int uf2_write_block (uint32_t block_no, uint8_t *data, WriteState *state) {
  (void) block_no;
  static bool write_is_complete = false;
  BoardConfigPtrConst bc = boardconfig_get();
  UF2_Block *bl = (void*) data;
  DUMP_UF2BLOCK(bl);




  if ( !is_uf2_bin_block(bl, bc) ) {

	  if (is_uf2_binmeta_block(bl, bc)) {
#ifdef UF2_WRITE_DEBUG_UF2_METAINFO_DUMP
		  /*
		  DEBUG_LN("Got our meta-data");
		  debug_dump_datablock(data, BPB_SECTOR_SIZE);
		  CDCWRITEFLUSH();
		  */
#endif
		  Bitstream_MetaInfo_Payload payloadmeta;
		  // doing a two-step, just for clarity and flexibility
		  // get the payload
		  memcpy(&payloadmeta, bl->data, sizeof(payloadmeta));
		  // save the meta info within
		  memcpy(&bs_write_metainfo, &payloadmeta.info, sizeof(bs_write_metainfo));


		#ifdef UF2_WRITE_DEBUG_UF2_METAINFO_DUMP
		  	  	  DEBUG_U32_LN(bs_write_metainfo.clock_hz);
				  DEBUG_LN("Copied payload is");
				  debug_dump_datablock(&bs_write_metainfo, sizeof(bs_write_metainfo));
		#endif

		  state->numWritten++;
	  } else if (is_uf2_binfactreset_block(bl, bc)) {
		  CDCWRITESTRING("This is a FACTORY RESET request!!!");
		  boardconfig_factoryreset(true);
		  CDCWRITEFLUSH();

    	  sleep_ms(150);
    	  uf2_write_complete();
	  }





#ifdef UF2_WRITE_DEBUG_NOT_UF2_DUMP
	  DEBUG_LN("\r\nNot a UF2:\r\n");
	  if ( block_no == 0 ) {
	  	  DEBUG_LN("Write to BLOCK 0\r\n");
	     } else if ( block_no < FS_START_ROOTDIR_SECTOR ) {
	       // Request was for a FAT table sector
	  		  DEBUG("Write to FAT table: ");
	  		  DEBUG_U32_LN(block_no);
	     } else if (block_no < FS_START_CLUSTERS_SECTOR) {
	  	   DEBUG("Write to root dir sector: ");
	  		  DEBUG_U32_LN(block_no);
	     } else {
	    	 DEBUG("block no: ");
	  		  DEBUG_U32_LN(block_no);
	     }
	  debug_dump_datablock(data, BPB_SECTOR_SIZE);
#endif


	  return -1;
  }

  // GF_DEBUG("NumBlock in U: ");
  // GF_DEBUG_U32_LN(bl->numBlocks);

  if (bl->familyID == bc->bin_download.family_id) {
    // generic family ID

    board_flash_write(bl->targetAddr, bl->data, bl->payloadSize);
  }else {
    // TODO family matches VID/PID
	GF_DEBUG_LN("Invalid fam");
    return -1;
  }

  //------------- Update written blocks -------------//
  if ( bl->numBlocks ) {
    // Update state num blocks if needed
    if ( state->numBlocks != bl->numBlocks ) {
      if ( bl->numBlocks >= MAX_BLOCKS || state->numBlocks ) {
        state->numBlocks = 0xffffffff;
      }
      else {
        state->numBlocks = bl->numBlocks;
        GF_DEBUG("NumBlock: ");
        GF_DEBUG_U32_LN(state->numBlocks);
      }
    }

    if ( bl->blockNo < MAX_BLOCKS ) {
      uint8_t const mask = 1 << (bl->blockNo % 8);
      uint32_t const pos = bl->blockNo / 8;

      // only increase written number with new write (possibly prevent overwriting from OS)
      if ( !(state->writtenMask[pos] & mask) ) {
        state->writtenMask[pos] |= mask;
        state->numWritten++;
        GF_DEBUG("Wrote ");
        GF_DEBUG_U32(state->numWritten);
        GF_DEBUG("/");
        GF_DEBUG_U32_LN(state->numBlocks);
        CDCWRITEFLUSH();
      } else {
    	  GF_DEBUG("Dupe @ ");
          GF_DEBUG_U32_LN(pos);
      }

      // flush last blocks
      // TODO numWritten can be smaller than numBlocks if return early
      if ( state->numWritten >= state->numBlocks && write_is_complete == false) {

		  GF_DEBUG("UF2 W done ");
		  if (bs_write_metainfo.bssize) {
			  GF_DEBUG_LN(bs_write_metainfo.name);
		  } else {
			  GF_DEBUG_LN(" no name.");
		  }
    	  write_is_complete = true;
    	  // grab this before playing with flash any further
    	  uint32_t bs_start_addy = board_first_written_address();
    	  uint32_t bs_size_written = board_size_written();



    	  uint32_t boundaries[POSITION_SLOTS_NUM] = {0};

    	  uint8_t slotidx = 0;
    	  bool foundslot = false;
    	  for (uint8_t i=0; i<POSITION_SLOTS_NUM; i++) {
    		  GF_DEBUG("CHKSLT ");
    		  GF_DEBUG_U32(FLASH_STORAGE_STARTADDRESS(i));
    		  GF_DEBUG(" - ");
    		  GF_DEBUG_U32_LN(FLASH_STORAGE_STARTADDRESS(i+1));
	    	  CDCWRITEFLUSH();
    		  if (
    				  (bl->targetAddr >= FLASH_STORAGE_STARTADDRESS(i))
					  &&
					  (bl->targetAddr < FLASH_STORAGE_STARTADDRESS(i+1))) {
    			  GF_DEBUG("Targ in slot ");
    			  GF_DEBUG_U8_LN(slotidx + 1);
    			  // CDCWRITESTRING("Target in slot ");
        		  // cdc_write_dec_u8_ln(slotidx + 1);
        		  slotidx = i;
        		  foundslot = true;
    		  }
    	  };

    	  if (foundslot == true) {
    		  if (slotidx != boardconfig_selected_bitstream_slot()) {
				  CDCWRITESTRING("Wrote UF2 to a new slot: ");
				  cdc_write_dec_u8_ln(slotidx + 1);
				  boardconfig_set_bitstream_slot(slotidx);
				  boardconfig_write();
    		  }
        	  bs_write_marker_to_slot(slotidx, state->numBlocks, bs_size_written, bs_start_addy,
        			  &bs_write_metainfo);
    	  } else {
    		  bs_write_marker(state->numBlocks, bs_size_written, bs_start_addy,&bs_write_metainfo);
    	  }

    	  CDCWRITEFLUSH();

    	  sleep_ms(150);
    	  uf2_write_complete();
      }
    }
  }

  return BPB_SECTOR_SIZE;
}
