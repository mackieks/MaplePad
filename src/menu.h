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

void fast_hsv2rgb_32bit(uint16_t, uint8_t, uint8_t, uint8_t *, uint8_t *, uint8_t *);

bool rainbowCycle(struct repeating_timer*);

void menu(void);