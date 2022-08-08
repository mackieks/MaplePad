#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include "Icon.c"
#include "IconMono.c"
#include "../src/format.h"

struct IconData
{
	char Comment[16];
	uint32_t MonochromeOffset;
	uint32_t ColourOffset;
	uint32_t Padding[2];
	uint8_t Monochrome[32 * 32 * 1 / 8];
	uint16_t Palette[16];
	uint8_t Colour[32 * 32 * 4 / 8];
};

IconData VMU;
uint8_t Padding[1024];

int main()
{
	memset(Padding, 0xFF, sizeof(Padding));

	strncpy(VMU.Comment, "Visual Memory   ", sizeof(VMU.Comment));
	VMU.MonochromeOffset = offsetof(IconData, Monochrome);
	VMU.ColourOffset = offsetof(IconData, Palette);
	for (int y = 0; y < 32; y++)
	{
		for (int x = 0; x < 32; x++)
		{
			if (IconMono.pixel_data[4 * (y * 32 + x)] == 0)
			{
				VMU.Monochrome[(y * 32 + x) / 8] |= 1 << (7 - (x & 7));
			}
		}
	}

	uint32_t Palette32[16];
	int NumEntries = 0;
	for (int y = 0; y < 32; y++)
	{
		for (int x = 0; x < 32; x++)
		{
			uint32_t Pixel = *(uint32_t *)&Icon.pixel_data[4 * (y * 32 + x)];
			int Index;
			for (Index = 0; Index < NumEntries; Index++)
			{
				if (Pixel == Palette32[Index])
				{
					break;
				}
			}
			if (Index == NumEntries)
			{
				if (NumEntries >= 16)
				{
					printf("Too many unique colours\n");
					return 1;
				}
				VMU.Palette[NumEntries] = (((Pixel >> 28) & 0xF) << 12);
				VMU.Palette[NumEntries] |= (((Pixel >> 4) & 0xF) << 8);
				VMU.Palette[NumEntries] |= (((Pixel >> 12) & 0xF) << 4);
				VMU.Palette[NumEntries] |= (((Pixel >> 20) & 0xF) << 0);
				Palette32[NumEntries++] = Pixel;
			}
			VMU.Colour[(y * 32 + x) * 4 / 8] |= (x & 1) ? Index : (Index << 4);
		}
	}

	FILE* f=fopen("ICONDATA.VMS", "wb");
	if (f)
	{
		fwrite(&VMU, sizeof(VMU), 1, f);
		fwrite(&Padding, sizeof(Padding) - sizeof(VMU), 1, f);
		fclose(f);
	}
	else
	{
		printf("Couldn't open ICONDATA.VMS\n");
		return 1;
	}

	uint8_t MemoryCard[128 * 1024];
	memset(MemoryCard, 0, sizeof(MemoryCard));
	//CheckFormatted(MemoryCard);
	f = fopen("VMU.bin", "wb");
	if (f)
	{
		fwrite(&MemoryCard, sizeof(MemoryCard), 1, f);
		fclose(f);
	}
	else
	{
		printf("Couldn't open VMU.bin\n");
	}

	return 0;
}
