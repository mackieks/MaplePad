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
#include "hardware/pwm.h"
#include "pico/multicore.h"
// Our assembled program:
#include "maple.pio.h"

#define SHOULD_SEND 1
#define POPNMUSIC 1

#define SHOULD_PRINT 1

#define USE_FAST_PARSER 1

#define SIZE_SHIFT 12
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

typedef enum ESendState_e
{
	SEND_NOTHING,
	SEND_CONTROLLER_INFO,
	SEND_CONTROLLER_STATUS
} ESendState;

ESendState SendState = SEND_NOTHING;

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

typedef struct ButtonInfo_s
{
	int InputIO;
	int OutputIO;
	int DCButtonMask;
	int Fade;
} ButtonInfo;

#define NUM_BUTTONS	9
#define FADE_SPEED 8
#define START_BUTTON 0x0008
#define START_MASK 0x0251

bool bPressingStart = false;

// A - White left   = 0xFFBF = 0x0040
// B - Yellow left  = 0xFFEF = 0x0010
// C - Green left   = 0xFFDF = 0x0020
// D - Blue left    = 0xFF7F = 0x0080
// E - Red centre   = 0xFFFB = 0x0004
// F - Blue right   = 0xFBFF = 0x0400
// G - Green right  = 0xFFFD = 0x0002
// H - Yellow right = 0xFDFF = 0x0200
// I - White right  = 0xFFFE = 0x0001
// Start            = 0xFFF7 = 0x0008

ButtonInfo ButtonInfos[NUM_BUTTONS]=
{
	{ 16, 13, 0x0040, 0 },
	{ 17, 12, 0x0010, 0 },
	{ 18, 11, 0x0020, 0 },
	{ 19, 10, 0x0080, 0 },
	{ 20, 9, 0x0004, 0 },
	{ 21, 8, 0x0400, 0 },
	{ 22, 7, 0x0002, 0 },
	{ 26, 6, 0x0200, 0 },
	{ 27, 5, 0x0001, 0 }
};

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
#if POPNMUSIC
	InfoPacket->Info.FuncData[0] = __builtin_bswap32(0x000006ff);
#else
	InfoPacket->Info.FuncData[0] = __builtin_bswap32(0x000f06fe);
#endif
	InfoPacket->Info.FuncData[1] = 0;
	InfoPacket->Info.FuncData[2] = 0;
	InfoPacket->Info.AreaCode = -1;
	InfoPacket->Info.ConnectorDirection = 0;
	strncpy(InfoPacket->Info.ProductName,
#if POPNMUSIC
			"pop'n music controller        ",
#else
			"Dreamcast Controller          ",
#endif
			sizeof(InfoPacket->Info.ProductName));
	strncpy(InfoPacket->Info.ProductLicense,
			"Produced By or Under License From SEGA ENTERPRISES,LTD.     ",
			sizeof(InfoPacket->Info.ProductLicense));
	InfoPacket->Info.StandbyPower = 430;
	InfoPacket->Info.MaxPower = 500;

	InfoPacket->CRC = CalcCRC((uint *)&InfoPacket->Header, sizeof(*InfoPacket) / sizeof(uint) - 2);

	sendPacket((uint*)InfoPacket, sizeof(*InfoPacket) / sizeof(uint));
}

void SendControllerPacket()
{
	uint Buttons = 0xFFFF;
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
			bPressingStart=false;
		}
		else
		{
			Buttons |= START_MASK;
			Buttons &= ~START_BUTTON;
		}
	}

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
					SendState = SEND_CONTROLLER_INFO;
					return true;
				}
				case CMD_GET_CONDITION:
				{
					SendState = SEND_CONTROLLER_STATUS;
					return true;
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
					if (Header->NumWords * 4 == sizeof(PacketControllerCondition))
					{
						SWAP4(ControllerCondition->Condition);
						if (ControllerCondition->Condition == 1)
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
		}
	}
	return false;
}

#if !USE_FAST_PARSER
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
#if SHOULD_PRINT
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
#endif
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
#endif

#if USE_FAST_PARSER
typedef struct SimpleState_s
{
	int Next[4];
	uint Status;
} SimpleState;

#define NUM_DATAS 64
#define NUM_STATES 40
SimpleState States[NUM_STATES];
int NumStates = 0;

int NewState(int Expected)
{
	int New = NumStates++;
	SimpleState* s = &States[New];
	memset(s, -1, sizeof(*s));
	s->Next[Expected] = New;
	return New;
}

int ExpectState(int Prev, int Expected)
{
	int New = NewState(Expected);
	States[Prev].Next[Expected] = New;
	return New;
}

int ExpectStateWithStatus(int Prev, int Expected, uint Status)
{
	int New = NewState(Expected);
	States[New].Status = Status;
	States[Prev].Next[Expected] = New;
	return New;
}

int ExpectState2(int Prev, int OtherPrev, int Expected)
{
	int New = NewState(Expected);
	States[Prev].Next[Expected] = New;
	States[OtherPrev].Next[Expected] = New;
	return New;
}

enum
{
	STATUS_NONE = -1,
	STATUS_START,
	STATUS_END,
	STATUS_PUSH0,
	STATUS_PUSH1,
	STATUS_PUSH2,
	STATUS_PUSH3,
	STATUS_PUSH4,
	STATUS_PUSH5,
	STATUS_PUSH6,
	STATUS_PUSH7,
	STATUS_BITSET = 128
};

void BuildBasicStates()
{
	// Start (11)
	int Prev = NewState(0b11);
	Prev = ExpectState(Prev, 0b10);
	Prev = ExpectState(Prev, 0b00);
	Prev = ExpectState(Prev, 0b10);
	Prev = ExpectState(Prev, 0b00);
	Prev = ExpectState(Prev, 0b10);
	Prev = ExpectState(Prev, 0b00);
	Prev = ExpectState(Prev, 0b10);
	Prev = ExpectState(Prev, 0b00);
	Prev = ExpectState(Prev, 0b10);
	Prev = ExpectStateWithStatus(Prev, 0b11, STATUS_START);

	// Byte (6*4 = 24)
	int PossibleEnd;
	int Option = Prev;
	for (int i = 0; i < 4; i++)
	{
		Prev = ExpectState2(Option, Prev, 0b01);
		Option = ExpectStateWithStatus(Prev, 0b11, (STATUS_PUSH0 + i * 2) | STATUS_BITSET);
		Prev = ExpectStateWithStatus(Prev, 0b00, (STATUS_PUSH0 + i * 2));
		PossibleEnd = Option;

		Prev = ExpectState2(Option, Prev, 0b10);
		Option = ExpectStateWithStatus(Prev, 0b11, (STATUS_PUSH1 + i * 2) | STATUS_BITSET);
		Prev = ExpectStateWithStatus(Prev, 0b00, (STATUS_PUSH1 + i * 2));
	}

	// End (5)
	Prev = ExpectState(PossibleEnd, 0b01);
	Prev = ExpectStateWithStatus(PossibleEnd, 0b00, STATUS_END); // Needs to be at least 4 transistions back (as only byte is pushed)
	Prev = ExpectState(PossibleEnd, 0b01);
	Prev = ExpectState(PossibleEnd, 0b00);
	Prev = ExpectState(PossibleEnd, 0b01);
	States[Prev].Next[0b11] = 0;
	assert(NumStates == NUM_STATES);
}

typedef struct StateMachine_s
{
	ushort NewState:6;
	ushort Data:6;
	ushort Push:1;
	ushort Error:1;
	ushort Reset:1;
	ushort End:1;
} StateMachine;

StateMachine Machine[NUM_STATES][256]; // 20Kb
uint8_t DataTable[NUM_DATAS][2];
int NumDatas = 0;

int AddData(uint8_t ByteA, uint8_t ByteB)
{
	for (int i=0; i<NumDatas; i++)
	{
		if (DataTable[i][0] == ByteA && DataTable[i][1]==ByteB)
		{
			return i;
		}
	}
	int NewEntry = NumDatas++;
	assert(NewEntry < NUM_DATAS);
	DataTable[NewEntry][0] = ByteA;
	DataTable[NewEntry][1] = ByteB;
	return NewEntry;
}

void BuildTable()
{
	BuildBasicStates();
	for (int S = 0; S<NUM_STATES; S++)
	{
		for (int V = 0; V<256; V++)
		{
			StateMachine M = { 0 };
			int State = S;
			int Transitions = V;
			uint8_t Data[2] = {0, 0};
			uint Byte = 0;
			for (int i = 0; i < 4; i++)
			{
				uint Status = States[State].Status;
				if (Status & STATUS_BITSET)
				{
					Data[Byte] |= (1 << ((Status & ~STATUS_BITSET) - STATUS_PUSH0));
				}
				switch (States[State].Status & ~STATUS_BITSET)
				{
					case STATUS_START:
						M.Reset = 1;
						break;
					case STATUS_END:
						M.End = 1;
						break;
					case STATUS_PUSH7:
						M.Push = 1;
						Byte = 1;
						break;
				}
				State = States[State].Next[(Transitions >> 6) & 3];
				if (State < 0)
				{
					M.Error = 1;
					State = 0;
				}
				Transitions <<= 2;
			}
			M.NewState = State;
			M.Data = AddData(Data[0], Data[1]);
			Machine[S][V] = M;
		}
	}
}

void core1_entry(void)
{
	BuildTable();

	uint State = 0;
	uint8_t Byte = 0;
	uint8_t XOR = 0;
	uint StartOfPacket = 0;
	uint Offset = 0;

	multicore_fifo_push_blocking(0); // Let's get going (gulp!)

	while (true)
	{
		// Have about 0.5us to process each byte if want to keep up real time
		// 65 clocks
		while ((pio1->fstat & (1u << (PIO_FSTAT_RXEMPTY_LSB))) != 0);
		const uint8_t Value = pio1->rxf[0];
		StateMachine M = Machine[State][Value];
		State = M.NewState;
		if (M.Reset)
		{
			Offset = StartOfPacket;
			Byte = 0;
			XOR = 0;
		}
		Byte |= DataTable[M.Data][0];
		if (M.Push)
		{
			capture_buf[Offset & (sizeof(capture_buf) - 1)] = Byte;
			XOR ^= Byte;
			Byte = DataTable[M.Data][1];
			Offset++;
		}
		if (M.End)
		{
			if (XOR == 0)
			{
				if (multicore_fifo_wready())
				{
					panic("Packet processing core isn't fast enough :(");
				}
				multicore_fifo_push_blocking(Offset);
				StartOfPacket = Offset;
			}
		}
		if ((pio1->fstat & (1u << (PIO_FSTAT_RXFULL_LSB))) != 0)
		{
			panic("Probably overflowed. This code isn't fast enough :(");
		}
	}
}
#endif

int main() {
	stdio_init_all();
	printf("Starting\n");

#if USE_FAST_PARSER
	multicore_launch_core1(core1_entry);
#endif

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
		pwm_set_gpio_level(ButtonInfos[i].OutputIO, 64*64);
	}

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
    maple_rx_triple_program_init(rxpio, rxoffset, PICO_PIN1_PIN_RX, PICO_PIN5_PIN_RX, 3.0f);

#if !USE_FAST_PARSER
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
#endif

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

	gpio_pull_up(PICO_PIN1_PIN);
	gpio_pull_up(PICO_PIN5_PIN);

#if USE_FAST_PARSER
	multicore_fifo_pop_blocking();
#endif

	pio_sm_set_enabled(rxpio, rxsm+1, true);
	pio_sm_set_enabled(rxpio, rxsm+2, true);
	pio_sm_set_enabled(rxpio, rxsm, true);

#if USE_FAST_PARSER
	uint SOP = 0;
#else
	uint ReadLoopAddress = 0;
	uint LastLoopAddress = 0;
	uint8_t* LastData = capture_buf;
#endif
    while (true)
	{
#if USE_FAST_PARSER
		uint EOP = multicore_fifo_pop_blocking();

		// TODO: Improve this
		for (uint i=SOP; i<EOP; i++)
		{
			Packet[i] = capture_buf[i & (sizeof(capture_buf) - 1)];
		}
		
		uint PacketSize = EOP - SOP;
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

		SOP = EOP;
#else
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
#if SHOULD_PRINT
				printf("Bad write ptr: %08x != %08x->%08x (%d)\n", Address, capture_buf, capture_buf + sizeof(capture_buf), DMACounter);
				printf("Now: %08x (%d)\n", dma_channel_hw_addr(dma_chan)->write_addr, dma_counter);
#endif
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

		volatile uint8_t* NewData = (uint8_t*)Address;
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
#endif

		if (SendState != SEND_NOTHING)
		{
#if SHOULD_SEND
			if (!dma_channel_is_busy(txdma_chan))
			{
				if (SendState == SEND_CONTROLLER_INFO)
				{
					SendInfoPacket();
				}
				else if (SendState == SEND_CONTROLLER_STATUS)
				{
					SendControllerPacket();
				}
			}
#endif
			SendState = SEND_NOTHING;
		}
	}
}
