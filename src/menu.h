/*
    Menu 
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "ssd1331.h"
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

typedef struct menuItem_s menuItem;

struct menuItem_s
{
	char name[14];
    int type;   // 0: "folder" with submenus, 1: boolean on/off, 2: active (loads separate window), 3: inert
	bool visible;
    bool selected;
    bool on;
    bool enabled;   // control for greyed-out menu items
    int (*run)(menuItem *self);
};

int paletteVMU(menuItem*);

int paletteUI(menuItem*);

int buttontest(menuItem*);

int stickcal(menuItem*);

int trigcal(menuItem*);

int deadzone(menuItem*);

int toggleOption(menuItem*);

int exitToPad(menuItem*);

int dummy(menuItem*);

int mainmen(menuItem*);

int sconfig(menuItem*);

int tconfig(menuItem*);

int setting(menuItem*);

void getSelectedElement(void);

void loadFlags(void);

void updateFlags(void);

void redrawMenu(void);

bool rainbowCycle(struct repeating_timer*);

void menu(void);