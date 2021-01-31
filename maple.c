/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
// Our assembled program:
#include "maple.pio.h"

#define SIZE_SHIFT 10
#define SIZE (1<<SIZE_SHIFT)
uint8_t capture_buf[SIZE] __attribute__ ((aligned(SIZE)));;

#define PICO_PIN1_PIN	14
#define PICO_PIN5_PIN	15

// Can't be same as not on same PIO?
#define PICO_PIN1_PIN_RX	PICO_PIN1_PIN
#define PICO_PIN5_PIN_RX	PICO_PIN5_PIN

#define SWAP(x) Swap((uint8_t*)&(x), sizeof(x))

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

void Swap(uint8_t* Data, int Size)
{
	uint8_t Tmp;
	switch (Size)
	{
	case 4:
		Tmp = Data[3];
		Data[3] = Data[0];
		Data[0] = Tmp;
		Tmp = Data[1];
		Data[1] = Data[2];
		Data[2] = Tmp;
		break;
	case 2:
		Tmp = Data[1];
		Data[1] = Data[0];
		Data[0] = Tmp;
		break;
	default:
		printf("Bad swap\n");
	}
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
				case CMD_GET_CONDITION:
					return true;
					break;
				case CMD_RESPOND_DEVICE_INFO:
				{
					PacketDeviceInfo *DeviceInfo = (PacketDeviceInfo *)(Header + 1);
					if (Header->NumWords * 4 == sizeof(PacketDeviceInfo))
					{
						SWAP(DeviceInfo->Func);
						SWAP(DeviceInfo->FuncData[0]);
						SWAP(DeviceInfo->FuncData[1]);
						SWAP(DeviceInfo->FuncData[2]);
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
						SWAP(ControllerCondition->Condition);
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

#define BIT_PHASE_HISTORY	512
uint8_t BitPhase[BIT_PHASE_HISTORY];
uint BitPhaseCounter=0;
uint WatchTail = 0;

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
			/*
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
			for (uint i = BitPhaseCounter - BIT_PHASE_HISTORY; i < BitPhaseCounter; i++)
			{
				uint32_t BitPhaseValue = BitPhase[i & (BIT_PHASE_HISTORY - 1)];
				printf("B:%d %d\n", BitPhaseValue & 1, (BitPhaseValue >> 1) & 1);
			}
			WatchTail = 32;
			*/
		}
		LeftToShift = 8;
		XOR = 0;
		PacketByte = 0;
	}
}

void processTransistions(uint8_t BitPair)
{
	static uint8_t LastPair = 3;
	if (WatchTail > 0)
	{
		printf("A:%d %d\n", BitPair & 1, (BitPair >> 1) & 1);
		WatchTail--;
	}
	BitPhase[(BitPhaseCounter++)&(BIT_PHASE_HISTORY-1)] = (BitPair&3);
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

int sendPacket(PIO pio, uint sm, const uint8_t* Data, uint Length)
{
	pio_sm_put_blocking(pio, sm, Length * 4); // Length in bit pairs
	for (int i=0; i<Length; i+=4)
	{
		uint Block = Data[i];
		for (int k=1; k<4; k++)
		{
			Block <<= 8;
			if (i+k<Length)
			{
				Block |= Data[i+k];
			}
		}
		pio_sm_put_blocking(pio, sm, Block); // Set data
	}
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
    //PIO pio = pio0;
    PIO rxpio = pio1;

    // Our assembled program needs to be loaded into this PIO's instruction
    // memory. This SDK function will find a location (offset) in the
    // instruction memory where there is enough space for our program. We need
    // to remember this location!
    //uint offset = pio_add_program(pio, &maple_tx_program);
    uint rxoffset[3] = {
		pio_add_program(rxpio, &maple_rx_triple1_program),
		pio_add_program(rxpio, &maple_rx_triple2_program),
		pio_add_program(rxpio, &maple_rx_triple3_program)
		};

    // Find a free state machine on our chosen PIO (erroring if there are
    // none). Configure it to run our program, and start it, using the
    // helper function we included in our .pio file.
    //uint sm = 0;//pio_claim_unused_sm(pio, true);
    //maple_tx_program_init(pio, sm, offset, PICO_PIN1_PIN, PICO_PIN5_PIN, 1.0f);

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
	
	printf("Starting RX PIO\n");

	pio_sm_set_enabled(rxpio, rxsm+1, true);
	pio_sm_set_enabled(rxpio, rxsm+2, true);
	pio_sm_set_enabled(rxpio, rxsm, true);

	//printf("About to send first data\n");

	uint ReadLoopAddress = 0;
	uint LastLoopAddress = 0;
	uint8_t* LastData = capture_buf;
	//const uint8_t Packet[]= { 0x00, 0xFF, 0xCD, 0xFF };
    while (true) {
		//sendPacket(pio, sm, Packet, sizeof(Packet));
		//printf("Sent\n");
        //sleep_ms(50);

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
		//printf("NewData:%p\n", NewData);
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
				//printf("Looped:%p\n", LastData);
			}
		}
	}
}
