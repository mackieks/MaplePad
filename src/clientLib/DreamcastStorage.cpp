#include "DreamcastStorage.hpp"

#include "dreamcast_constants.h"

#include <memory>
#include <string.h>

#define U16_TO_UPPER_HALF_WORD(val) ((static_cast<uint32_t>(val) << 24) | ((static_cast<uint32_t>(val) << 8) & 0x00FF0000))
#define U16_TO_LOWER_HALF_WORD(val) (((static_cast<uint32_t>(val) << 8) & 0x0000FF00) | ((static_cast<uint32_t>(val) >> 8) & 0x000000FF))

client::DreamcastStorage::DreamcastStorage(std::shared_ptr<SystemMemory> systemMemory, uint32_t memoryOffset) :
    DreamcastPeripheralFunction(DEVICE_FN_STORAGE),
    mSystemMemory(systemMemory),
    mMemoryOffset(memoryOffset),
    mDataBlock{}
{}

const uint8_t* client::DreamcastStorage::readBlock(uint16_t blockNum)
{
    uint32_t size = BYTES_PER_BLOCK;
    const uint8_t* mem =
        mSystemMemory->read(mMemoryOffset + (blockNum * BYTES_PER_BLOCK), size);
    if (size < BYTES_PER_BLOCK)
    {
        // Failure
        mem = nullptr;
    }

    return mem;
}

bool client::DreamcastStorage::writeBlock(uint16_t blockNum, void* data)
{
    uint32_t size = BYTES_PER_BLOCK;
    if (blockNum == (SYSTEM_BLOCK_NO - NUM_SYSTEM_BLOCKS + 1))
    {
        // Dreamcast does something weird where the save area block count and number is seemingly
        // expected to be overridden by the memory unit itself if host sets it to 0
        uint32_t* data32 = reinterpret_cast<uint32_t*>(data);
        uint32_t* saveAreaWord = data32 + MEDIA_INFO_WORD_OFFSET + SAVE_AREA_MEDIA_INFO_OFFSET;
        if (*saveAreaWord == 0)
        {
            setDefaultMediaInfoFlipped(saveAreaWord, SAVE_AREA_MEDIA_INFO_OFFSET, 1);
        }
    }
    return mSystemMemory->write(mMemoryOffset + (blockNum * BYTES_PER_BLOCK), data, size);
}

void client::DreamcastStorage::setDefaultMediaInfo(uint32_t* out, uint8_t infoOffset, uint8_t infoLen)
{
    static const uint32_t executionFile = 0x00008000;
    static const uint32_t mediaInfo[6] = {
        U16_TO_UPPER_HALF_WORD(NUM_BLOCKS - 1) | U16_TO_LOWER_HALF_WORD(0),
        U16_TO_UPPER_HALF_WORD(SYSTEM_BLOCK_NO) | U16_TO_LOWER_HALF_WORD(FAT_BLOCK_NO),
        U16_TO_UPPER_HALF_WORD(NUM_FAT_BLOCKS) | U16_TO_LOWER_HALF_WORD(FILE_INFO_BLOCK_NO),
        U16_TO_UPPER_HALF_WORD(NUM_FILE_INFO_BLOCKS) | U16_TO_LOWER_HALF_WORD(0),
        U16_TO_UPPER_HALF_WORD(NUM_SAVE_AREA_BLOCKS) | U16_TO_LOWER_HALF_WORD(SAVE_AREA_BLOCK_NO),
        executionFile
    };
    memcpy(out, mediaInfo + infoOffset, infoLen * sizeof(mediaInfo[0]));
}

void client::DreamcastStorage::setDefaultMediaInfoFlipped(uint32_t* out, uint8_t infoOffset, uint8_t infoLen)
{
    setDefaultMediaInfo(out, infoOffset, infoLen);
    while (infoLen-- > 0)
    {
        *out = flipWordBytes(*out);
        ++out;
    }
}

bool client::DreamcastStorage::format()
{
    //
    // System Block
    //
    uint32_t* systemBlock = reinterpret_cast<uint32_t*>(mDataBlock);
    memset(systemBlock, 0, BYTES_PER_BLOCK);
    // There should only ever be 1 system block, but if there is more, clear up to system block num
    for (uint32_t i = 0; i < NUM_SYSTEM_BLOCKS - 1; ++i)
    {
        if (!writeBlock(SYSTEM_BLOCK_NO - i, systemBlock))
        {
            return false;
        }
    }
    memset(systemBlock, 0x55, 4 * WORD_SIZE);

    // I've seen the following data on some VMUs but not others. It doesn't seem necessary, so this
    // is omitted until I have better understanding of the purpose.
#if 0
    systemBlock[4] = flipWordBytes(0x01FFFFFF);
    systemBlock[5] = flipWordBytes(0xFF000000);
#endif

    // Date/time markers
    systemBlock[12] = flipWordBytes(0x19990909);
    systemBlock[13] = flipWordBytes(0x00001000);
    // Media info data
    setDefaultMediaInfoFlipped(&systemBlock[MEDIA_INFO_WORD_OFFSET]);
    if (!writeBlock(SYSTEM_BLOCK_NO - NUM_SYSTEM_BLOCKS + 1, systemBlock))
    {
        return false;
    }
    //
    // FAT Block(s)
    //
    uint32_t* fatBlock = reinterpret_cast<uint32_t*>(mDataBlock);
    for (uint32_t i = 0; i < WORDS_PER_BLOCK; ++i)
    {
        fatBlock[i] = flipWordBytes(U16_TO_UPPER_HALF_WORD(0xFFFC) | U16_TO_LOWER_HALF_WORD(0xFFFC));
    }
    for (uint32_t i = 0; i < NUM_FAT_BLOCKS - 1; ++i)
    {
        if (!writeBlock(FAT_BLOCK_NO - i, fatBlock))
        {
            return false;
        }
    }
    fatBlock += (WORDS_PER_BLOCK - 1);
    bool lower = true;
    for (uint32_t i = 0; i < (NUM_SYSTEM_BLOCKS + NUM_FAT_BLOCKS); ++i)
    {
        setFatAddr(fatBlock, lower, 0xFFFA);
    }
    uint16_t addr = NUM_BLOCKS - NUM_SYSTEM_BLOCKS - NUM_FAT_BLOCKS - 1;
    for (uint32_t i = 0; i < NUM_FILE_INFO_BLOCKS - 1; ++i)
    {
        setFatAddr(fatBlock, lower, --addr);
    }
    setFatAddr(fatBlock, lower, 0xFFFA);
    fatBlock = reinterpret_cast<uint32_t*>(mDataBlock);
    if (!writeBlock(FAT_BLOCK_NO - NUM_FAT_BLOCKS + 1, fatBlock))
    {
        return false;
    }
    //
    // File Info Blocks
    //
    uint32_t* fileInfoBlock = reinterpret_cast<uint32_t*>(mDataBlock);
    memset(fileInfoBlock, 0, BYTES_PER_BLOCK);
    for (uint32_t i = 0; i < NUM_FILE_INFO_BLOCKS; ++i)
    {
        if (!writeBlock(FILE_INFO_BLOCK_NO - i, fileInfoBlock))
        {
            return false;
        }
    }
    return true;
}

bool client::DreamcastStorage::handlePacket(const MaplePacket& in, MaplePacket& out)
{
    const uint8_t cmd = in.frame.command;
    switch (cmd)
    {
        case COMMAND_GET_MEMORY_INFORMATION:
        {
            const uint8_t* mem = readBlock(SYSTEM_BLOCK_NO);
            if (mem != nullptr)
            {
                out.frame.command = COMMAND_RESPONSE_DATA_XFER;
                // This media info is usually stored at block 16 offset in the system block. If
                // memory is wiped though, the Dreamcast will get confused by the returned media
                // info and not know how to manage data. Thus, this info is made static here with
                // the exceptions of the volume icon and save area information.
                const uint32_t* storageMediaInfo =
                    reinterpret_cast<const uint32_t*>(mem) + MEDIA_INFO_WORD_OFFSET;
                uint32_t payload[7] = {mFunctionCode};
                setDefaultMediaInfo(&payload[1]);
                payload[4] = (payload[4] & 0xFFFF0000) | (flipWordBytes(storageMediaInfo[3]) & 0x0000FFFF);
                payload[5] = flipWordBytes(storageMediaInfo[4]);

                out.setPayload(payload, 7);
            }
            else
            {
                // Drive not ready error
                out.frame.command = COMMAND_RESPONSE_FILE_ERROR;
                out.setPayload(0x00000040);
            }
            return true;
        }
        break;

        case COMMAND_BLOCK_READ:
        {
            if (in.payload.size() > 1)
            {
                uint32_t locationWord = in.payload[1];
                uint16_t blockOffset = locationWord & 0xFFFF;
                uint8_t phase = (locationWord >> 16) & 0xFF;
                if (blockOffset >= NUM_BLOCKS || phase != 0)
                {
                    // Outside range
                    out.frame.command = COMMAND_RESPONSE_FILE_ERROR;
                    out.setPayload(0x00000010);
                }
                else
                {
                    const uint8_t* mem = readBlock(blockOffset);
                    if (mem != nullptr)
                    {
                        out.frame.command = COMMAND_RESPONSE_DATA_XFER;
                        out.reservePayload(WORDS_PER_BLOCK + 2);
                        out.setPayload(&mFunctionCode, 1);
                        out.appendPayload(locationWord);
                        out.appendPayloadFlipWords(reinterpret_cast<const uint32_t*>(mem), WORDS_PER_BLOCK);
                    }
                    else
                    {
                        // Drive not ready error
                        out.frame.command = COMMAND_RESPONSE_FILE_ERROR;
                        out.setPayload(0x00000040);
                    }
                }
                return true;
            }
        }
        break;

        case COMMAND_BLOCK_WRITE:
        {
            if (in.payload.size() > 1)
            {
                uint32_t locationWord = in.payload[1];
                uint16_t blockOffset = locationWord & 0xFFFF;
                uint8_t phase = (locationWord >> 16) & 0xFF;
                if (blockOffset >= NUM_BLOCKS
                    || phase >= WRITES_PER_BLOCK
                    || (in.payload.size() - 2) != (BYTES_PER_WRITE / 4))
                {
                    // Outside range
                    out.frame.command = COMMAND_RESPONSE_FILE_ERROR;
                    out.setPayload(0x00000010);
                }
                else
                {
                    uint32_t byteOffset = (phase * BYTES_PER_WRITE);
                    uint32_t numBytes = sizeof(uint32_t);
                    uint8_t* outPtr = &mDataBlock[byteOffset];
                    const uint32_t* inPtr = &in.payload[2];
                    for (uint32_t i = 0; i < BYTES_PER_WRITE; i+=4, outPtr+=numBytes, ++inPtr)
                    {
                        if (i + numBytes > BYTES_PER_WRITE)
                        {
                            // Last chunk is not full 4 bytes
                            // (this should never happen with standard DC storage)
                            numBytes = BYTES_PER_WRITE - i;
                        }
                        uint32_t flippedWord = flipWordBytes(*inPtr);
                        memcpy(outPtr, &flippedWord, numBytes);
                    }

                    out.frame.command = COMMAND_RESPONSE_ACK;
                }
                return true;
            }
        }
        break;

        case COMMAND_GET_LAST_ERROR:
        {
            if (in.payload.size() > 1)
            {
                uint32_t locationWord = in.payload[1];
                uint16_t blockOffset = locationWord & 0xFFFF;
                uint8_t phase = (locationWord >> 16) & 0xFF;
                if (blockOffset >= NUM_BLOCKS || phase != WRITES_PER_BLOCK)
                {
                    // Outside range
                    out.frame.command = COMMAND_RESPONSE_FILE_ERROR;
                    out.setPayload(0x00000010);
                }
                else
                {
                    if (writeBlock(blockOffset, mDataBlock))
                    {
                        out.frame.command = COMMAND_RESPONSE_ACK;
                    }
                    else
                    {
                        out.frame.command = COMMAND_RESPONSE_FILE_ERROR;
                        out.setPayload(0x00000008);
                    }
                }
                return true;
            }
        }
        break;

        default:
        {
        }
        break;
    }
    return false;
}

void client::DreamcastStorage::reset()
{}

uint32_t client::DreamcastStorage::getFunctionDefinition()
{
    return (
        (IS_REMOVABLE ? 0x00000080 : 0)
        | (CRC_NEEDED ? 0x00000040 : 0)
        | ((READS_PER_BLOCK & 0xF) << 8)
        | ((WRITES_PER_BLOCK & 0xF) << 12)
        | (((BYTES_PER_BLOCK / 32 - 1) & 0xFF) << 16)
        | (((NUMBER_OF_PARTITIONS - 1) & 0xFF) << 24)
    );
}

void client::DreamcastStorage::setFatAddr(uint32_t*& fatBlock, bool& lower, uint16_t value)
{
    if (lower)
    {
        *fatBlock = flipWordBytes((flipWordBytes(*fatBlock) & 0xFFFF0000) | U16_TO_LOWER_HALF_WORD(value));
    }
    else
    {
        *fatBlock = flipWordBytes(U16_TO_UPPER_HALF_WORD(value) | (flipWordBytes(*fatBlock) & 0x0000FFFF));
        --fatBlock;
    }
    lower = !lower;
}
