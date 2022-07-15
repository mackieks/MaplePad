#ifdef __cplusplus
extern "C"
{
#endif

#define CARD_BLOCKS 256
#define ROOT_BLOCK 255
#define FAT_BLOCK 254
#define NUM_FAT_BLOCKS 1
#define DIRECTORY_BLOCK 253
#define NUM_DIRECTORY_BLOCKS 13
#define SAVE_BLOCK 200
#define NUM_SAVE_BLOCKS 31 // Not sure what this means

#define BLOCK_SIZE 512

uint32_t CheckFormatted(uint8_t* MemoryCard);

#ifdef __cplusplus
}
#endif