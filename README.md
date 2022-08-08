# MaplePad<br/>

<img src="maplepad_logo.png" width="500">

Fork of charcole's Dreamcast controller emulator for RP2040 (Raspberry Pi Pico). Works on my custom MaplePad PCB (see hardware folder) as well as on the Pico (some modification required.) Note: the development SW is currently targeting the stock Pico.

This project is still a WIP, and is not recommended for general use at this time.

Feature List:
- [x] Full FT<sub>0</sub> (controller) support including analog joystick and triggers
- [x] Full FT<sub>1</sub> (storage) support for savegames with 1900 blocks of space
- [x] Multipaging for memory card (8 separate 238-block memory cards)
- [x] Full FT<sub>2</sub> (LCD) support with SSD1331 96\*64 color SPI OLED for VMU display (SSD1306 128\*64 I2C OLED also supported)
- [x] Customizable color palettes for all 8 internal memory cards
- [x] Robust FT<sub>8</sub> (vibration) functionality (WIP)
- [x] Basic FT<sub>3</sub> (timer/RTC) reporting for compatibility purposes

To-do: (highest priority to lowest)
- [ ] Finish FT<sub>8</sub> (vibration) continuous vibration and AST
- [ ] Finish menu for calibrating sticks and customizing MaplePad behavior
- [ ] Implement 'fancy' VMU color palettes (gradients, animated backgrounds, etc.)
- [ ] Implement DC boot animation on OLED
- [ ] Finish FT<sub>3</sub> (timer/RTC) support with either dummy time values or external RTC
- [ ] Implement FT<sub>4</sub> (microphone) support
