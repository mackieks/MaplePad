/**
 * Pop'n'Music controller
 * Dreamcast Maple Bus Transiever example for Raspberry Pi Pico (RP2040)
 * (C) Charlie Cole 2021
 *
 * Dreamcast controller connector pin 1 (Data) to 14 (PICO_PIN1_PIN)
 * Dreamcast controller connector pin 5 (Data) to 15 (PICO_PIN5_PIN)
 * Dreamcast controller connector pin 2 (5V) to VSYS (I did via diode so could power from either USB/VBUS or Dreamcast safely)
 * Dreamcast controller connector pins 3 (GND) and 4 (Sense) to GND
 * GPIO pins for buttons 16-22,26 and 27 (uses internal pullups, switch to GND. See ButtonInfos)
 * LEDs on pins 13-5 (to white LED anode through resistor (say 82ohms) to GND. See ButtonInfos)
 * 
 * Maple TX done completely in PIO. Sends start of packet, data and end of packet. Fed by DMA so fire and forget.
 *
 * Maple RX done mostly in software on core 1. PIO just waits for transitions and shifts in whenever data pins change.
 * For maximum speed the RX state machine is implemented in lookup table to process 4 transitions at a time
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/flash.h"
#include "pico/multicore.h"
#include "maple.pio.h"
#include "state_machine.h"
#include "format.h"

#define SHOULD_SEND 1		// Set to zero to sniff two devices sending signals to each other
#define SHOULD_PRINT 0		// Nice for debugging but can cause timing issues

#define POPNMUSIC 1			// Pop'n Music controller or generic controller
#define NUM_BUTTONS	9		// On a Pop'n Music controller
#define FADE_SPEED 8		// How fast the LEDs fade after press
#define START_BUTTON 0x0008	// Bitmask for the start button
#define START_MASK 0x0251	// Key combination for Start as we don't have a dedicated button
#define PHASE_SIZE (BLOCK_SIZE / 4)

#define FLASH_WRITE_DELAY	16    // About quarter of a second if polling once a frame
#define FLASH_OFFSET (512 * 1024) // How far into Flash to store the memory card data
								  // We only have around 16Kb of code so assuming this will be fine

#define PICO_PIN1_PIN	14
#define PICO_PIN5_PIN	15
#define PICO_PIN1_PIN_RX	PICO_PIN1_PIN
#define PICO_PIN5_PIN_RX	PICO_PIN5_PIN

#define ADDRESS_DREAMCAST 0
#define ADDRESS_CONTROLLER 0x20
#define ADDRESS_SUBPERIPHERAL 0x01
#define ADDRESS_CONTROLLER_AND_SUBS (ADDRESS_CONTROLLER|ADDRESS_SUBPERIPHERAL)
#define ADDRESS_PORT_MASK 0xC0
#define ADDRESS_PERIPHERAL_MASK (~ADDRESS_PORT_MASK)

#define TXPIO pio0
#define RXPIO pio1

#define SWAP4(x) do { x=__builtin_bswap32(x); } while(0)

typedef enum ESendState_e
{
	SEND_NOTHING,
	SEND_CONTROLLER_INFO,
	SEND_CONTROLLER_STATUS,
	SEND_MEMORY_CARD_INFO,
	SEND_MEMORY_INFO,
	SEND_ACK,
	SEND_DATA
} ESendState;

enum ECommands
{
	CMD_RESPOND_FILE_ERROR = -5,
	CMD_RESPOND_SEND_AGAIN = -4,
	CMD_RESPOND_UNKNOWN_COMMAND = -3,
	CMD_RESPOND_FUNC_CODE_UNSUPPORTED = -2,
	CMD_NO_RESPONSE = -1,
	CMD_REQUEST_DEVICE_INFO = 1,
	CMD_REQUEST_EXTENDED_DEVICE_INFO,
	CMD_RESET_DEVICE,
	CMD_SHUTDOWN_DEVICE,
	CMD_RESPOND_DEVICE_INFO,
	CMD_RESPOND_EXTENDED_DEVICE_INFO,
	CMD_RESPOND_COMMAND_ACK,
	CMD_RESPOND_DATA_TRANSFER,
	CMD_GET_CONDITION,
	CMD_GET_MEMORY_INFO,
	CMD_BLOCK_READ,
	CMD_BLOCK_WRITE,
	CMD_BLOCK_COMPLETE_WRITE,
	CMD_SET_CONDITION
};

enum EFunction
{
	FUNC_CONTROLLER = 1,
	FUNC_MEMORY_CARD = 2
};

typedef struct PacketHeader_s
{
	int8_t Command;
	uint8_t Recipient;
	uint8_t Sender;
	uint8_t NumWords;
} PacketHeader;

typedef struct PacketDeviceInfo_s
{
	uint Func;			// Nb. Big endian
	uint FuncData[3];	// Nb. Big endian
	int8_t AreaCode;
	uint8_t ConnectorDirection;
	char ProductName[30];
	char ProductLicense[60];
	uint16_t StandbyPower;
	uint16_t MaxPower;
} PacketDeviceInfo;

typedef struct PacketMemoryInfo_s
{
	uint Func;			// Nb. Big endian
	uint16_t TotalSize;
	uint16_t ParitionNumber;
	uint16_t SystemArea;
	uint16_t FATArea;
	uint16_t NumFATBlocks;
	uint16_t FileInfoArea;
	uint16_t NumInfoBlocks;
	uint8_t VolumeIcon;
	uint8_t Reserved;
	uint16_t SaveArea;
	uint16_t NumSaveBlocks;
	uint Reserved32;
} PacketMemoryInfo;

typedef struct PacketControllerCondition_s
{
	uint Condition;		// Nb. Big endian
	uint16_t Buttons;
	uint8_t RightTrigger;
	uint8_t LeftTrigger;
	uint8_t JoyX;
	uint8_t JoyY;
	uint8_t JoyX2;
	uint8_t JoyY2;
} PacketControllerCondition;

typedef struct PacketBlockRead_s
{
	uint Func;			// Nb. Big endian
	uint Address;
	uint8_t Data[BLOCK_SIZE];
} PacketBlockRead;

typedef struct FACKPacket_s
{
	uint BitPairsMinus1;
	PacketHeader Header;
	uint CRC;
} FACKPacket;

typedef struct FInfoPacket_s
{
	uint BitPairsMinus1;
	PacketHeader Header;
	PacketDeviceInfo Info;
	uint CRC;
} FInfoPacket;

typedef struct FMemoryInfoPacket_s
{
	uint BitPairsMinus1;
	PacketHeader Header;
	PacketMemoryInfo Info;
	uint CRC;
} FMemoryInfoPacket;

typedef struct FControllerPacket_s
{
	uint BitPairsMinus1;
	PacketHeader Header;
	PacketControllerCondition Controller;
	uint CRC;
} FControllerPacket;

typedef struct FBlockReadResponsePacket_s
{
	uint BitPairsMinus1;
	PacketHeader Header;
	PacketBlockRead BlockRead;
	uint CRC;
} FBlockReadResponsePacket;

typedef struct ButtonInfo_s
{
	int InputIO;
	int OutputIO;
	int DCButtonMask;
	int Fade;
} ButtonInfo;

static ButtonInfo ButtonInfos[NUM_BUTTONS]=
{
	{ 16, 13, 0x0040, 0 },	// White left
	{ 17, 12, 0x0010, 0 },	// Yellow left
	{ 18, 11, 0x0020, 0 },	// Green left
	{ 19, 10, 0x0080, 0 },	// Blue left
	{ 20, 9, 0x0004, 0 },	// Red centre
	{ 21, 8, 0x0400, 0 },	// Blue right
	{ 22, 7, 0x0002, 0 },	// Green right
	{ 26, 6, 0x0200, 0 },	// Yellow right
	{ 27, 5, 0x0001, 0 }	// White right
};

// Buffers
static uint8_t RecieveBuffer[4096] __attribute__ ((aligned(4))); // Ring buffer for reading packets
static uint8_t Packet[1024 + 8] __attribute__ ((aligned(4))); // Temp buffer for consuming packets (could remove)
static FControllerPacket ControllerPacket; // Send buffer for controller status packet (pre-built for speed)
static FInfoPacket InfoPacket;	// Send buffer for controller info packet (pre-built for speed)
static FInfoPacket SubPeripheralInfoPacket;	// Send buffer for memory card info packet (pre-built for speed)
static FMemoryInfoPacket MemoryInfoPacket;	// Send buffer for memory card info packet (pre-built for speed)
static FACKPacket ACKPacket; // Send buffer for ACK packet (pre-built for speed)
static FBlockReadResponsePacket DataPacket; // Send buffer for ACK packet (pre-built for speed)

static ESendState NextPacketSend = SEND_NOTHING;
static uint OriginalControllerCRC = 0;
static uint OriginalReadBlockResponseCRC = 0;
static uint TXDMAChannel = 0;
static bool bPressingStart = false;
static uint CurrentPort = 0;

static uint8_t MemoryCard[128 * 1024];
static uint SectorDirty = 0;
static uint SendBlockAddress = ~0u;
static uint MessagesSinceWrite = FLASH_WRITE_DELAY;

uint CalcCRC(const uint* Words, uint NumWords)
{
	uint XOR_Checksum = 0;
	for (uint i = 0; i < NumWords; i++)
	{
		XOR_Checksum ^= *(Words++);
	}
	XOR_Checksum ^= (XOR_Checksum << 16);
	XOR_Checksum ^= (XOR_Checksum << 8);
	return XOR_Checksum;
}

void BuildACKPacket()
{
	ACKPacket.BitPairsMinus1 = (sizeof(ACKPacket) - 7) * 4 - 1;

	ACKPacket.Header.Command = CMD_RESPOND_COMMAND_ACK;
	ACKPacket.Header.Recipient = ADDRESS_DREAMCAST;
	ACKPacket.Header.Sender = ADDRESS_SUBPERIPHERAL;
	ACKPacket.Header.NumWords = 0;
	
	ACKPacket.CRC = CalcCRC((uint *)&ACKPacket.Header, sizeof(ACKPacket) / sizeof(uint) - 2);
}

void BuildInfoPacket()
{
	InfoPacket.BitPairsMinus1 = (sizeof(InfoPacket) - 7) * 4 - 1;

	InfoPacket.Header.Command = CMD_RESPOND_DEVICE_INFO;
	InfoPacket.Header.Recipient = ADDRESS_DREAMCAST;
	InfoPacket.Header.Sender = ADDRESS_CONTROLLER_AND_SUBS;
	InfoPacket.Header.NumWords = sizeof(InfoPacket.Info) / sizeof(uint);

	InfoPacket.Info.Func = __builtin_bswap32(FUNC_CONTROLLER);
#if POPNMUSIC
	InfoPacket.Info.FuncData[0] = __builtin_bswap32(0x000006ff); // What buttons it supports
#else
	InfoPacket.Info.FuncData[0] = __builtin_bswap32(0x000f06fe); // What buttons it supports
#endif
	InfoPacket.Info.FuncData[1] = 0;
	InfoPacket.Info.FuncData[2] = 0;
	InfoPacket.Info.AreaCode = -1;
	InfoPacket.Info.ConnectorDirection = 0;
	strncpy(InfoPacket.Info.ProductName,
#if POPNMUSIC
			"pop'n music controller        ",
#else
			"Dreamcast Controller          ",
#endif
			sizeof(InfoPacket.Info.ProductName));
	strncpy(InfoPacket.Info.ProductLicense,
			// NOT REALLY! Don't sue me Sega!
			"Produced By or Under License From SEGA ENTERPRISES,LTD.     ",
			sizeof(InfoPacket.Info.ProductLicense));
	InfoPacket.Info.StandbyPower = 430;
	InfoPacket.Info.MaxPower = 500;

	InfoPacket.CRC = CalcCRC((uint *)&InfoPacket.Header, sizeof(InfoPacket) / sizeof(uint) - 2);
}

void BuildSubPeripheralInfoPacket()
{
	SubPeripheralInfoPacket.BitPairsMinus1 = (sizeof(SubPeripheralInfoPacket) - 7) * 4 - 1;

	SubPeripheralInfoPacket.Header.Command = CMD_RESPOND_DEVICE_INFO;
	SubPeripheralInfoPacket.Header.Recipient = ADDRESS_DREAMCAST;
	SubPeripheralInfoPacket.Header.Sender = ADDRESS_SUBPERIPHERAL;
	SubPeripheralInfoPacket.Header.NumWords = sizeof(SubPeripheralInfoPacket.Info) / sizeof(uint);

	SubPeripheralInfoPacket.Info.Func = __builtin_bswap32(FUNC_MEMORY_CARD);
	SubPeripheralInfoPacket.Info.FuncData[0] = __builtin_bswap32(0x000f4100); // What a VMU would send (not sure what it means)
	SubPeripheralInfoPacket.Info.FuncData[1] = 0;
	SubPeripheralInfoPacket.Info.FuncData[2] = 0;
	SubPeripheralInfoPacket.Info.AreaCode = -1;
	SubPeripheralInfoPacket.Info.ConnectorDirection = 0;
	strncpy(SubPeripheralInfoPacket.Info.ProductName,
			"Visual Memory                 ",
			sizeof(SubPeripheralInfoPacket.Info.ProductName));
	strncpy(SubPeripheralInfoPacket.Info.ProductLicense,
			// NOT REALLY! Don't sue me Sega!
			"Produced By or Under License From SEGA ENTERPRISES,LTD.     ",
			sizeof(SubPeripheralInfoPacket.Info.ProductLicense));
	SubPeripheralInfoPacket.Info.StandbyPower = 430;
	SubPeripheralInfoPacket.Info.MaxPower = 500;

	SubPeripheralInfoPacket.CRC = CalcCRC((uint *)&SubPeripheralInfoPacket.Header, sizeof(SubPeripheralInfoPacket) / sizeof(uint) - 2);
}

void BuildMemoryInfoPacket()
{
	MemoryInfoPacket.BitPairsMinus1 = (sizeof(MemoryInfoPacket) - 7) * 4 - 1;

	MemoryInfoPacket.Header.Command = CMD_RESPOND_DATA_TRANSFER;
	MemoryInfoPacket.Header.Recipient = ADDRESS_DREAMCAST;
	MemoryInfoPacket.Header.Sender = ADDRESS_SUBPERIPHERAL;
	MemoryInfoPacket.Header.NumWords = sizeof(MemoryInfoPacket.Info) / sizeof(uint);

	MemoryInfoPacket.Info.Func = __builtin_bswap32(FUNC_MEMORY_CARD);
	// This seems to be what emulators return but some values seem a bit weird
	// TODO: Sniff the communication with a real VMU
	MemoryInfoPacket.Info.TotalSize = CARD_BLOCKS - 1;
	MemoryInfoPacket.Info.ParitionNumber = 0;
	MemoryInfoPacket.Info.SystemArea = ROOT_BLOCK;	// Seems like this should be root block instead of "system area"
	MemoryInfoPacket.Info.FATArea = FAT_BLOCK;
	MemoryInfoPacket.Info.NumFATBlocks = NUM_FAT_BLOCKS;
	MemoryInfoPacket.Info.FileInfoArea = DIRECTORY_BLOCK;
	MemoryInfoPacket.Info.NumInfoBlocks = NUM_DIRECTORY_BLOCKS; // Grows downwards?
	MemoryInfoPacket.Info.VolumeIcon = 0;
	MemoryInfoPacket.Info.Reserved = 0;
	MemoryInfoPacket.Info.SaveArea = SAVE_BLOCK;
	MemoryInfoPacket.Info.NumSaveBlocks = NUM_SAVE_BLOCKS; // Grows downwards? Shouldn't it to be 200?
	MemoryInfoPacket.Info.Reserved32 = 0;

	MemoryInfoPacket.CRC = CalcCRC((uint *)&MemoryInfoPacket.Header, sizeof(MemoryInfoPacket) / sizeof(uint) - 2);
}

void BuildControllerPacket()
{
	ControllerPacket.BitPairsMinus1 = (sizeof(ControllerPacket) - 7) * 4 - 1;

	ControllerPacket.Header.Command = CMD_RESPOND_DATA_TRANSFER;
	ControllerPacket.Header.Recipient = ADDRESS_DREAMCAST;
	ControllerPacket.Header.Sender = ADDRESS_CONTROLLER_AND_SUBS;
	ControllerPacket.Header.NumWords = sizeof(ControllerPacket.Controller) / sizeof(uint);

	ControllerPacket.Controller.Condition = __builtin_bswap32(FUNC_CONTROLLER);
	ControllerPacket.Controller.Buttons = 0;
	ControllerPacket.Controller.LeftTrigger = 0;
	ControllerPacket.Controller.RightTrigger = 0;
	ControllerPacket.Controller.JoyX = 0x80;
	ControllerPacket.Controller.JoyY = 0x80;
	ControllerPacket.Controller.JoyX2 = 0x80;
	ControllerPacket.Controller.JoyY2 = 0x80;
	
	OriginalControllerCRC = CalcCRC((uint *)&ControllerPacket.Header, sizeof(ControllerPacket) / sizeof(uint) - 2);
}
	
void BuildBlockReadResponsePacket()
{
	DataPacket.BitPairsMinus1 = (sizeof(DataPacket) - 7) * 4 - 1;

	DataPacket.Header.Command = CMD_RESPOND_DATA_TRANSFER;
	DataPacket.Header.Recipient = ADDRESS_DREAMCAST;
	DataPacket.Header.Sender = ADDRESS_SUBPERIPHERAL;
	DataPacket.Header.NumWords = sizeof(DataPacket.BlockRead) / sizeof(uint);

	DataPacket.BlockRead.Func = __builtin_bswap32(FUNC_MEMORY_CARD);
	DataPacket.BlockRead.Address = 0;
	memset(DataPacket.BlockRead.Data, 0, sizeof(DataPacket.BlockRead.Data));
	
	OriginalReadBlockResponseCRC = CalcCRC((uint *)&DataPacket.Header, sizeof(DataPacket) / sizeof(uint) - 2);
}

int SendPacket(const uint* Words, uint NumWords)
{
	// Correct the port number. Doesn't change CRC as same on both sender and recipient
	PacketHeader *Header = (PacketHeader *)(Words + 1);
	Header->Sender = (Header->Sender & ADDRESS_PERIPHERAL_MASK) | CurrentPort;
	Header->Recipient = (Header->Recipient & ADDRESS_PERIPHERAL_MASK) | CurrentPort;

	dma_channel_set_read_addr(TXDMAChannel, Words, false);
	dma_channel_set_trans_count(TXDMAChannel, NumWords, true);
}

void SendControllerStatus()
{
	uint Buttons = 0xFFFF;

	// TODO: Possible improvement if we did while waiting for packet from core1
	// Or run it all in interrupts
	for (int i = 0; i < NUM_BUTTONS; i++)
	{
		if (!gpio_get(ButtonInfos[i].InputIO))
		{
			Buttons &= ~ButtonInfos[i].DCButtonMask;
			ButtonInfos[i].Fade = 0xFF;
		}
		ButtonInfos[i].Fade = (ButtonInfos[i].Fade > FADE_SPEED) ? ButtonInfos[i].Fade - FADE_SPEED : 0;
		pwm_set_gpio_level(ButtonInfos[i].OutputIO, ButtonInfos[i].Fade * ButtonInfos[i].Fade);
	}
	if ((Buttons & START_MASK) == 0)
	{
		bPressingStart = true;
	}
	if (bPressingStart)
	{
		if ((Buttons & START_MASK) == START_MASK)
		{
			bPressingStart = false;
		}
		else
		{
			Buttons |= START_MASK;
			Buttons &= ~START_BUTTON;
		}
	}
	
	ControllerPacket.Controller.Buttons = Buttons;
	uint CRC = Buttons;
	CRC ^= CRC << 16;
	CRC ^= CRC << 8;
	CRC ^= OriginalControllerCRC;
	ControllerPacket.CRC = CRC;

	SendPacket((uint *)&ControllerPacket, sizeof(ControllerPacket) / sizeof(uint));
}

void SendBlockReadResponsePacket()
{
	uint Partition = (SendBlockAddress >> 24) & 0xFF;
	uint Phase = (SendBlockAddress >> 16) & 0xFF;
	uint Block = SendBlockAddress & 0xFF; // Emulators also seem to ignore top bits for a read

	assert(Phase == 0);
	uint MemoryOffset = Block * BLOCK_SIZE + Phase * PHASE_SIZE;

	DataPacket.BlockRead.Address = SendBlockAddress;
	memcpy(DataPacket.BlockRead.Data, &MemoryCard[MemoryOffset], sizeof(DataPacket.BlockRead.Data));
	uint CRC = CalcCRC(&DataPacket.BlockRead.Address, 1 + (sizeof(DataPacket.BlockRead.Data) / sizeof(uint)));
	DataPacket.CRC = CRC ^ OriginalReadBlockResponseCRC;
	
	SendBlockAddress = ~0u;
	
	SendPacket((uint *)&DataPacket, sizeof(DataPacket) / sizeof(uint));
}

void BlockRead(uint Address)
{
#if SHOULD_PRINT
	printf("Read %08x\n", Address);
#endif
	assert(SendBlockAddress == ~0u); // No send pending
	SendBlockAddress = Address;
	NextPacketSend = SEND_DATA;
}

void BlockWrite(uint Address, uint* Data, uint NumWords)
{
	uint Partition = (Address >> 24) & 0xFF;
	uint Phase = (Address >> 16) & 0xFF;
	uint Block = Address & 0xFFFF;
	
	assert(NumWords * sizeof(uint) == PHASE_SIZE);
#if SHOULD_PRINT
	printf("Write %08x %d\n", Address, NumWords);
#endif

	uint MemoryOffset = Block * BLOCK_SIZE + Phase * PHASE_SIZE;
	memcpy(&MemoryCard[MemoryOffset], Data, PHASE_SIZE);
	SectorDirty |= 1u << (MemoryOffset / FLASH_SECTOR_SIZE);
	MessagesSinceWrite = 0;

	NextPacketSend = SEND_ACK;
}

void BlockCompleteWrite(uint Address)
{
	uint Partition = (Address >> 24) & 0xFF;
	uint Phase = (Address >> 16) & 0xFF;
	uint Block = Address & 0xFFFF;

	assert(Phase == 4);
	
	uint MemoryOffset = Block * BLOCK_SIZE;
	SectorDirty |= 1u << (MemoryOffset / FLASH_SECTOR_SIZE);
	MessagesSinceWrite = 0;

	NextPacketSend = SEND_ACK;
}

bool ConsumePacket(uint Size)
{
	if ((Size & 3) == 1) // Even number of words + CRC
	{
		Size--; // Drop CRC byte
		if (Size > 0)
		{
			PacketHeader *Header = (PacketHeader *)Packet;
			uint *PacketData = (uint *)(Header + 1);
			if (Size == (Header->NumWords + 1) * 4)
			{
				// Set the port number then mask it off
				if ((Header->Sender & ADDRESS_PERIPHERAL_MASK) == ADDRESS_DREAMCAST)
				{
					CurrentPort = Header->Sender & ADDRESS_PORT_MASK;
				}
				Header->Recipient &= ADDRESS_PERIPHERAL_MASK;
				Header->Sender &= ADDRESS_PERIPHERAL_MASK;

				// If it's for us or we've sent something and want to check it
				if (Header->Recipient == ADDRESS_CONTROLLER || (Header->Recipient == ADDRESS_DREAMCAST && Header->Sender == ADDRESS_CONTROLLER_AND_SUBS))
				{
					switch (Header->Command)
					{
					case CMD_REQUEST_DEVICE_INFO:
					{
						NextPacketSend = SEND_CONTROLLER_INFO;
						return true;
					}
					case CMD_GET_CONDITION:
					{
						if (Header->NumWords >= 1 && *PacketData == __builtin_bswap32(FUNC_CONTROLLER))
						{
							NextPacketSend = SEND_CONTROLLER_STATUS;
							return true;
						}
						break;
					}
					case CMD_RESPOND_DEVICE_INFO:
					{
						PacketDeviceInfo *DeviceInfo = (PacketDeviceInfo *)(Header + 1);
						if (Header->NumWords * sizeof(uint) == sizeof(PacketDeviceInfo))
						{
							SWAP4(DeviceInfo->Func);
							SWAP4(DeviceInfo->FuncData[0]);
							SWAP4(DeviceInfo->FuncData[1]);
							SWAP4(DeviceInfo->FuncData[2]);
#if SHOULD_PRINT
							printf("Info:\nFunc: %08x (%08x %08x %08x) Area: %d Dir: %d Name: %.*s License: %.*s Pwr: %d->%d\n",
								   DeviceInfo->Func, DeviceInfo->FuncData[0], DeviceInfo->FuncData[1], DeviceInfo->FuncData[2],
								   DeviceInfo->AreaCode, DeviceInfo->ConnectorDirection, 30, DeviceInfo->ProductName, 60, DeviceInfo->ProductLicense,
								   DeviceInfo->StandbyPower, DeviceInfo->MaxPower);
#endif
							return true;
						}
						break;
					}
					case CMD_RESPOND_DATA_TRANSFER:
					{
						PacketControllerCondition *ControllerCondition = (PacketControllerCondition *)(Header + 1);
						if (Header->NumWords * sizeof(uint) == sizeof(PacketControllerCondition))
						{
							SWAP4(ControllerCondition->Condition);
							if (ControllerCondition->Condition == FUNC_CONTROLLER)
							{
#if SHOULD_PRINT
								printf("Buttons: %02x LT:%d RT:%d Joy: %d %d Joy2: %d %d\n",
									   ControllerCondition->Buttons, ControllerCondition->LeftTrigger, ControllerCondition->RightTrigger,
									   ControllerCondition->JoyX - 0x80, ControllerCondition->JoyY - 0x80,
									   ControllerCondition->JoyX2 - 0x80, ControllerCondition->JoyY2 - 0x80);
#endif
								return true;
							}
						}
						break;
					}
					}
				}
				else if (Header->Recipient == ADDRESS_SUBPERIPHERAL || (Header->Recipient == ADDRESS_DREAMCAST && Header->Sender == ADDRESS_SUBPERIPHERAL))
				{
					switch (Header->Command)
					{
					case CMD_REQUEST_DEVICE_INFO:
					{
						NextPacketSend = SEND_MEMORY_CARD_INFO;
						return true;
					}
					case CMD_RESPOND_DEVICE_INFO:
					{
						PacketDeviceInfo *DeviceInfo = (PacketDeviceInfo *)(Header + 1);
						if (Header->NumWords * sizeof(uint) == sizeof(PacketDeviceInfo))
						{
							SWAP4(DeviceInfo->Func);
							SWAP4(DeviceInfo->FuncData[0]);
							SWAP4(DeviceInfo->FuncData[1]);
							SWAP4(DeviceInfo->FuncData[2]);
#if SHOULD_PRINT
							printf("Info:\nFunc: %08x (%08x %08x %08x) Area: %d Dir: %d Name: %.*s License: %.*s Pwr: %d->%d\n",
								   DeviceInfo->Func, DeviceInfo->FuncData[0], DeviceInfo->FuncData[1], DeviceInfo->FuncData[2],
								   DeviceInfo->AreaCode, DeviceInfo->ConnectorDirection, 30, DeviceInfo->ProductName, 60, DeviceInfo->ProductLicense,
								   DeviceInfo->StandbyPower, DeviceInfo->MaxPower);
#endif
							return true;
						}
						break;
					}
					case CMD_GET_MEMORY_INFO:
					{
						if (Header->NumWords >= 2 && *PacketData == __builtin_bswap32(FUNC_MEMORY_CARD) && *(PacketData + 1) == 0)
						{
							NextPacketSend = SEND_MEMORY_INFO;
							return true;
						}
						break;
					}
					case CMD_BLOCK_READ:
					{
						if (Header->NumWords >= 2)
						{
							assert(*PacketData == __builtin_bswap32(FUNC_MEMORY_CARD));
							BlockRead(__builtin_bswap32(*(PacketData + 1)));
							return true;
						}
						break;
					}
					case CMD_BLOCK_WRITE:
					{
						if (Header->NumWords >= 2)
						{
							assert(*PacketData == __builtin_bswap32(FUNC_MEMORY_CARD));
							BlockWrite(__builtin_bswap32(*(PacketData + 1)), PacketData + 2, Header->NumWords - 2);
							return true;
						}
						break;
					}
					case CMD_BLOCK_COMPLETE_WRITE:
					{
						if (Header->NumWords >= 2)
						{
							assert(*PacketData == __builtin_bswap32(FUNC_MEMORY_CARD));
							BlockCompleteWrite(__builtin_bswap32(*(PacketData + 1)));
							return true;
						}
						break;
					}
					case CMD_RESPOND_DATA_TRANSFER:
					{
						return true;
						break;
					}
					case CMD_RESPOND_COMMAND_ACK:
					{
						if (Header->NumWords == 0)
							return true;
						break;
					}
					}
				}
			}
		}
	}
	return false;
}

// *IMPORTANT* This function must be in RAM. Will be too slow if have to fetch code from flash
static void __not_in_flash_func(core1_entry)(void)
{
	uint State = 0;
	uint8_t Byte = 0;
	uint8_t XOR = 0;
	uint StartOfPacket = 0;
	uint Offset = 0;

	BuildStateMachineTables();

	multicore_fifo_push_blocking(0); // Tell core0 we're ready
	multicore_fifo_pop_blocking(); // Wait for core0 to acknowledge and start RXPIO

	// Make sure we are ready to go	by flushing the FIFO 
	while ((RXPIO->fstat & (1u << (PIO_FSTAT_RXEMPTY_LSB))) == 0)
	{
		pio_sm_get(RXPIO, 0);
	}

	while (true)
	{
		// Worst case we could have only 0.5us (~65 cycles) to process each byte if want to keep up real time
		// In practice we have around 4us on average so this code is easily fast enough
		while ((RXPIO->fstat & (1u << (PIO_FSTAT_RXEMPTY_LSB))) != 0);
		const uint8_t Value = RXPIO->rxf[0];
		StateMachine M = Machine[State][Value];
		State = M.NewState;
		if (M.Reset)
		{
			Offset = StartOfPacket;
			Byte = 0;
			XOR = 0;
		}
		Byte |= SetBits[M.SetBitsIndex][0];
		if (M.Push)
		{
			RecieveBuffer[Offset & (sizeof(RecieveBuffer) - 1)] = Byte;
			XOR ^= Byte;
			Byte = SetBits[M.SetBitsIndex][1];
			Offset++;
		}
		if (M.End)
		{
			if (XOR == 0)
			{
				if (multicore_fifo_wready())
				{
					//multicore_fifo_push_blocking(Offset);  //Don't call as needs all be in RAM. Inlined below
					sio_hw->fifo_wr = Offset;
					__sev();
					StartOfPacket = ((Offset + 3) & ~3); // Align up for easier swizzling
				}
				else
				{
#if !SHOULD_PRINT // Core can be too slow due to printing
					panic("Packet processing core isn't fast enough :(\n");
#endif
				}
			}
		}
		if ((RXPIO->fstat & (1u << (PIO_FSTAT_RXFULL_LSB))) != 0)
		{
			// Should be a panic but the inlining of multicore_fifo_push_blocking caused it to fire
			// Weirdly after changing this to a printf it never gets called :/
			// Something to work out...
			printf("Probably overflowed. This code isn't fast enough :(\n");
		}
	}
}

void SetupButtons()
{
	pwm_config PWMConfig = pwm_get_default_config();
	pwm_config_set_clkdiv(&PWMConfig, 4.0f);
	for (int i = 0; i < NUM_BUTTONS; i++)
	{
		gpio_init(ButtonInfos[i].InputIO);
		gpio_set_dir(ButtonInfos[i].InputIO, false);
		gpio_pull_up(ButtonInfos[i].InputIO);

		gpio_set_function(ButtonInfos[i].OutputIO, GPIO_FUNC_PWM);
		pwm_init(pwm_gpio_to_slice_num(ButtonInfos[i].OutputIO), &PWMConfig, true);
	}
	for (int i = 0; i < NUM_BUTTONS; i++)
	{
		// Light up buttons dimly so can tell we are powered
		pwm_set_gpio_level(ButtonInfos[i].OutputIO, 64*64);
	}
}

void SetupMapleTX()
{
    uint TXStateMachine = pio_claim_unused_sm(TXPIO, true);
    uint TXPIOOffset = pio_add_program(TXPIO, &maple_tx_program);
    maple_tx_program_init(TXPIO, TXStateMachine, TXPIOOffset, PICO_PIN1_PIN, PICO_PIN5_PIN, 3.0f);

	TXDMAChannel = dma_claim_unused_channel(true);
	dma_channel_config TXDMAConfig = dma_channel_get_default_config(TXDMAChannel);
    channel_config_set_read_increment(&TXDMAConfig, true);
    channel_config_set_write_increment(&TXDMAConfig, false);
	channel_config_set_transfer_data_size(&TXDMAConfig, DMA_SIZE_32);
    channel_config_set_dreq(&TXDMAConfig, pio_get_dreq(TXPIO, TXStateMachine, true));
    dma_channel_configure(TXDMAChannel, &TXDMAConfig,
        &TXPIO->txf[TXStateMachine],	// Destinatinon pointer
        NULL,  							// Source pointer (will set when want to send)
        0,								// Number of transfers (will set when want to send)
        false              				// Don't start yet
    );

	gpio_pull_up(PICO_PIN1_PIN);
	gpio_pull_up(PICO_PIN5_PIN);
}

void SetupMapleRX()
{
	uint RXPIOOffsets[3] = {
		pio_add_program(RXPIO, &maple_rx_triple1_program),
		pio_add_program(RXPIO, &maple_rx_triple2_program),
		pio_add_program(RXPIO, &maple_rx_triple3_program)
		};
	maple_rx_triple_program_init(RXPIO, RXPIOOffsets, PICO_PIN1_PIN_RX, PICO_PIN5_PIN_RX, 3.0f);

	// Make sure core1 is ready to say we are ready
	multicore_fifo_pop_blocking();
	multicore_fifo_push_blocking(0);

	pio_sm_set_enabled(RXPIO, 1, true);
	pio_sm_set_enabled(RXPIO, 2, true);
	pio_sm_set_enabled(RXPIO, 0, true);
}

void ReadFlash()
{
	memcpy(MemoryCard, (uint8_t *)XIP_BASE + FLASH_OFFSET, sizeof(MemoryCard));
	SectorDirty = CheckFormatted(MemoryCard);
}

int main() {
	stdio_init_all();
	printf("Starting\n");

	ReadFlash();

	multicore_launch_core1(core1_entry);

	// Controller packets
	BuildInfoPacket();
	BuildControllerPacket();
	
	// Subperipheral (memory card) packets
	BuildACKPacket();
	BuildSubPeripheralInfoPacket();
	BuildMemoryInfoPacket();
	BuildBlockReadResponsePacket();

	SetupButtons();
	SetupMapleTX();
	SetupMapleRX();
	
	uint StartOfPacket = 0;
    while (true)
	{
		uint EndOfPacket = multicore_fifo_pop_blocking();

		// TODO: Improve. Would be nice not to move here
		for (uint i = StartOfPacket; i < EndOfPacket; i += 4)
		{
			*(uint*)&Packet[i - StartOfPacket] = __builtin_bswap32(*(uint*)&RecieveBuffer[i & (sizeof(RecieveBuffer) - 1)]);
		}
		
		uint PacketSize = EndOfPacket - StartOfPacket;
		if (!ConsumePacket(PacketSize))
		{
#if SHOULD_PRINT
			for (uint i = 0; i < (PacketSize & ~3); i += 4)
			{
				printf("%02x %02x %02x %02x\n", Packet[i + 0], Packet[i + 1], Packet[i + 2], Packet[i + 3]);
			}
			for (uint i = (PacketSize & ~3); i < PacketSize; i += 4)
			{
				uint Index = (i & ~3) + 3 - (i & 3);
				printf("%02x\n", Packet[Index]);
			}
#endif
		}
		StartOfPacket = ((EndOfPacket + 3) & ~3);

		if (NextPacketSend != SEND_NOTHING)
		{
#if SHOULD_SEND
			if (!dma_channel_is_busy(TXDMAChannel))
			{
				switch (NextPacketSend)
				{
				case SEND_CONTROLLER_INFO:
					SendPacket((uint *)&InfoPacket, sizeof(InfoPacket) / sizeof(uint));
					break;
				case SEND_CONTROLLER_STATUS:
					SendControllerStatus();

					// Doing flash writes on controller status as likely got a frame until next message
					// And unlikely to be in middle of doing rapid flash operations like a format or reading a large file
					// Ideally this would be asynchronous but doesn't seem possible :(
				    // We delay writes as flash reprogramming too slow to keep up with Dreamcast
					// Also has side benefit of amalgamating flash writes thus reducing wear 
					if (SectorDirty && !multicore_fifo_rvalid() && MessagesSinceWrite >= FLASH_WRITE_DELAY)
					{
						uint Sector = 31 - __builtin_clz(SectorDirty);
						SectorDirty &= ~(1 << Sector);
						uint SectorOffset = Sector * FLASH_SECTOR_SIZE;

						uint Interrupts = save_and_disable_interrupts();
						flash_range_erase(FLASH_OFFSET + SectorOffset, FLASH_SECTOR_SIZE);
						flash_range_program(FLASH_OFFSET + SectorOffset, &MemoryCard[SectorOffset], FLASH_SECTOR_SIZE);
						restore_interrupts(Interrupts);
					}
					else if (MessagesSinceWrite < FLASH_WRITE_DELAY)
					{
						MessagesSinceWrite++;
					}
					break;
				case SEND_MEMORY_CARD_INFO:
					SendPacket((uint *)&SubPeripheralInfoPacket, sizeof(SubPeripheralInfoPacket) / sizeof(uint));
					break;
				case SEND_MEMORY_INFO:
					SendPacket((uint *)&MemoryInfoPacket, sizeof(MemoryInfoPacket) / sizeof(uint));
					break;
				case SEND_ACK:
					SendPacket((uint *)&ACKPacket, sizeof(ACKPacket) / sizeof(uint));
					break;
				case SEND_DATA:
					SendBlockReadResponsePacket();
					break;
				}
			}
#endif
			NextPacketSend = SEND_NOTHING;
		}
	}
}
