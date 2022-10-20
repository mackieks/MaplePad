/*
 * MaplePad Menu
 *
 * Attempt at a flexible & extensible hierarchical menu system 
 * 
 * TO-DO:
 * ☐ VMU Palette submenu
 * ☐ UI Palette submenu
 * ☐ Deadzone adjust submenus for stick + trigger
 * ☐ Button test submenu (lineart + ellipse routine for now)
 * ☐ OLED Detect (ssd1331present, needs to make certain menu items invisible if ssd1306 is selected)
 * 
 */

#include "maple.h"
#include "menu.h"
#include "display.h"



uint32_t flipLockout;
uint ssd1331present = 1;
volatile bool redraw = 1;

struct repeating_timer redrawTimer;

static uint16_t color = 0x0000;

int paletteVMU(menuItem *self){
    // draw VMU palette selection
    return(1);
}

int paletteUI(menuItem *self){
    // draw UI palette selection
    return(1);
}

int buttontest(menuItem *self){
    // draw button test
	
    return(1);
}

int sCal(menuItem *self){
    // draw stick calibration
	redraw = 0; // Disable redrawMenu

	while(!gpio_get(ButtonInfos[0].InputIO));

    clearDisplay(oledType);
    char *cal_string = "center stick";
    putString(cal_string, 0, 0, 0x049f);
	cal_string = "and press A";
	putString(cal_string, 0, 1, 0x049f);
    updateDisplay(oledType);

	sleep_ms(500);
	while(gpio_get(ButtonInfos[0].InputIO));

    adc_select_input(0); // X
    xCenter = adc_read() >> 4;

    adc_select_input(1); // Y
    yCenter = adc_read() >> 4;

    clearDisplay(oledType);
    cal_string = "xMin left";
    putString(cal_string, 0, 0, 0x049f);
	cal_string = "and press A";
	putString(cal_string, 0, 1, 0x049f);
    updateDisplay(oledType);

	sleep_ms(500);
	while(gpio_get(ButtonInfos[0].InputIO));

    adc_select_input(0); // Xmin
    xMin = adc_read() >> 4;

    clearDisplay(oledType);
    cal_string = "yMin up";
    putString(cal_string, 0, 0, 0x049f);
	cal_string = "and press A";
	putString(cal_string, 0, 1, 0x049f);
    updateDisplay(oledType);

	sleep_ms(500);
	while(gpio_get(ButtonInfos[0].InputIO));

    adc_select_input(1); // Ymin
    yMin = adc_read() >> 4;

    clearDisplay(oledType);
    cal_string = "yMax down";
    putString(cal_string, 0, 0, 0x049f);
	cal_string = "and press A";
	putString(cal_string, 0, 1, 0x049f);
    updateDisplay(oledType);

	sleep_ms(500);
	while(gpio_get(ButtonInfos[0].InputIO));

    adc_select_input(1); // Ymax
    yMax = adc_read() >> 4;

    clearDisplay(oledType);
    cal_string = "xMax right";
    putString(cal_string, 0, 0, 0x049f);
	cal_string = "and press A";
	putString(cal_string, 0, 1, 0x049f);
    updateDisplay(oledType);

	sleep_ms(500);
	while(gpio_get(ButtonInfos[0].InputIO));

    adc_select_input(0); // Xmax
    xMax = adc_read() >> 4;

	if (xMin > xMax){
		uint temp = xMin;
		xMin = xMax;
		xMax = temp;
	}

	if (yMin > yMax){
		uint temp = yMin;
		yMin = yMax;
		yMax= temp;
	}

    // Write config values to flash
    updateFlashData();

	redraw = 1;

    return(1);
}

int tCal(menuItem *self){
    // draw trigger calibration
	redraw = 0; // Disable redrawMenu

	while(!gpio_get(ButtonInfos[0].InputIO));

    clearDisplay(oledType);
    char *cal_string = "leave";
    putString(cal_string, 0, 0, 0x049f);
	cal_string = "triggers idle";
	putString(cal_string, 0, 1, 0x049f);
	cal_string = "and press A";
	putString(cal_string, 0, 2, 0x049f);
    updateDisplay(oledType);

	sleep_ms(500);
	while(gpio_get(ButtonInfos[0].InputIO));

    adc_select_input(2); // L
    lMin = adc_read() >> 4;

    adc_select_input(3); // R
    rMin = adc_read() >> 4;

    clearDisplay(oledType);
    cal_string = "hold lMax";
    putString(cal_string, 0, 0, 0x049f);
	cal_string = "and press A";
	putString(cal_string, 0, 1, 0x049f);
    updateDisplay(oledType);

	sleep_ms(500);
	while(gpio_get(ButtonInfos[0].InputIO));

    adc_select_input(2); // lMax
    lMax = adc_read() >> 4;

    clearDisplay(oledType);
    cal_string = "hold rMax";
    putString(cal_string, 0, 0, 0x049f);
	cal_string = "and press A";
	putString(cal_string, 0, 1, 0x049f);
    updateDisplay(oledType);

	sleep_ms(500);
	while(gpio_get(ButtonInfos[0].InputIO));

    adc_select_input(3); // rMax
    rMax = adc_read() >> 4;

	if (lMin > lMax){
		uint temp = lMin;
		lMin = lMax;
		lMax = temp;
	}

	if (rMin > rMax){
		uint temp = rMin;
		rMin = rMax;
		rMax= temp;
	}

    // Write config values to flash
    updateFlashData();

	redraw = 1;

    return(1);
}

int sDeadzone(menuItem *self){
    // draw deadzone configuration
    return(1);
}

int tDeadzone(menuItem *self){
    // draw deadzone configuration
    return(1);
}

int toggleOption(menuItem *self){

	if(!strcmp(self->name, "OLED Flip     ")){
		if((to_ms_since_boot(get_absolute_time()) - flipLockout) > 500){

			if(self->type == 1)
    			self->on = !(self->on);

			flipLockout = to_ms_since_boot(get_absolute_time());
			
			cancel_repeating_timer(&redrawTimer);
			updateFlags();
			updateFlashData();

			ssd1331_init();
			sleep_ms(100);

			add_repeating_timer_ms(-10, rainbowCycle, NULL, &redrawTimer);
			return(1);
		} else return(1);
	} else {
		if(self->type == 1)
    		self->on = !(self->on);
	}

    return(1);
} 

int exitToPad(menuItem *self){
	// gather up flags and update them
	updateFlags();
    return(0);
} 

int dummy(menuItem* self){
	return(1);
}

menuItem mainMenu[6] = {
    {"Button Test   ", 2, 1, 1, 1, 1, buttontest},
	{"Stick Config  ", 0, 1, 0, 1, 1, dummy},
	{"Trigger Config", 0, 1, 0, 1, 1, dummy},
    {"Edit VMU Color", 2, 1, 0, 1, 1, paletteVMU}, // ssd1331 present
	{"Settings      ", 0, 1, 0, 1, 1, dummy},
    {"Exit          ", 2, 0, 0, 1, 1, exitToPad}
};

menuItem* currentMenu = mainMenu;
uint8_t currentNumEntries = sizeof(mainMenu)/sizeof(menuItem);
uint8_t currentEntry = 0;
uint8_t selectedEntry = 0;
uint8_t firstVisibleEntry = 0;
uint8_t lastVisibleEntry = 4;
uint8_t prevEntryModifier = 0;
uint8_t entryModifier = 0;

int mainmen(menuItem* self){
    currentMenu = mainMenu;
	currentNumEntries = sizeof(mainMenu)/sizeof(menuItem);
	entryModifier = prevEntryModifier;
    return(1);
}

static menuItem stickConfig[6] = {
    {"Back          ", 2, 1, 0, 1, 1, mainmen}, // i.e. setCurrentMenu to mainMenu
    {"Calibration   ", 2, 1, 1, 1, 1, sCal},
    {"Deadzone Edit ", 2, 1, 0, 1, 1, sDeadzone},
    {"Invert X      ", 1, 1, 0, 0, 1, toggleOption},
    {"Invert Y      ", 1, 1, 0, 0, 1, toggleOption},
    {"Swap X&Y      ", 1, 0, 0, 0, 1, toggleOption}
};

int sConfig(menuItem* self){
    currentMenu = stickConfig;
    currentNumEntries = sizeof(stickConfig)/sizeof(menuItem);
	prevEntryModifier = entryModifier;
	entryModifier = 0;
    return(1);
}

static menuItem triggerConfig[6] = {
    {"Back          ", 2, 1, 0, 1, 1, mainmen},
    {"Calibration   ", 2, 1, 1, 1, 1, tCal},
	{"Deadzone Edit ", 2, 1, 0, 1, 1, tDeadzone},
    {"Invert L      ", 1, 1, 0, 0, 1, toggleOption},
    {"Invert R      ", 1, 1, 0, 0, 1, toggleOption},
    {"Swap L&R      ", 1, 1, 0, 0, 1, toggleOption}
};

int tConfig(menuItem* self){
    currentMenu = triggerConfig;
    currentNumEntries = sizeof(triggerConfig)/sizeof(menuItem);
	prevEntryModifier = entryModifier;
	entryModifier = 0;
    return(1);
}

static menuItem settings[8] = {
    {"Back          ", 2, 1, 0, 1, 1, mainmen},
    {"Boot Video    ", 3, 1, 1, 0, 0, toggleOption},
    {"Rumble        ", 1, 1, 0, 1, 1, toggleOption},
	{"VMU           ", 1, 1, 0, 1, 1, toggleOption},
    {"UI Color      ", 2, 1, 0, 1, 1, paletteUI}, // ssd1331 present
    {"OLED:  SSD1331", 3, 0, 0, 1, 0, toggleOption},
	{"OLED Flip     ", 1, 0, 0, 0, 1, toggleOption},
    {"Firmware  1.4c", 3, 0, 0, 1, 0, dummy}
};

int setting(menuItem* self){
    currentMenu = settings;
    currentNumEntries = sizeof(settings)/sizeof(menuItem);
	prevEntryModifier = entryModifier;
	entryModifier = 0;
    return(1);
}

void loadFlags(){
	stickConfig[3].on = invertX;
	stickConfig[4].on = invertY;
	stickConfig[5].on = swapXY;
	triggerConfig[2].on = invertL;
	triggerConfig[3].on = invertR;
	triggerConfig[4].on = swapLR;
	settings[2].on = rumbleEnable;
	settings[3].on = vmuEnable;
	settings[6].on = oledFlip; 
}

void updateFlags(){
	invertX = stickConfig[3].on;
	invertY = stickConfig[4].on;
	swapXY = stickConfig[5].on;
	invertL = triggerConfig[2].on;
	invertR = triggerConfig[3].on;
	swapLR = triggerConfig[4].on;
	rumbleEnable = settings[2].on;
	vmuEnable = settings[3].on;
	oledFlip = settings[6].on; 
}

void getSelectedEntry(){
    for(int i = 0; i < currentNumEntries; i++){
        if(currentMenu[i].selected){
            selectedEntry = i;
            break;
        }
    }
}

void getFirstVisibleEntry(){
	for(int i = 0; i < currentNumEntries; i++){
		if(currentMenu[i].visible){
			firstVisibleEntry = i;
			break;
		}
	}
}

void getLastVisibleEntry(){
	for(int i = currentNumEntries - 1; i >= 0; i--){
		if(currentMenu[i].visible){
			lastVisibleEntry = i;
			break;
		}
	}
}

void redrawMenu(){
	clearDisplay(oledType);

	for(uint8_t n = 0; n < currentNumEntries; n++){
		if(currentMenu[n].visible){
			putString(currentMenu[n].name, 0, n + entryModifier, color);
			if(currentMenu[n].type == 1) // boolean type menuItem
				drawToggle(n + entryModifier, color, currentMenu[n].on);
		}
		drawCursor(selectedEntry + entryModifier, color);
	}
	updateDisplay(oledType);
}

static uint16_t hue = 0;

bool rainbowCycle(struct repeating_timer *t){
	static uint8_t r = 0x00;
	static uint8_t g = 0x00;
	static uint8_t b = 0x00;

	fast_hsv2rgb_32bit(hue, 255, 255, &r, &g, &b);

	color = ((r & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (b >> 3);

	if(hue == 1535)
		hue = 0;
	else hue++;

	if(redraw)
		redrawMenu();

	return(true);
}

void menu(){

// Check for menu button combo (Start + Down + B)
	
		/* Menu
		-Stick to 96x64 size even on monochrome OLED... Simpler!
		-Need method to handle number of current menu entries. Iterate through different arrays of menu_item structs to know which entries to draw...?
		-menu_item struct can have a "visible" flag that menuRedraw or menuScroll will toggle
		-Function call to redraw menu list (menuRedraw()?)
		-Draw cursor next to current menu item, move up and down when dpad (or optionally, analog stick) is moved
		-Cursor should default to second item on menus with 'Back' option
		-Scroll menu when cursor reaches bottom of screen

		-SSD1306 and SSD1331 drawing functions can accelerate menu development:
			-Drawing squares with top left and bottom right coordinates and specific color ✓
			-Drawing hollow rectangles with inner and outer top left and bottom right coordinates and specific color ✓
			-Drawing circles (AA?) at certain coordinates ✓
			-Drawing cursor at generic menu item locations ✓
			-Drawing menu items (text) at prescribed locations using specific font ✓

		Menu Layout:
		-Button Test
			-Enters button test screen (Press and hold B to exit)
		-Sticks Config
			-Back
			-Calibration ✓
			-Deadzone Adjust
			-Invert X  ✓
			-Invert Y  ✓
			-Swap X and Y
		-Triggers Config
			-Back
			-Analog/Digital
			-Calibration, greyed out if digital selected
			-Invert L, greyed out if digital selected
			-Invert R, greyed out if digital selected
			-Swap L and R
		-Edit VMU Colors (greyed out if SSD1306 detected)
			-Enters VMU Colors screen. Cycle left and right through all 8 VMU pages, and select from 8 preset colors or enter a custom RGB565 value (Press and hold B to exit)
		-Settings
			-Back
			-Splashscreen (on/off), gets greyed out when Boot Video is turned on
			-Boot Video (on/off), gets greyed out when Splashscreen is turned on or if SSD1306 detected
			-Rumble (on/off)
			-UI Color (same interface as VMU Colors, greyed out if SSD1306 detected)
			-Control UI with analog stick (on/off)
			-OLED Model (shows 'SSD1306' or 'SSD1331' next to text)
			-Firmware Version (shows firmware version next to text)
		-Exit

		*/
		
		// enter main menu
		 // needs to be declared globally
		//selectedEntry = ? getSelectedEntry should update this global variable ✓

		mainMenu[1].run = sConfig;
		mainMenu[2].run = tConfig;
		mainMenu[4].run = setting;

		loadFlags();

		// negative interval means the callback func is called every 10ms regardless of how long callback takes to execute
		add_repeating_timer_ms(-10, rainbowCycle, NULL, &redrawTimer);
	
		while(1){
			getSelectedEntry(); // where to draw cursor
			//redrawMenu(); // called by redrawTimer

			// Wait for A button release (submenu rate-limit)
			while(!gpio_get(ButtonInfos[0].InputIO));

			uint8_t pressed = 0;
			do {
				for (int i = 0; i < 9; i++){	
					pressed |= (!gpio_get(ButtonInfos[i].InputIO));
				}
			} while(!pressed);

			sleep_ms(75); // Wait out switch bounce + rate-limiting

			if(!gpio_get(ButtonInfos[4].InputIO)){ // Up
				// check currently selected element
				// if element is not the top one, deselect current element 
				// and select element above it
				if(selectedEntry){  // i.e. not 0
					currentMenu[selectedEntry].selected = 0;
					currentMenu[selectedEntry - 1].selected = 1;
				

				getFirstVisibleEntry();
				if((selectedEntry == firstVisibleEntry) && (firstVisibleEntry)){
					currentMenu[firstVisibleEntry + 4].visible = 0;
					currentMenu[firstVisibleEntry - 1].visible = 1;
					entryModifier++;
				}

				}

			}

			else if(!gpio_get(ButtonInfos[5].InputIO)){ // Down
				// check currently selected element
				// if element is not the bottom one, deselect current element
				// and select element below it
				if(selectedEntry < currentNumEntries - 1)
				{
					currentMenu[selectedEntry].selected = 0;
					currentMenu[selectedEntry + 1].selected = 1;

				getLastVisibleEntry();
				if((selectedEntry == lastVisibleEntry) && (lastVisibleEntry < currentNumEntries)){
					currentMenu[lastVisibleEntry - 4].visible = 0;
					currentMenu[lastVisibleEntry + 1].visible = 1;
					entryModifier--;
				}

				} 
			}

			else if(!gpio_get(ButtonInfos[0].InputIO)){ // A
				// check currently selected element
				// if element is enabled, run element's function
				// element functions should set currentMenu if they enter a submenu.
				if(currentMenu[selectedEntry].enabled)
					if(!currentMenu[selectedEntry].run(&currentMenu[selectedEntry])) break;
			}
			// The elements' functions should all return 1 except for mainMenu.exitToPad,  
			// which should return 0 and result in a break from this while loop.
		}
	cancel_repeating_timer(&redrawTimer);
}
