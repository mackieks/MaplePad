/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
// Our assembled program:
#include "maple.pio.h"

#define SHOULD_SEND 1

#define SIZE_SHIFT 11
#define SIZE (1<<SIZE_SHIFT)
uint8_t capture_buf[SIZE] __attribute__ ((aligned(SIZE)));

uint32_t send_buf[256+3];

#define PICO_PIN1_PIN	14
#define PICO_PIN5_PIN	15

// Can't be same as not on same PIO?
#define PICO_PIN1_PIN_RX	PICO_PIN1_PIN
#define PICO_PIN5_PIN_RX	PICO_PIN5_PIN

#define SWAP4(x) do { x=__builtin_bswap32(x); } while(0)

#define ADDRESS_DREAMCAST 0
#define ADDRESS_CONTROLLER 0x20

PIO txpio;
uint txsm;
uint txdma_chan;

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
	CMD_SET_CONDITION
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
	uint Func;			// BE
	uint FuncData[3];	// BE
	int8_t AreaCode;
	uint8_t ConnectorDirection;
	char ProductName[30];
	char ProductLicense[60];
	uint16_t StandbyPower;
	uint16_t MaxPower;
} PacketDeviceInfo;

typedef struct PacketControllerCondition_s
{
	uint Condition;
	uint16_t Buttons;
	uint8_t RightTrigger;
	uint8_t LeftTrigger;
	uint8_t JoyX;
	uint8_t JoyY;
	uint8_t JoyX2;
	uint8_t JoyY2;
} PacketControllerCondition;

int sendPacket(const uint* Words, uint NumWords)
{
	dma_channel_set_read_addr(txdma_chan, Words, false);
	dma_channel_set_trans_count(txdma_chan, NumWords, true);
}

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

void SendInfoPacket()
{
	dma_channel_wait_for_finish_blocking(txdma_chan);

	struct FPacket
	{
		uint BitPairsMinus1;
		PacketHeader Header;
		PacketDeviceInfo Info;
		uint CRC;
	}* InfoPacket = (struct FPacket*)send_buf;

	InfoPacket->BitPairsMinus1 = (sizeof(*InfoPacket) - 7) * 4 - 1;

	InfoPacket->Header.Command = CMD_RESPOND_DEVICE_INFO;
	InfoPacket->Header.Recipient = ADDRESS_DREAMCAST;
	InfoPacket->Header.Sender = ADDRESS_CONTROLLER;
	InfoPacket->Header.NumWords = sizeof(InfoPacket->Info) / sizeof(uint);

	InfoPacket->Info.Func = __builtin_bswap32(1);
	InfoPacket->Info.FuncData[0] = __builtin_bswap32(0x000f06fe);
	InfoPacket->Info.FuncData[1] = 0;
	InfoPacket->Info.FuncData[2] = 0;
	InfoPacket->Info.AreaCode = -1;
	InfoPacket->Info.ConnectorDirection = 0;
	strncpy(InfoPacket->Info.ProductName,
			"Dreamcast Controller          ",
			sizeof(InfoPacket->Info.ProductName));
	strncpy(InfoPacket->Info.ProductLicense,
			"Produced By or Under License From SEGA ENTERPRISES,LTD.     ",
			sizeof(InfoPacket->Info.ProductLicense));
	InfoPacket->Info.StandbyPower = 430;
	InfoPacket->Info.MaxPower = 500;

	InfoPacket->CRC = CalcCRC((uint *)&InfoPacket->Header, sizeof(*InfoPacket) / sizeof(uint) - 2);

	sendPacket((uint*)InfoPacket, sizeof(*InfoPacket) / sizeof(uint));
}

void SendControllerPacket(uint Buttons)
{
	dma_channel_wait_for_finish_blocking(txdma_chan);

	struct FPacket
	{
		uint BitPairsMinus1;
		PacketHeader Header;
		PacketControllerCondition Controller;
		uint CRC;
	}* ControllerPacket = (struct FPacket*)send_buf;
	
	ControllerPacket->BitPairsMinus1 = (sizeof(*ControllerPacket) - 7) * 4 - 1;

	ControllerPacket->Header.Command = CMD_RESPOND_DATA_TRANSFER;
	ControllerPacket->Header.Recipient = ADDRESS_DREAMCAST;
	ControllerPacket->Header.Sender = ADDRESS_CONTROLLER;
	ControllerPacket->Header.NumWords = sizeof(ControllerPacket->Controller) / sizeof(uint);

	ControllerPacket->Controller.Condition = __builtin_bswap32(1);
	ControllerPacket->Controller.Buttons = Buttons;
	ControllerPacket->Controller.LeftTrigger = 0;
	ControllerPacket->Controller.RightTrigger = 0;
	ControllerPacket->Controller.JoyX = 0x80;
	ControllerPacket->Controller.JoyY = 0x80;
	ControllerPacket->Controller.JoyX2 = 0x80;
	ControllerPacket->Controller.JoyY2 = 0x80;
	
	ControllerPacket->CRC = CalcCRC((uint *)&ControllerPacket->Header, sizeof(*ControllerPacket) / sizeof(uint) - 2);

	sendPacket((uint *)ControllerPacket, sizeof(*ControllerPacket) / sizeof(uint));
}

uint8_t Packet[1024 + 8]; // Maximum possible size
uint PacketByte=0;

bool ConsumePacket(uint Size)
{
	if ((Size & 3) == 1) // Even number of words + CRC
	{
		Size--; // Drop CRC byte
		if (Size > 0)
		{
			PacketHeader *Header = (PacketHeader *)Packet;
			if (Size == (Header->NumWords + 1) * 4)
			{
				switch (Header->Command)
				{
				case CMD_REQUEST_DEVICE_INFO:
				{
#if SHOULD_SEND
					SendInfoPacket();
#endif
					return true;
					break;
				}
				case CMD_GET_CONDITION:
				{
#if SHOULD_SEND
					static uint ButtonCounter = 0;
					ButtonCounter++;
					SendControllerPacket((ButtonCounter & 0x100) ? 0xFF7F : 0xFFBF);
#endif
					return true;
					break;
				}
				case CMD_RESPOND_DEVICE_INFO:
				{
					PacketDeviceInfo *DeviceInfo = (PacketDeviceInfo *)(Header + 1);
					if (Header->NumWords * 4 == sizeof(PacketDeviceInfo))
					{
						SWAP4(DeviceInfo->Func);
						SWAP4(DeviceInfo->FuncData[0]);
						SWAP4(DeviceInfo->FuncData[1]);
						SWAP4(DeviceInfo->FuncData[2]);
						printf("Info:\nFunc: %08x (%08x %08x %08x) Area: %d Dir: %d Name: %.*s License: %.*s Pwr: %d->%d\n",
							   DeviceInfo->Func, DeviceInfo->FuncData[0], DeviceInfo->FuncData[1], DeviceInfo->FuncData[2],
							   DeviceInfo->AreaCode, DeviceInfo->ConnectorDirection, 30, DeviceInfo->ProductName, 60, DeviceInfo->ProductLicense,
							   DeviceInfo->StandbyPower, DeviceInfo->MaxPower);
						return true;
					}
					break;
				}
				case CMD_RESPOND_DATA_TRANSFER:
				{
					PacketControllerCondition *ControllerCondition = (PacketControllerCondition *)(Header + 1);
					if (Header->NumWords * 4 == sizeof(PacketControllerCondition))
					{
						SWAP4(ControllerCondition->Condition);
						if (ControllerCondition->Condition == 1)
						{
							if (ControllerCondition->Buttons != 0xFFFF)
							{
								printf("Buttons: %02x LT:%d RT:%d Joy: %d %d Joy2: %d %d\n",
									   ControllerCondition->Buttons, ControllerCondition->LeftTrigger, ControllerCondition->RightTrigger,
									   ControllerCondition->JoyX - 0x80, ControllerCondition->JoyY - 0x80,
									   ControllerCondition->JoyX2 - 0x80, ControllerCondition->JoyY2 - 0x80);
							}
							return true;
						}
					}
					break;
				}
				}
			}
		}
	}
	return false;
}

uint8_t CurrentByte = 0;
uint8_t LeftToShift = 8;
uint8_t XOR = 0;

void grabBit(uint8_t Bit, uint8_t Phase)
{
	static uint8_t LastPhase = 0;
	if (Phase != LastPhase)
	{
		CurrentByte<<=1;
		CurrentByte|=(Bit&1);
		LeftToShift--;
		if (LeftToShift == 0)
		{
			if (PacketByte < sizeof(Packet))
			{
				uint Index = PacketByte++;
				Index = (Index&~3) + 3 - (Index&3);
				Packet[Index] = CurrentByte;
			}
			XOR ^= CurrentByte;
			LeftToShift=8;
		}
		LastPhase = Phase;
	}
	else
	{
		if (PacketByte > 0 && (XOR != 0 || !ConsumePacket(PacketByte)))
		{
			printf("Bad packet? XOR:%x\n", XOR);
			for (uint i = 0; i < (PacketByte & ~3); i += 4)
			{
				printf("%02x %02x %02x %02x\n",
					   Packet[i + 0], Packet[i + 1], Packet[i + 2], Packet[i + 3]);
			}
			for (uint i = (PacketByte & ~3); i < PacketByte; i += 4)
			{
				uint Index = (i & ~3) + 3 - (i & 3);
				printf("%02x\n", Packet[Index]);
			}
		}
		LeftToShift = 8;
		XOR = 0;
		PacketByte = 0;
	}
}

void processTransistions(uint8_t BitPair)
{
	static uint8_t LastPair = 3;
	if ((LastPair&1)!=0 && (BitPair&1)==0)
	{
		grabBit(BitPair>>1, 1);
	}
	else if ((LastPair&2)!=0 && (BitPair&2)==0)
	{
		grabBit(BitPair, 2);
	}
	LastPair = BitPair;
}

uint dma_mask = 0;
volatile uint dma_counter = 0;

void dma_handler()
{
	// Clear interrupt and restart DMA
	dma_hw->ints0 = dma_mask;
	dma_start_channel_mask(dma_mask);
	dma_counter++;
}

int main() {
	stdio_init_all();
	printf("Starting\n");

    // Choose which PIO instance to use (there are two instances)
    txpio = pio0;
    PIO rxpio = pio1;

    // Our assembled program needs to be loaded into this PIO's instruction
    // memory. This SDK function will find a location (offset) in the
    // instruction memory where there is enough space for our program. We need
    // to remember this location!
    uint txoffset = pio_add_program(txpio, &maple_tx_program);
    uint rxoffset[3] = {
		pio_add_program(rxpio, &maple_rx_triple1_program),
		pio_add_program(rxpio, &maple_rx_triple2_program),
		pio_add_program(rxpio, &maple_rx_triple3_program)
		};

    // Find a free state machine on our chosen PIO (erroring if there are
    // none). Configure it to run our program, and start it, using the
    // helper function we included in our .pio file.
    txsm = 0;//pio_claim_unused_sm(pio, true);
    maple_tx_program_init(txpio, txsm, txoffset, PICO_PIN1_PIN, PICO_PIN5_PIN, 3.0f);

	uint rxsm = 0;
    maple_rx_triple_program_init(rxpio, rxoffset, PICO_PIN1_PIN_RX, PICO_PIN5_PIN_RX, 1.0f);

	uint dma_chan = dma_claim_unused_channel(true);
	dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
	channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_dreq(&c, pio_get_dreq(rxpio, rxsm, false));
	channel_config_set_ring(&c, true, SIZE_SHIFT); // Loop around

    dma_channel_configure(dma_chan, &c,
        capture_buf,        // Destinatinon pointer
        &rxpio->rxf[rxsm],  // Source pointer
        sizeof(capture_buf),// Number of transfers
        false               // Delay start
    );

	dma_mask = (1u << dma_chan);
	dma_channel_set_irq0_enabled(dma_chan, true);
	irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
	irq_set_enabled(DMA_IRQ_0, true);
	dma_start_channel_mask(dma_mask);

	txdma_chan = dma_claim_unused_channel(true);
	dma_channel_config txc = dma_channel_get_default_config(txdma_chan);
    channel_config_set_read_increment(&txc, true);
    channel_config_set_write_increment(&txc, false);
	channel_config_set_transfer_data_size(&txc, DMA_SIZE_32);
    channel_config_set_dreq(&txc, pio_get_dreq(txpio, txsm, true));
    dma_channel_configure(txdma_chan, &txc,
        &txpio->txf[txsm],	// Destinatinon pointer
        NULL,  				// Source pointer
        0,					// Number of transfers
        false               // Delay start
    );
	
	printf("Starting RX PIO\n");

	pio_sm_set_enabled(rxpio, rxsm+1, true);
	pio_sm_set_enabled(rxpio, rxsm+2, true);
	pio_sm_set_enabled(rxpio, rxsm, true);

	gpio_pull_up(PICO_PIN1_PIN);
	gpio_pull_up(PICO_PIN5_PIN);

	uint ReadLoopAddress = 0;
	uint LastLoopAddress = 0;
	uint8_t* LastData = capture_buf;
    while (true)
	{
		// Detect underflow (debug only)
		uint DMACounter = 0;
		uint Address = 0;
		do
		{
			DMACounter = dma_counter;
			Address = dma_channel_hw_addr(dma_chan)->write_addr;
			if ((uint8_t*)Address < capture_buf || (uint8_t*)Address >= capture_buf + sizeof(capture_buf))
			{
				// What?! Not sure how this can happen. Hardware bug?
				printf("Bad write ptr: %08x != %08x->%08x (%d)\n", Address, capture_buf, capture_buf + sizeof(capture_buf), DMACounter);
				printf("Now: %08x (%d)\n", dma_channel_hw_addr(dma_chan)->write_addr, dma_counter);
				continue;
			}
		}
		while (DMACounter != dma_counter); // Solve issue when restart happens while reading address
		uint WriteLoopAddress = (DMACounter<<SIZE_SHIFT) + (Address&(sizeof(capture_buf)-1));
		if (WriteLoopAddress >= LastLoopAddress + sizeof(capture_buf))
		{
			panic("Underflow (could have been bad data) %08x %08x %08x\n", LastLoopAddress, ReadLoopAddress, WriteLoopAddress);
		}
		LastLoopAddress = ReadLoopAddress;

		uint8_t* NewData = (uint8_t*)Address;
		while (LastData != NewData)
		{
			processTransistions(LastData[0]>>6);
			processTransistions(LastData[0]>>4);
			processTransistions(LastData[0]>>2);
			processTransistions(LastData[0]>>0);
			LastData++;
			ReadLoopAddress++;
			if (LastData >= capture_buf + sizeof(capture_buf))
			{
				LastData = capture_buf;
			}
		}
/*
		if (!dma_channel_is_busy(txdma_chan))
		{
			static int i=0;
			i++;
			if (i&1)
			{
				SendInfoPacket();
			}
			else
			{
				SendControllerPacket(0xCDC);
			}
		}
		*/
	}
}
