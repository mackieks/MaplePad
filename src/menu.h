/*
    Menu 
*/

#pragma once 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "ssd1331.h"
#include "ssd1306.h"

#include "pico/stdlib.h"

#define xCenter flashData[0]
#define xMin flashData[1]
#define xMax flashData[2]
#define yCenter flashData[3]
#define yMin flashData[4]
#define yMax flashData[5]
#define lMin flashData[6]
#define lMax flashData[7]
#define rMin flashData[8]
#define rMax flashData[9]
#define invertX flashData[10]
#define invertY flashData[11]
#define invertL flashData[12]
#define invertR flashData[13]
#define firstBoot flashData[14]
#define currentPage flashData[15]
#define rumbleEnable flashData[16]
#define vmuEnable flashData[17]
#define oledFlip flashData[18] 
#define swapXY flashData[19]
#define swapLR flashData[20]
#define oledType flashData[21]
#define triggerMode flashData[22] // 1 = analog, 0 = digital
#define xDeadzone flashData[23]
#define xAntiDeadzone flashData[24]
#define yDeadzone flashData[25]
#define yAntiDeadzone flashData[26]
#define lDeadzone flashData[27]
#define lAntiDeadzone flashData[28]
#define rDeadzone flashData[29]
#define rAntiDeadzone flashData[30]

extern ButtonInfo ButtonInfos[];

typedef struct menu_s menu;

struct menu_s
{
	char name[14];
    int type;   // 0: submenu, 1: boolean toggle, 2: function, 3: inert
	bool visible;
    bool selected;
    bool on;
    bool enabled;   // control for hidden menu items (ssd1306)
    int (*run)(menu *self);
};

int paletteVMU(menu*);

int paletteUI(menu*);

int buttontest(menu*);

int stickcal(menu*);

int trigcal(menu*);

int deadzone(menu*);

int toggleOption(menu*);

int exitToPad(menu*);

int dummy(menu*);

int mainmen(menu*);

int sconfig(menu*);

int tconfig(menu*);

int setting(menu*);

void getSelectedElement(void);

void loadFlags(void);

void updateFlags(void);

void redrawMenu(void);

bool rainbowCycle(struct repeating_timer*);

void runMenu(void);