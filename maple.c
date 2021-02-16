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
#include "pico/multicore.h"
#include "maple.pio.h"
#include "state_machine.h"

#define SHOULD_SEND 1		// Set to zero to sniff two devices sending signals to each other
#define SHOULD_PRINT 0		// Nice for debugging but can cause timing issues

#define POPNMUSIC 1			// Pop'n Music controller or generic controller
#define NUM_BUTTONS	9		// On a Pop'n Music controller
#define FADE_SPEED 8		// How fast the LEDs fade after press
#define START_BUTTON 0x0008	// Bitmask for the start button
#define START_MASK 0x0251	// Key combination for Start as we don't have a dedicated button

#define PICO_PIN1_PIN	14
#define PICO_PIN5_PIN	15
#define PICO_PIN1_PIN_RX	PICO_PIN1_PIN
#define PICO_PIN5_PIN_RX	PICO_PIN5_PIN

#define ADDRESS_DREAMCAST 0
#define ADDRESS_CONTROLLER 0x20

#define TXPIO pio0
#define RXPIO pio1

#define SWAP4(x) do { x=__builtin_bswap32(x); } while(0)

typedef enum ESendState_e
{
	SEND_NOTHING,
	SEND_CONTROLLER_INFO,
	SEND_CONTROLLER_STATUS
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
	uint Func;			// Nb. Big endian
	uint FuncData[3];	// Nb. Big endian
	int8_t AreaCode;
	uint8_t ConnectorDirection;
	char ProductName[30];
	char ProductLicense[60];
	uint16_t StandbyPower;
	uint16_t MaxPower;
} PacketDeviceInfo;

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

typedef struct FInfoPacket_s
{
	uint BitPairsMinus1;
	PacketHeader Header;
	PacketDeviceInfo Info;
	uint CRC;
} FInfoPacket;

typedef struct FControllerPacket_s
{
	uint BitPairsMinus1;
	PacketHeader Header;
	PacketControllerCondition Controller;
	uint CRC;
} FControllerPacket;

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

static ESendState NextPacketSend = SEND_NOTHING;
static uint OriginalControllerCRC = 0;
static uint TXDMAChannel = 0;
static bool bPressingStart = false;

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

void BuildInfoPacket()
{
	InfoPacket.BitPairsMinus1 = (sizeof(InfoPacket) - 7) * 4 - 1;

	InfoPacket.Header.Command = CMD_RESPOND_DEVICE_INFO;
	InfoPacket.Header.Recipient = ADDRESS_DREAMCAST;
	InfoPacket.Header.Sender = ADDRESS_CONTROLLER;
	InfoPacket.Header.NumWords = sizeof(InfoPacket.Info) / sizeof(uint);

	InfoPacket.Info.Func = __builtin_bswap32(1);
#if POPNMUSIC
	InfoPacket.Info.FuncData[0] = __builtin_bswap32(0x000006ff);
#else
	InfoPacket.Info.FuncData[0] = __builtin_bswap32(0x000f06fe);
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

void BuildControllerPacket()
{
	ControllerPacket.BitPairsMinus1 = (sizeof(ControllerPacket) - 7) * 4 - 1;

	ControllerPacket.Header.Command = CMD_RESPOND_DATA_TRANSFER;
	ControllerPacket.Header.Recipient = ADDRESS_DREAMCAST;
	ControllerPacket.Header.Sender = ADDRESS_CONTROLLER;
	ControllerPacket.Header.NumWords = sizeof(ControllerPacket.Controller) / sizeof(uint);

	ControllerPacket.Controller.Condition = __builtin_bswap32(1);
	ControllerPacket.Controller.Buttons = 0;
	ControllerPacket.Controller.LeftTrigger = 0;
	ControllerPacket.Controller.RightTrigger = 0;
	ControllerPacket.Controller.JoyX = 0x80;
	ControllerPacket.Controller.JoyY = 0x80;
	ControllerPacket.Controller.JoyX2 = 0x80;
	ControllerPacket.Controller.JoyY2 = 0x80;
	
	OriginalControllerCRC = CalcCRC((uint *)&ControllerPacket.Header, sizeof(ControllerPacket) / sizeof(uint) - 2);
}

int SendPacket(const uint* Words, uint NumWords)
{
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
					NextPacketSend = SEND_CONTROLLER_INFO;
					return true;
				}
				case CMD_GET_CONDITION:
				{
					NextPacketSend = SEND_CONTROLLER_STATUS;
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
					multicore_fifo_push_blocking(Offset);
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
			panic("Probably overflowed. This code isn't fast enough :(\n");
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

	uint TXDMAChannel = dma_claim_unused_channel(true);
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

int main() {
	stdio_init_all();
	printf("Starting\n");

	multicore_launch_core1(core1_entry);

	BuildInfoPacket();
	BuildControllerPacket();

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
				if (NextPacketSend == SEND_CONTROLLER_INFO)
				{
					SendPacket((uint*)&InfoPacket, sizeof(InfoPacket) / sizeof(uint));
				}
				else if (NextPacketSend == SEND_CONTROLLER_STATUS)
				{
					SendControllerStatus();
				}
			}
#endif
			NextPacketSend = SEND_NOTHING;
		}
	}
}
