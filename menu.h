/*
    Menu 
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "pico/stdlib.h"

// extern ButtonInfo ButtonInfos[];
// uint ssd1331present = 1;

// typedef struct menuItem_s menuItem;

// struct menuItem_s
// {
// 	char* name[16];
//     int type;   // 0: "folder" with submenus, 1: boolean on/off, 2: active (loads separate window), 3: inert
// 	bool visible;
//     bool selected;
//     bool on;
//     bool enabled;   // control for greyed-out menu items
//     int (*run)(menuItem *self);
// };

// menuItem* currentMenu = 0;
// uint8_t selectedElement = 0;

// int paletteVMU(menuItem *self){
//     // draw VMU palette selection
//     return(1);
// }

// int paletteUI(menuItem *self){
//     // draw UI palette selection
//     return(1);
// }

// int buttontest(menuItem *self){
//     // draw button test
//     return(1);
// }

// int stickcal(menuItem self){
//     // draw stick calibration
//     return(1);
// }

// int trigcal(menuItem self){
//     // draw trigger calibration
//     return(1);
// }

// int deadzone(menuItem self){
//     // draw deadzone configuration
//     return(1);
// }

// int toggleOption(menuItem self){
//     if(self.type == 1)
//         self.on = !self.on;
//     return(1);
// } 

// int exitToPad(menuItem self){
//     return(0);
// } 

// void getSelectedElement(){
//     for(int i = 0; i < (sizeof(currentMenu)/sizeof((currentMenu)[0])); i++){
//         if((currentMenu)[i].selected){
//             selectedElement = i;
//             break;
//         }
//     }
// }

// static menuItem mainMenu[6] = {
//     {"Button Test    ", 2, 1, 1, 1, 1, buttontest},
//     {"Stick Config   ", 0, 1, 0, 1, 1, sconfig},
//     {"Trigger Config ", 0, 1, 0, 1, 1, tconfig},
//     {"Edit VMU Colors", 2, 1, 0, 1, 1, paletteVMU}, // ssd1331 present
//     {"Settings       ", 0, 1, 0, 1, 1, setting},
//     {"Exit           ", 2, 1, 0, 1, 1, exitToPad}
// };


// static menuItem stickConfig[6] = {
//     {"Back           ", 2, 1, 0, 1, 1, mainmen}, // i.e. setCurrentMenu to mainMenu
//     {"Calibration    ", 2, 1, 1, 1, 1, stickcal},
//     {"Deadzone Adjust", 2, 1, 0, 1, 1, deadzone},
//     {"Invert X       ", 1, 1, 0, 0, 1, toggleOption},
//     {"Invert Y       ", 1, 1, 0, 0, 1, toggleOption},
//     {"Swap X and Y   ", 1, 1, 0, 0, 1, toggleOption}
// };


// static menuItem triggerConfig[5] = {
//     {"Back           ", 2, 1, 0, 1, 1, mainmen},
//     {"Calibration    ", 2, 1, 1, 1, 1, trigcal},
//     {"Invert X       ", 1, 1, 0, 0, 1, toggleOption},
//     {"Invert Y       ", 1, 1, 0, 0, 1, toggleOption},
//     {"Swap X and Y   ", 1, 1, 0, 0, 1, toggleOption}
// };


// static menuItem settings[7] = {
//     {"Back           ", 2, 1, 0, 1, 1, mainmen},
//     {"Splashscreen   ", 1, 1, 1, 1, 1, toggleOption},
//     {"Boot Video     ", 1, 1, 0, 0, 0, toggleOption},
//     {"Rumble         ", 1, 1, 0, 1, 1, toggleOption},
//     {"UI Color       ", 2, 1, 0, 1, 1, paletteUI}, // ssd1331 present
//     {"OLED Model     ", 3, 1, 0, 1, 0, toggleOption},
//     {"Firmware Ver.  ", 3, 1, 0, 1, 0, toggleOption}
// };

// int mainmen(menuItem* self){
//     currentMenu = mainMenu;
//     return(1);
// }

// int sconfig(menuItem* self){
//     currentMenu = stickConfig;
//     return(1);
// }

// int tconfig(menuItem* self){
//     currentMenu = triggerConfig;
//     return(1);
// }

// int setting(menuItem* self){
//     currentMenu = settings;
//     return(1);
// }

// int dummy(menuItem* self){
// 	return(1);
// }


// void menu(){

// // Check for menu button combo (Start + Down + B)
	
// 		/* Menu
// 		-Stick to 96x64 size even on monochrome OLED... Simpler!
// 		-Need method to handle number of current menu entries. Iterate through different arrays of menu_item structs to know which entries to draw...?
// 		-menu_item struct can have a "visible" flag that menuRedraw or menuScroll will toggle
// 		-Function call to redraw menu list (menuRedraw()?)
// 		-Draw cursor next to current menu item, move up and down when dpad (or optionally, analog stick) is moved
// 		-Cursor should default to second item on menus with 'Back' option
// 		-Scroll menu when cursor reaches bottom of screen

// 		-SSD1306 and SSD1331 drawing functions can accelerate menu development:
// 			-Drawing squares with top left and bottom right coordinates and specific color ✓
// 			-Drawing hollow rectangles with inner and outer top left and bottom right coordinates and specific color ✓
// 			-Drawing circles (AA?) at certain coordinates
// 			-Drawing cursor at generic menu item locations
// 			-Drawing menu items (text) at prescribed locations using specific font

// 		Menu Layout:
// 		-Button Test
// 			-Enters button test screen (Press and hold B to exit)
// 		-Sticks Config
// 			-Back
// 			-Calibration
// 			-Deadzone Adjust
// 			-Invert X 
// 			-Invert Y 
// 			-Swap X and Y
// 		-Triggers Config
// 			-Back
// 			-Analog/Digital
// 			-Calibration, greyed out if digital selected
// 			-Invert L, greyed out if digital selected
// 			-Invert R, greyed out if digital selected
// 			-Swap L and R
// 		-Edit VMU Colors (greyed out if SSD1306 detected)
// 			-Enters VMU Colors screen. Cycle left and right through all 8 VMU pages, and select from 8 preset colors or enter a custom RGB565 value (Press and hold B to exit)
// 		-Settings
// 			-Back
// 			-Splashscreen (on/off), gets greyed out when Boot Video is turned on
// 			-Boot Video (on/off), gets greyed out when Splashscreen is turned on or if SSD1306 detected
// 			-Rumble (on/off)
// 			-UI Color (same interface as VMU Colors, greyed out if SSD1306 detected)
// 			-Control UI with analog stick (on/off)
// 			-OLED Model (shows 'SSD1306' or 'SSD1331' next to text)
// 			-Firmware Version (shows firmware version next to text)
// 		-Exit

// 		*/
		
// 		// enter main menu
// 		 // needs to be declared globally
// 		//selectedElement = ? getSelectedElement should update this global variable ✓





// 		while(1){
// 			getSelectedElement(); // where to draw cursor
// 			redrawMenu(); // should include redrawing cursor based on getSelectedElement()!

// 			// Wait for button release
// 			uint8_t pressed = 0;
// 			do {
// 				for (int i = 0; i < 9; i++){
// 					pressed |= (!gpio_get(ButtonInfos[i].InputIO));
// 				}
// 			} while(pressed);

// 			sleep_ms(75); // Wait out switch bounce + rate-limiting

// 			if(!gpio_get(ButtonInfos[4].InputIO)){
// 				// check currently selected element
// 				// if element is not the top one, deselect current element 
// 				// and select element above it
// 				if(selectedElement){ 
// 					currentMenu[selectedElement].selected = 0;
// 					currentMenu[selectedElement - 1].selected = 1;
// 				}
// 			}

// 			else if(!gpio_get(ButtonInfos[5].InputIO)){
// 				// check currently selected element
// 				// if element is not the bottom one, deselect current element
// 				// and select element below it
// 				if(selectedElement < (sizeof(currentMenu)/sizeof(currentMenu[0])))
// 				{
// 					currentMenu[selectedElement].selected = 0;
// 					currentMenu[selectedElement + 1].selected = 1;
// 				}
// 			}

// 			else if(!gpio_get(ButtonInfos[0].InputIO)){
// 				// check currently selected element
// 				// if element is enabled, run element's function
// 				// element functions should set currentMenu if they enter a submenu.
// 				if(currentMenu[selectedElement].enabled)
// 					if(!currentMenu[selectedElement].run(*currentMenu)) break;
// 			}
// 			// The elements' functions should all return 1 except for mainMenu.exitToPad,  
// 			// which should return 0 and result in a break from this while loop.
// 		}

// }
