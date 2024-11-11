/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
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
 *
 */

#include "msc_disk.hpp"
#include "bsp/board.h"
#include "tusb.h"

#include "utils.h"

#include <string.h>

#include "hal/Usb/usb_interface.hpp"
#include "hal/Usb/UsbFileSystem.hpp"
#include "hal/Usb/UsbFile.hpp"
#include "hal/System/MutexInterface.hpp"
#include "hal/System/LockGuard.hpp"

#include <mutex>

#define MAX_FILE_SIZE_BYTES (128 * 1024)
#define BLOCKS_PER_FILE 0x100
#define START_EXTERNAL_FILE_BLOCK 0x100

static MutexInterface* fileMutex = nullptr;

struct FileEntry
{
  uint16_t startBlock;
  uint16_t numBlocks;
  bool isReadOnly;
  uint32_t size;
  const char* filename;
  UsbFile* handle;
};

static FileEntry fileEntries[8] = {};
static uint32_t numFileEntries = 0;

// whether host does safe-eject
static bool ejected = true;
static bool new_data = false;
static uint8_t errorCount = 0;

// Once this threshold is reached, drive will be forcibly ejected
#define MAX_ERROR_COUNT 50

// 1 README included in root directory
#define NUM_INTERNAL_FILES 1

// README contents stored on ramdisk - must not be greater than 512 bytes
#define README_CONTENTS "\
This is where Dreamcast memory unit data may be viewed when one or more are\n\
inserted into any controller. To write, copy file with the same name as the\n\
target memory unit. Attempting to write more than 128 kb or to read from/write\n\
to a VMU not attached will cause a drive error.\n\
\n\
Reading an entire VMU takes about 3 seconds and write takes 15 seconds. It is\n\
important to note that any operation done on the mass storage device will delay\n\
other controller operations.\
"

// Size of string minus null terminator byte
#define README_SIZE (sizeof(README_CONTENTS) - 1)

// Computes the seconds increment for a given seconds value (seconds may be floating point)
#define DIRECTORY_ENTRY_TIME_SEC_INC(seconds) (int)(((seconds - ((int)seconds - ((int)seconds % 2))) * 100) + 0.5)
#define DIRECTORY_ENTRY_TIME(hours, minutes, seconds) ((((hours - 1) & 0x1F) << 11) | ((minutes & 0x3F) << 5) | (((int)seconds / 2) & 0x1F))
#define DIRECTORY_ENTRY_DATE(year, month, day) ((((year - 1980) & 0x7F) << 9) | ((month & 0x0F) << 5) | (day & 0x1F))

// Different mask values for attr1
#define ATTR1_READ_ONLY 0x01
#define ATTR1_HIDDEN 0x02
#define ATTR1_SYSTEM 0x04
#define ATTR1_VOLUME_LABEL 0x08
#define ATTR1_SUBDIR 0x10
#define ATTR1_ARCHIVE 0x20
#define ATTR1_DEVICE 0x40
#define ATTR1_RESERVED 0x80

#define ATTR2_LOWERCASE_BASENAME 0x08
#define ATTR2_LOWERCASE_EXT 0x10

#define CHAR_TO_UPPER(c) (uint8_t)((c >= 'a' && c <= 'z') ? (c - ('a' - 'A')) : c)

// Converts a set length string to a list of characters
#define CHARACTERIFY1(str) str[0]
#define CHARACTERIFY2(str) CHARACTERIFY1(str), str[1]
#define CHARACTERIFY3(str) CHARACTERIFY2(str), str[2]
#define CHARACTERIFY4(str) CHARACTERIFY3(str), str[3]
#define CHARACTERIFY5(str) CHARACTERIFY4(str), str[4]
#define CHARACTERIFY6(str) CHARACTERIFY5(str), str[5]
#define CHARACTERIFY7(str) CHARACTERIFY6(str), str[6]
#define CHARACTERIFY8(str) CHARACTERIFY7(str), str[7]
#define CHARACTERIFY9(str) CHARACTERIFY8(str), str[8]
#define CHARACTERIFY10(str) CHARACTERIFY9(str), str[9]
#define CHARACTERIFY11(str) CHARACTERIFY10(str), str[10]

// Same as above but makes upper case
#define UCHARACTERIFY1(str) CHAR_TO_UPPER(str[0])
#define UCHARACTERIFY2(str) UCHARACTERIFY1(str), CHAR_TO_UPPER(str[1])
#define UCHARACTERIFY3(str) UCHARACTERIFY2(str), CHAR_TO_UPPER(str[2])
#define UCHARACTERIFY4(str) UCHARACTERIFY3(str), CHAR_TO_UPPER(str[3])
#define UCHARACTERIFY5(str) UCHARACTERIFY4(str), CHAR_TO_UPPER(str[4])
#define UCHARACTERIFY6(str) UCHARACTERIFY5(str), CHAR_TO_UPPER(str[5])
#define UCHARACTERIFY7(str) UCHARACTERIFY6(str), CHAR_TO_UPPER(str[6])
#define UCHARACTERIFY8(str) UCHARACTERIFY7(str), CHAR_TO_UPPER(str[7])

// Case for volume name seems to be handled properly, at least in Windows
#define VOLUME_LABEL_ENTRY(name11, mod_time, mod_date) \
      CHARACTERIFY11(name11),   \
      ATTR1_VOLUME_LABEL, 0x00, \
      0x00,                     \
      0x00, 0x00,               \
      0x00, 0x00,               \
      0x00, 0x00,               \
      0x00, 0x00,               \
      U16_TO_U8S_LE(mod_time),  \
      U16_TO_U8S_LE(mod_date),  \
      0x00, 0x00,               \
      0x00, 0x00, 0x00, 0x00

// Directory entries must be in upper case or Windows will get confused when it tries to open the file
#define DIRECTORY_ENTRY(name8, ext3, attr1, attr2, creation_time_sec_inc, creation_time, creation_date, access_date, file_attr, mod_time, mod_date, starting_page, file_size) \
      UCHARACTERIFY8(name8),   \
      UCHARACTERIFY3(ext3),    \
      attr1, attr2,                 \
      creation_time_sec_inc,        \
      U16_TO_U8S_LE(creation_time), \
      U16_TO_U8S_LE(creation_date), \
      U16_TO_U8S_LE(access_date),   \
      U16_TO_U8S_LE(file_attr),     \
      U16_TO_U8S_LE(mod_time),      \
      U16_TO_U8S_LE(mod_date),      \
      U16_TO_U8S_LE(starting_page), \
      U32_TO_U8S_LE(file_size)

// Default date/time is September 9, 1999 at 00:00:00 (Dreamcast release date in US)
#define DEFAULT_FILE_TIME_SECONDS 0
#define DEFAULT_FILE_TIME_SEC_INC DIRECTORY_ENTRY_TIME_SEC_INC(DEFAULT_FILE_TIME_SECONDS)
#define DEFAULT_FILE_TIME DIRECTORY_ENTRY_TIME(24, 0, DEFAULT_FILE_TIME_SECONDS)
#define DEFAULT_FILE_DATE DIRECTORY_ENTRY_DATE(1999, 9, 8)
#define DEFAULT_ACCESS_DATE DIRECTORY_ENTRY_DATE(1999, 9, 9)

#define SIMPLE_VOL_ENTRY(name11) VOLUME_LABEL_ENTRY(name11, DEFAULT_FILE_TIME, DEFAULT_FILE_DATE)

#define SIMPLE_DIR_ENTRY(name8, ext3, attr1, attr2, starting_page, file_size) \
      DIRECTORY_ENTRY(name8,                      \
                      ext3,                       \
                      attr1,                      \
                      attr2,                      \
                      DEFAULT_FILE_TIME_SEC_INC,  \
                      DEFAULT_FILE_TIME,          \
                      DEFAULT_FILE_DATE,          \
                      DEFAULT_ACCESS_DATE,        \
                      0,                          \
                      DEFAULT_FILE_TIME,          \
                      DEFAULT_FILE_DATE,          \
                      starting_page,              \
                      file_size)

#define VOLUME_LABEL11_STR "DC-Memory  "
#define VOLUME_ENTRY() SIMPLE_VOL_ENTRY(VOLUME_LABEL11_STR)

enum
{
  ALLOCATED_DISK_BLOCK_NUM  = 13, // Number of actual blocks reserved in RAM disk
  BAD_SECTOR_DISK_BLOCK_NUM = 252, // Used to line up memory files starting at address 0x0100
  EXTERNAL_DISK_BLOCK_NUM = 2048, // Number of blocks that exist outside of RAM
  FAKE_DISK_BLOCK_NUM = 32768,    // Report extra space in order to force FAT16 formatting
  DISK_BLOCK_SIZE = 512           // Size in bytes of each block
};
#define REPORTED_BLOCK_NUM (ALLOCATED_DISK_BLOCK_NUM + BAD_SECTOR_DISK_BLOCK_NUM + EXTERNAL_DISK_BLOCK_NUM + FAKE_DISK_BLOCK_NUM)

#define BYTES_PER_ROOT_ENTRY 32

#define NUM_RESERVED_SECTORS 1
#define NUM_FAT_COPIES 1
#define SECTORS_PER_FAT 10
#define NUM_ROOT_ENTRIES 16

#define NUM_FAT_SECTORS (NUM_FAT_COPIES * SECTORS_PER_FAT)
#define NUM_ROOT_SECTORS ((NUM_ROOT_ENTRIES * BYTES_PER_ROOT_ENTRY) / DISK_BLOCK_SIZE)
#define NUM_HEADER_SECTORS (NUM_RESERVED_SECTORS + NUM_FAT_SECTORS + NUM_ROOT_SECTORS)

#define FIRST_VALID_FAT_ADDRESS 2

#define VMUS_PER_PLAYER 2

const uint8_t defaultRootEntry[BYTES_PER_ROOT_ENTRY] =
  {SIMPLE_DIR_ENTRY("        ", "   ", ATTR1_ARCHIVE, ATTR2_LOWERCASE_BASENAME | ATTR2_LOWERCASE_EXT, 0, 0)};

#define FULL_PAGE_FAT_ENTRY(startAddr) \
      U16_TO_U8S_LE(startAddr + 0x01), U16_TO_U8S_LE(startAddr + 0x02), U16_TO_U8S_LE(startAddr + 0x03), U16_TO_U8S_LE(startAddr + 0x04), \
      U16_TO_U8S_LE(startAddr + 0x05), U16_TO_U8S_LE(startAddr + 0x06), U16_TO_U8S_LE(startAddr + 0x07), U16_TO_U8S_LE(startAddr + 0x08), \
      U16_TO_U8S_LE(startAddr + 0x09), U16_TO_U8S_LE(startAddr + 0x0A), U16_TO_U8S_LE(startAddr + 0x0B), U16_TO_U8S_LE(startAddr + 0x0C), \
      U16_TO_U8S_LE(startAddr + 0x0D), U16_TO_U8S_LE(startAddr + 0x0E), U16_TO_U8S_LE(startAddr + 0x0F), U16_TO_U8S_LE(startAddr + 0x10), \
      U16_TO_U8S_LE(startAddr + 0x11), U16_TO_U8S_LE(startAddr + 0x12), U16_TO_U8S_LE(startAddr + 0x13), U16_TO_U8S_LE(startAddr + 0x14), \
      U16_TO_U8S_LE(startAddr + 0x15), U16_TO_U8S_LE(startAddr + 0x16), U16_TO_U8S_LE(startAddr + 0x17), U16_TO_U8S_LE(startAddr + 0x18), \
      U16_TO_U8S_LE(startAddr + 0x19), U16_TO_U8S_LE(startAddr + 0x1A), U16_TO_U8S_LE(startAddr + 0x1B), U16_TO_U8S_LE(startAddr + 0x1C), \
      U16_TO_U8S_LE(startAddr + 0x1D), U16_TO_U8S_LE(startAddr + 0x1E), U16_TO_U8S_LE(startAddr + 0x1F), U16_TO_U8S_LE(startAddr + 0x20), \
      U16_TO_U8S_LE(startAddr + 0x21), U16_TO_U8S_LE(startAddr + 0x22), U16_TO_U8S_LE(startAddr + 0x23), U16_TO_U8S_LE(startAddr + 0x24), \
      U16_TO_U8S_LE(startAddr + 0x25), U16_TO_U8S_LE(startAddr + 0x26), U16_TO_U8S_LE(startAddr + 0x27), U16_TO_U8S_LE(startAddr + 0x28), \
      U16_TO_U8S_LE(startAddr + 0x29), U16_TO_U8S_LE(startAddr + 0x2A), U16_TO_U8S_LE(startAddr + 0x2B), U16_TO_U8S_LE(startAddr + 0x2C), \
      U16_TO_U8S_LE(startAddr + 0x2D), U16_TO_U8S_LE(startAddr + 0x2E), U16_TO_U8S_LE(startAddr + 0x2F), U16_TO_U8S_LE(startAddr + 0x30), \
      U16_TO_U8S_LE(startAddr + 0x31), U16_TO_U8S_LE(startAddr + 0x32), U16_TO_U8S_LE(startAddr + 0x33), U16_TO_U8S_LE(startAddr + 0x34), \
      U16_TO_U8S_LE(startAddr + 0x35), U16_TO_U8S_LE(startAddr + 0x36), U16_TO_U8S_LE(startAddr + 0x37), U16_TO_U8S_LE(startAddr + 0x38), \
      U16_TO_U8S_LE(startAddr + 0x39), U16_TO_U8S_LE(startAddr + 0x3A), U16_TO_U8S_LE(startAddr + 0x3B), U16_TO_U8S_LE(startAddr + 0x3C), \
      U16_TO_U8S_LE(startAddr + 0x3D), U16_TO_U8S_LE(startAddr + 0x3E), U16_TO_U8S_LE(startAddr + 0x3F), U16_TO_U8S_LE(startAddr + 0x40), \
      U16_TO_U8S_LE(startAddr + 0x41), U16_TO_U8S_LE(startAddr + 0x42), U16_TO_U8S_LE(startAddr + 0x43), U16_TO_U8S_LE(startAddr + 0x44), \
      U16_TO_U8S_LE(startAddr + 0x45), U16_TO_U8S_LE(startAddr + 0x46), U16_TO_U8S_LE(startAddr + 0x47), U16_TO_U8S_LE(startAddr + 0x48), \
      U16_TO_U8S_LE(startAddr + 0x49), U16_TO_U8S_LE(startAddr + 0x4A), U16_TO_U8S_LE(startAddr + 0x4B), U16_TO_U8S_LE(startAddr + 0x4C), \
      U16_TO_U8S_LE(startAddr + 0x4D), U16_TO_U8S_LE(startAddr + 0x4E), U16_TO_U8S_LE(startAddr + 0x4F), U16_TO_U8S_LE(startAddr + 0x50), \
      U16_TO_U8S_LE(startAddr + 0x51), U16_TO_U8S_LE(startAddr + 0x52), U16_TO_U8S_LE(startAddr + 0x53), U16_TO_U8S_LE(startAddr + 0x54), \
      U16_TO_U8S_LE(startAddr + 0x55), U16_TO_U8S_LE(startAddr + 0x56), U16_TO_U8S_LE(startAddr + 0x57), U16_TO_U8S_LE(startAddr + 0x58), \
      U16_TO_U8S_LE(startAddr + 0x59), U16_TO_U8S_LE(startAddr + 0x5A), U16_TO_U8S_LE(startAddr + 0x5B), U16_TO_U8S_LE(startAddr + 0x5C), \
      U16_TO_U8S_LE(startAddr + 0x5D), U16_TO_U8S_LE(startAddr + 0x5E), U16_TO_U8S_LE(startAddr + 0x5F), U16_TO_U8S_LE(startAddr + 0x60), \
      U16_TO_U8S_LE(startAddr + 0x61), U16_TO_U8S_LE(startAddr + 0x62), U16_TO_U8S_LE(startAddr + 0x63), U16_TO_U8S_LE(startAddr + 0x64), \
      U16_TO_U8S_LE(startAddr + 0x65), U16_TO_U8S_LE(startAddr + 0x66), U16_TO_U8S_LE(startAddr + 0x67), U16_TO_U8S_LE(startAddr + 0x68), \
      U16_TO_U8S_LE(startAddr + 0x69), U16_TO_U8S_LE(startAddr + 0x6A), U16_TO_U8S_LE(startAddr + 0x6B), U16_TO_U8S_LE(startAddr + 0x6C), \
      U16_TO_U8S_LE(startAddr + 0x6D), U16_TO_U8S_LE(startAddr + 0x6E), U16_TO_U8S_LE(startAddr + 0x6F), U16_TO_U8S_LE(startAddr + 0x70), \
      U16_TO_U8S_LE(startAddr + 0x71), U16_TO_U8S_LE(startAddr + 0x72), U16_TO_U8S_LE(startAddr + 0x73), U16_TO_U8S_LE(startAddr + 0x74), \
      U16_TO_U8S_LE(startAddr + 0x75), U16_TO_U8S_LE(startAddr + 0x76), U16_TO_U8S_LE(startAddr + 0x77), U16_TO_U8S_LE(startAddr + 0x78), \
      U16_TO_U8S_LE(startAddr + 0x79), U16_TO_U8S_LE(startAddr + 0x7A), U16_TO_U8S_LE(startAddr + 0x7B), U16_TO_U8S_LE(startAddr + 0x7C), \
      U16_TO_U8S_LE(startAddr + 0x7D), U16_TO_U8S_LE(startAddr + 0x7E), U16_TO_U8S_LE(startAddr + 0x7F), U16_TO_U8S_LE(startAddr + 0x80), \
      U16_TO_U8S_LE(startAddr + 0x81), U16_TO_U8S_LE(startAddr + 0x82), U16_TO_U8S_LE(startAddr + 0x83), U16_TO_U8S_LE(startAddr + 0x84), \
      U16_TO_U8S_LE(startAddr + 0x85), U16_TO_U8S_LE(startAddr + 0x86), U16_TO_U8S_LE(startAddr + 0x87), U16_TO_U8S_LE(startAddr + 0x88), \
      U16_TO_U8S_LE(startAddr + 0x89), U16_TO_U8S_LE(startAddr + 0x8A), U16_TO_U8S_LE(startAddr + 0x8B), U16_TO_U8S_LE(startAddr + 0x8C), \
      U16_TO_U8S_LE(startAddr + 0x8D), U16_TO_U8S_LE(startAddr + 0x8E), U16_TO_U8S_LE(startAddr + 0x8F), U16_TO_U8S_LE(startAddr + 0x90), \
      U16_TO_U8S_LE(startAddr + 0x91), U16_TO_U8S_LE(startAddr + 0x92), U16_TO_U8S_LE(startAddr + 0x93), U16_TO_U8S_LE(startAddr + 0x94), \
      U16_TO_U8S_LE(startAddr + 0x95), U16_TO_U8S_LE(startAddr + 0x96), U16_TO_U8S_LE(startAddr + 0x97), U16_TO_U8S_LE(startAddr + 0x98), \
      U16_TO_U8S_LE(startAddr + 0x99), U16_TO_U8S_LE(startAddr + 0x9A), U16_TO_U8S_LE(startAddr + 0x9B), U16_TO_U8S_LE(startAddr + 0x9C), \
      U16_TO_U8S_LE(startAddr + 0x9D), U16_TO_U8S_LE(startAddr + 0x9E), U16_TO_U8S_LE(startAddr + 0x9F), U16_TO_U8S_LE(startAddr + 0xA0), \
      U16_TO_U8S_LE(startAddr + 0xA1), U16_TO_U8S_LE(startAddr + 0xA2), U16_TO_U8S_LE(startAddr + 0xA3), U16_TO_U8S_LE(startAddr + 0xA4), \
      U16_TO_U8S_LE(startAddr + 0xA5), U16_TO_U8S_LE(startAddr + 0xA6), U16_TO_U8S_LE(startAddr + 0xA7), U16_TO_U8S_LE(startAddr + 0xA8), \
      U16_TO_U8S_LE(startAddr + 0xA9), U16_TO_U8S_LE(startAddr + 0xAA), U16_TO_U8S_LE(startAddr + 0xAB), U16_TO_U8S_LE(startAddr + 0xAC), \
      U16_TO_U8S_LE(startAddr + 0xAD), U16_TO_U8S_LE(startAddr + 0xAE), U16_TO_U8S_LE(startAddr + 0xAF), U16_TO_U8S_LE(startAddr + 0xB0), \
      U16_TO_U8S_LE(startAddr + 0xB1), U16_TO_U8S_LE(startAddr + 0xB2), U16_TO_U8S_LE(startAddr + 0xB3), U16_TO_U8S_LE(startAddr + 0xB4), \
      U16_TO_U8S_LE(startAddr + 0xB5), U16_TO_U8S_LE(startAddr + 0xB6), U16_TO_U8S_LE(startAddr + 0xB7), U16_TO_U8S_LE(startAddr + 0xB8), \
      U16_TO_U8S_LE(startAddr + 0xB9), U16_TO_U8S_LE(startAddr + 0xBA), U16_TO_U8S_LE(startAddr + 0xBB), U16_TO_U8S_LE(startAddr + 0xBC), \
      U16_TO_U8S_LE(startAddr + 0xBD), U16_TO_U8S_LE(startAddr + 0xBE), U16_TO_U8S_LE(startAddr + 0xBF), U16_TO_U8S_LE(startAddr + 0xC0), \
      U16_TO_U8S_LE(startAddr + 0xC1), U16_TO_U8S_LE(startAddr + 0xC2), U16_TO_U8S_LE(startAddr + 0xC3), U16_TO_U8S_LE(startAddr + 0xC4), \
      U16_TO_U8S_LE(startAddr + 0xC5), U16_TO_U8S_LE(startAddr + 0xC6), U16_TO_U8S_LE(startAddr + 0xC7), U16_TO_U8S_LE(startAddr + 0xC8), \
      U16_TO_U8S_LE(startAddr + 0xC9), U16_TO_U8S_LE(startAddr + 0xCA), U16_TO_U8S_LE(startAddr + 0xCB), U16_TO_U8S_LE(startAddr + 0xCC), \
      U16_TO_U8S_LE(startAddr + 0xCD), U16_TO_U8S_LE(startAddr + 0xCE), U16_TO_U8S_LE(startAddr + 0xCF), U16_TO_U8S_LE(startAddr + 0xD0), \
      U16_TO_U8S_LE(startAddr + 0xD1), U16_TO_U8S_LE(startAddr + 0xD2), U16_TO_U8S_LE(startAddr + 0xD3), U16_TO_U8S_LE(startAddr + 0xD4), \
      U16_TO_U8S_LE(startAddr + 0xD5), U16_TO_U8S_LE(startAddr + 0xD6), U16_TO_U8S_LE(startAddr + 0xD7), U16_TO_U8S_LE(startAddr + 0xD8), \
      U16_TO_U8S_LE(startAddr + 0xD9), U16_TO_U8S_LE(startAddr + 0xDA), U16_TO_U8S_LE(startAddr + 0xDB), U16_TO_U8S_LE(startAddr + 0xDC), \
      U16_TO_U8S_LE(startAddr + 0xDD), U16_TO_U8S_LE(startAddr + 0xDE), U16_TO_U8S_LE(startAddr + 0xDF), U16_TO_U8S_LE(startAddr + 0xE0), \
      U16_TO_U8S_LE(startAddr + 0xE1), U16_TO_U8S_LE(startAddr + 0xE2), U16_TO_U8S_LE(startAddr + 0xE3), U16_TO_U8S_LE(startAddr + 0xE4), \
      U16_TO_U8S_LE(startAddr + 0xE5), U16_TO_U8S_LE(startAddr + 0xE6), U16_TO_U8S_LE(startAddr + 0xE7), U16_TO_U8S_LE(startAddr + 0xE8), \
      U16_TO_U8S_LE(startAddr + 0xE9), U16_TO_U8S_LE(startAddr + 0xEA), U16_TO_U8S_LE(startAddr + 0xEB), U16_TO_U8S_LE(startAddr + 0xEC), \
      U16_TO_U8S_LE(startAddr + 0xED), U16_TO_U8S_LE(startAddr + 0xEE), U16_TO_U8S_LE(startAddr + 0xEF), U16_TO_U8S_LE(startAddr + 0xF0), \
      U16_TO_U8S_LE(startAddr + 0xF1), U16_TO_U8S_LE(startAddr + 0xF2), U16_TO_U8S_LE(startAddr + 0xF3), U16_TO_U8S_LE(startAddr + 0xF4), \
      U16_TO_U8S_LE(startAddr + 0xF5), U16_TO_U8S_LE(startAddr + 0xF6), U16_TO_U8S_LE(startAddr + 0xF7), U16_TO_U8S_LE(startAddr + 0xF8), \
      U16_TO_U8S_LE(startAddr + 0xF9), U16_TO_U8S_LE(startAddr + 0xFA), U16_TO_U8S_LE(startAddr + 0xFB), U16_TO_U8S_LE(startAddr + 0xFC), \
      U16_TO_U8S_LE(startAddr + 0xFD), U16_TO_U8S_LE(startAddr + 0xFE), U16_TO_U8S_LE(startAddr + 0xFF), U16_TO_U8S_LE(0xFFFF)

// The ramdisk
uint8_t msc_disk[ALLOCATED_DISK_BLOCK_NUM][DISK_BLOCK_SIZE] =
{
  //------------- Block0: Boot Sector -------------//
  {
      // Jump instruction
      0xEB, 0x3C, 0x90,
      // OEM Name
      CHARACTERIFY8("MSDOS5.0"),

      // BIOS Parameter Block
      // Bytes per sector
      U16_TO_U8S_LE(DISK_BLOCK_SIZE),
      // Sectors per cluster
      0x01,
      // Reserved sectors
      U16_TO_U8S_LE(NUM_RESERVED_SECTORS),
      // Number of copies of file allocation tables
      NUM_FAT_COPIES,
      // Number of root entries (maximum number of files under root)
      U16_TO_U8S_LE(NUM_ROOT_ENTRIES),
      // Number of sectors (small)
      U16_TO_U8S_LE(REPORTED_BLOCK_NUM),
      // Media type (hard disk)
      0xF8,
      // Sectors per FAT
      U16_TO_U8S_LE(SECTORS_PER_FAT),
      // Sectors per track
      U16_TO_U8S_LE(1),
      // Number of heads
      U16_TO_U8S_LE(1),
      // Hidden sectors
      U32_TO_U8S_LE(0),
      // Number of sectors (large) (only used if small value is 0)
      U32_TO_U8S_LE(0),
      // Physical disk number
      0x80,
      // Current head
      0x00,
      // signature (must be either 0x28 or 0x29)
      0x29,
      // Volume serial number
      0x34, 0x12, 0x00, 0x00,
      // Volume label (11 bytes) (no longer actually used)
      CHARACTERIFY11(VOLUME_LABEL11_STR),
      // System ID (8 bytes) (host doesn't even use this)
      CHARACTERIFY8("FAT16   "),

      // Zero up to 2 last bytes of FAT magic code
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      // FAT magic code
      0x55, 0xAA
  },

  //------------- Block1-9: FAT16 Table -------------//
  {
      // first 16 bit entry must be FFF8
      U16_TO_U8S_LE(0xFFF8),
      // End of chain indicator / maintenance flags (reserved for cluster 1)
      U16_TO_U8S_LE(0xFFFF),
      // The rest is pointers to next cluster number or 0xFFFF if end (for page 2+)
      U16_TO_U8S_LE(0xFFFF),
      // Bad sectors for the rest of this block of addresses
      U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7),
      U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7), U16_TO_U8S_LE(0xFFF7)
  },
  // Each one of these blocks address the 128KB needed for each memory file
  {FULL_PAGE_FAT_ENTRY(0x0100)},
  {FULL_PAGE_FAT_ENTRY(0x0200)},
  {FULL_PAGE_FAT_ENTRY(0x0300)},
  {FULL_PAGE_FAT_ENTRY(0x0400)},
  {FULL_PAGE_FAT_ENTRY(0x0500)},
  {FULL_PAGE_FAT_ENTRY(0x0600)},
  {FULL_PAGE_FAT_ENTRY(0x0700)},
  {FULL_PAGE_FAT_ENTRY(0x0800)},
  // Some hosts (looking at you, Windows) check if there is empty space available before overwriting
  // an EXISTING file of the same size. For that fact, 128 kb will look available to fill just so
  // the host proceeds past that check. This area is not actually writeable.
  {},

  //------------- Block11: Root Directory -------------//
  {
      // first entry is volume label
      VOLUME_ENTRY(),
      // second entry is readme file (will always be read only)
      SIMPLE_DIR_ENTRY("README  ", "TXT", ATTR1_READ_ONLY, ATTR2_LOWERCASE_EXT, FIRST_VALID_FAT_ADDRESS, README_SIZE),
  },

  //------------- Block12+: File Content -------------//
  {README_CONTENTS}
};

bool upper_found(const char* name, uint32_t len)
{
  for (uint32_t i = 0; i < len; ++i)
  {
    if (name[i] >= 'A' && name[i] <= 'Z')
    {
      return true;
    }
  }
  return false;
}

// Parses a filename string into name and extension fields
// Returns attribute 2 value
uint8_t parse_filename(const char* filename, uint8_t* name8, uint8_t* ext3)
{
  uint8_t rv = 0;

  // Make sure characters not set are padded with spaces
  memset(name8, ' ', 8);
  memset(ext3, ' ', 3);

  // Find extension position
  const char* dotPos = strchr(filename, '.');
  const char* extStart = nullptr;
  if (dotPos == NULL)
  {
    // Put pointer at end of string
    dotPos = filename + strlen(filename);
    extStart = dotPos;
  }
  else
  {
    extStart = dotPos + 1;
  }

  // Copy name
  uint32_t nameLen = dotPos - filename;
  if (nameLen > 8) nameLen = 8;
  if (!upper_found(filename, nameLen))
  {
    rv = rv | ATTR2_LOWERCASE_BASENAME;
  }
  for (uint8_t i = 0; i < nameLen; ++i)
  {
    *name8++ = CHAR_TO_UPPER(*filename);
    ++filename;
  }

  // Copy extension
  uint32_t extLen = strlen(extStart);
  if (extLen > 3) extLen = 3;
  if (!upper_found(extStart, 3))
  {
    rv = rv | ATTR2_LOWERCASE_EXT;
  }
  for (uint8_t i = 0; i < extLen; ++i)
  {
    *ext3++ = CHAR_TO_UPPER(*extStart);
    ++extStart;
  }

  return rv;
}

void set_file_entries()
{
    uint8_t* rootDirEntry = msc_disk[NUM_RESERVED_SECTORS + NUM_FAT_SECTORS];
    uint8_t const * rootEnd = msc_disk[NUM_HEADER_SECTORS];
    // Move past volume label and internal files
    rootDirEntry += ((1 + NUM_INTERNAL_FILES) * BYTES_PER_ROOT_ENTRY);

    // Set all files in fileEntries
    for (uint32_t i = 0;
         i < (sizeof(fileEntries) / sizeof(fileEntries[0])) && (rootDirEntry < rootEnd);
         ++i)
    {
      // File entry is only considered set if handle is set
      if (fileEntries[i].handle != nullptr)
      {
        memcpy(rootDirEntry, defaultRootEntry, sizeof(defaultRootEntry));
        // Parse filename into the name and extension fields
        rootDirEntry[12] = parse_filename(fileEntries[i].filename, rootDirEntry, rootDirEntry + 8);
        // Set address and size
        uint8_t addrAndSize[6] = {U16_TO_U8S_LE(fileEntries[i].startBlock),
                                  U32_TO_U8S_LE(fileEntries[i].size)};
        memcpy(rootDirEntry + (BYTES_PER_ROOT_ENTRY - 6), addrAndSize, 6);
        // Set read only flag is it is set
        if (fileEntries[i].isReadOnly)
        {
          rootDirEntry[11] |= ATTR1_READ_ONLY;
        }
        // Move to next entry in FAT
        rootDirEntry += BYTES_PER_ROOT_ENTRY;
      }
    }

    // Empty out everything else
    if (rootDirEntry < rootEnd)
    {
      memset(rootDirEntry, 0, rootEnd - rootDirEntry);
    }
}

void usb_msc_add(UsbFile* file)
{
  LockGuard lockGuard(*fileMutex);
  assert(lockGuard.isLocked());

  const char* filename = file->getFileName();
  if (*filename != '\0')
  {
    // Find first empty slot and add file
    for (uint32_t i = 0;
         i < (sizeof(fileEntries) / sizeof(fileEntries[0]));
         ++i)
    {
      if (fileEntries[i].handle == nullptr)
      {
        fileEntries[i].handle = file;
        fileEntries[i].filename = file->getFileName();
        fileEntries[i].size = file->getFileSize();
        if (fileEntries[i].size > MAX_FILE_SIZE_BYTES)
        {
          fileEntries[i].size = MAX_FILE_SIZE_BYTES;
        }
        fileEntries[i].startBlock = START_EXTERNAL_FILE_BLOCK + (i * BLOCKS_PER_FILE);
        fileEntries[i].numBlocks = INT_DIVIDE_CEILING(fileEntries[i].size, DISK_BLOCK_SIZE);
        fileEntries[i].isReadOnly = file->isReadOnly();
        ++numFileEntries;
        set_file_entries();
        new_data = true;
        break;
      }
    }
  }
}

void usb_msc_remove(UsbFile* file)
{
  LockGuard lockGuard(*fileMutex);
  assert(lockGuard.isLocked());

  // Allow the drive to be used again if it ejected due to failure
  errorCount = 0;

  // Find entry with matching file and remove it
  for (uint32_t i = 0;
       i < (sizeof(fileEntries) / sizeof(fileEntries[0]));
       ++i)
  {
      if (fileEntries[i].handle == file)
      {
        fileEntries[i].handle = nullptr;
        fileEntries[i].filename = nullptr;
        fileEntries[i].size = 0;
        fileEntries[i].startBlock = 0;
        --numFileEntries;
        set_file_entries();
        // Only tell host to update data if device isn't ejected
        new_data = true;
        break;
      }
  }
}

class UsbMscFileSystem : public UsbFileSystem
{
  public:
    virtual void add(UsbFile* file) final
    {
      usb_msc_add(file);
    }

    virtual void remove(UsbFile* file) final
    {
      usb_msc_remove(file);
    }

};

static UsbMscFileSystem fileSystem;

UsbFileSystem& usb_msc_get_file_system()
{
  return fileSystem;
}

void msc_init(MutexInterface* mutex)
{
  fileMutex = mutex;
}

// Invoked when received SCSI_CMD_INQUIRY
// Application fill vendor id, product id and revision with string up to 8, 16, 4 characters respectively
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
  (void) lun;

  const char vid[] = "OngFx86";
  const char pid[] = "Dreamcast Ctrlr";
  const char rev[] = "1.0";

  memcpy(vendor_id  , vid, strlen(vid));
  memcpy(product_id , pid, strlen(pid));
  memcpy(product_rev, rev, strlen(rev));
}

// Invoked when received Test Unit Ready command.
// return true allowing host to read/write this LUN e.g SD card inserted
bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
  (void) lun;

  if (errorCount >= MAX_ERROR_COUNT)
  {
    // Force eject
    ejected = true;
  }
  else if (new_data)
  {
    new_data = false;
    // Only cause drive to reattach iff there is at least 1 file present
    if (numFileEntries > 0)
    {
      ejected = false;
      tud_msc_set_sense(lun, SCSI_SENSE_UNIT_ATTENTION, 0x28, 0x00);
      return false;
    }
    else
    {
      // Cause eject if no files present
      ejected = true;
    }
  }

  if (ejected)
  {
    tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00);
    return false;
  }

  return true;
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine the disk size
// Application update block count and block size
void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size)
{
  (void) lun;

  *block_count = REPORTED_BLOCK_NUM;
  *block_size  = DISK_BLOCK_SIZE;
}

// Invoked when received Start Stop Unit command
// - Start = 0 : stopped power mode, if load_eject = 1 : unload disk storage
// - Start = 1 : active mode, if load_eject = 1 : load disk storage
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
  (void) lun;
  (void) power_condition;

  if ( load_eject )
  {
    if (start)
    {
      ejected = (numFileEntries == 0 || errorCount >= MAX_ERROR_COUNT);
      if (ejected)
      {
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00);
        return false;
      }
    }
    else
    {
      // unload disk storage
      ejected = true;
    }
  }

  return true;
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
  (void) lun;

  int32_t numRead = -1;

  if (lba < ALLOCATED_DISK_BLOCK_NUM)
  {
    // RAM disk area
    uint8_t const* addr = msc_disk[lba] + offset;
    memcpy(buffer, addr, bufsize);
    numRead = bufsize;
  }
  else if (lba < ALLOCATED_DISK_BLOCK_NUM + BAD_SECTOR_DISK_BLOCK_NUM)
  {
    // Attempt to read bad sectors (just send back zeros)
    memset(buffer, 0, bufsize);
    numRead = bufsize;
  }
  else if (lba < ALLOCATED_DISK_BLOCK_NUM + BAD_SECTOR_DISK_BLOCK_NUM + EXTERNAL_DISK_BLOCK_NUM)
  {
    // Serialize this section with file add/remove
    LockGuard lockGuard(*fileMutex);
    assert(lockGuard.isLocked());

    // This actually works out perfectly since each read will be up to 1 block of data, our block of
    // data is 512 bytes, and a VMU block of data is also 512 bytes.

    uint32_t realAddr = lba + FIRST_VALID_FAT_ADDRESS - NUM_HEADER_SECTORS;

    // Find the file which contains this address
    for (uint32_t i = 0;
        i < (sizeof(fileEntries) / sizeof(fileEntries[0]));
        ++i)
    {
        if (realAddr >= fileEntries[i].startBlock && realAddr < (fileEntries[i].startBlock + fileEntries[i].numBlocks))
        {
          // Found the matching file!
          uint32_t vmuAddr = realAddr & 0xFF;
          numRead = fileEntries[i].handle->read(vmuAddr, buffer, bufsize, 20000);
          if (numRead < 0)
          {
            // timeout
            tud_msc_set_sense(lun, SCSI_SENSE_ABORTED_COMMAND, 0x1B, 0x00);
            if (errorCount < MAX_ERROR_COUNT)
            {
              ++errorCount;
            }
          }
          break;
        }
    }
  }
  else if (lba < REPORTED_BLOCK_NUM)
  {
    // Fake data (host shouldn't attempt to read here, but return data anyway)
    memset(buffer, 0, bufsize);
    numRead = bufsize;
  }
  else
  {
    // Attempt to read out of bounds
    numRead = 0;
  }

  return numRead;
}

bool tud_msc_is_writable_cb (uint8_t lun)
{
  (void) lun;

  return true;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and return number of written bytes
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
{
  (void) lun;

  int32_t numWrite = -1;

  if (lba < ALLOCATED_DISK_BLOCK_NUM)
  {
    // RAM disk area
    uint8_t const* addr = msc_disk[lba] + offset;
    if (lba == NUM_RESERVED_SECTORS + NUM_FAT_SECTORS)
    {
      // Special case: allow host to write only if it isn't changing important things
      bool ok = true;
      for (uint32_t idx = offset; idx < offset + bufsize; ++idx, ++addr, ++buffer)
      {
        if (*addr != *buffer)
        {
          uint32_t entryIdx = idx % BYTES_PER_ROOT_ENTRY;
          // If it touches name field or location, that's bad!
          if ((entryIdx >= 0 && entryIdx <= 10) || entryIdx == 26 || entryIdx == 27)
          {
            ok = false;
            break;
          }
        }
      }

      if (ok)
      {
        // Tell the host this was successful, but don't actually change anything
        numWrite = bufsize;
      }
      else
      {
        tud_msc_set_sense(lun, SCSI_SENSE_DATA_PROTECT, 0x00, 0x06);
        numWrite = -1;
      }
    }
    else if (memcmp(addr, buffer, bufsize) == 0)
    {
      // Write ok since it matches
      numWrite = bufsize;
    }
    else
    {
      // Don't allow the host to write
      tud_msc_set_sense(lun, SCSI_SENSE_DATA_PROTECT, 0x00, 0x06);
      numWrite = -1;
    }
  }
  else if (lba < ALLOCATED_DISK_BLOCK_NUM + BAD_SECTOR_DISK_BLOCK_NUM)
  {
    // Attempt to write bad sectors
    tud_msc_set_sense(lun, SCSI_SENSE_DATA_PROTECT, 0x00, 0x06);
    numWrite = -1;
  }
  else if (lba < ALLOCATED_DISK_BLOCK_NUM + BAD_SECTOR_DISK_BLOCK_NUM + EXTERNAL_DISK_BLOCK_NUM)
  {
    // Serialize this section with file add/remove
    LockGuard lockGuard(*fileMutex);
    assert(lockGuard.isLocked());

    // This actually works out perfectly since each read will be up to 1 block of data, our block of
    // data is 512 bytes, and a VMU block of data is also 512 bytes.

    uint32_t realAddr = lba + FIRST_VALID_FAT_ADDRESS - NUM_HEADER_SECTORS;

    // Find the file which contains this address
    bool found = false;
    for (uint32_t i = 0;
        i < (sizeof(fileEntries) / sizeof(fileEntries[0]));
        ++i)
    {
        if (realAddr >= fileEntries[i].startBlock && realAddr < (fileEntries[i].startBlock + fileEntries[i].numBlocks))
        {
          // Found the matching file!
          found = true;
          uint32_t vmuAddr = realAddr & 0xFF;
          if (!fileEntries[i].isReadOnly)
          {
            numWrite = fileEntries[i].handle->write(vmuAddr, buffer, bufsize, 250000);
            if (numWrite < 0)
            {
              // timeout
              tud_msc_set_sense(lun, SCSI_SENSE_HARDWARE_ERROR, 0x44, 0x00);
              if (errorCount < MAX_ERROR_COUNT)
              {
                ++errorCount;
              }
            }
          }
          else
          {
            // Throw a data protect error to stop the host from writing here
            tud_msc_set_sense(lun, SCSI_SENSE_DATA_PROTECT, 0x00, 0x06);
            numWrite = -1;
          }
          break;
        }
    }

    if (!found)
    {
      tud_msc_set_sense(lun, SCSI_SENSE_DATA_PROTECT, 0x00, 0x06);
      numWrite = -1;
    }
  }
  else if (lba < REPORTED_BLOCK_NUM)
  {
    // Fake data (host shouldn't attempt to write here)
    tud_msc_set_sense(lun, SCSI_SENSE_DATA_PROTECT, 0x00, 0x06);
    numWrite = -1;
  }
  else
  {
    // Attempt to read out of bounds
    tud_msc_set_sense(lun, SCSI_SENSE_DATA_PROTECT, 0x00, 0x06);
    numWrite = -1;
  }

  return numWrite;
}

// Callback invoked when received an SCSI command not in built-in list below
// - READ_CAPACITY10, READ_FORMAT_CAPACITY, INQUIRY, MODE_SENSE6, REQUEST_SENSE
// - READ10 and WRITE10 has their own callbacks
int32_t tud_msc_scsi_cb (uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize)
{
  // read10 & write10 has their own callback and MUST not be handled here

  void const* response = NULL;
  int32_t resplen = 0;

  // most scsi handled is input
  bool in_xfer = true;

  switch (scsi_cmd[0])
  {
    case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
      // Host is about to read/write etc ... better not to disconnect disk
      resplen = 0;
    break;

    default:
      // Set Sense = Invalid Command Operation
      tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);

      // negative means error -> tinyusb could stall and/or response with failed status
      resplen = -1;
    break;
  }

  // return resplen must not larger than bufsize
  if ( resplen > bufsize ) resplen = bufsize;

  if ( response && (resplen > 0) )
  {
    if(in_xfer)
    {
      memcpy(buffer, response, resplen);
    }else
    {
      // SCSI output
    }
  }

  return resplen;
}
