# MaplePad<br/>

<img src="maplepad_logo_shadow.png" width="500">

MaplePad is an all-in-one Dreamcast controller, VMU, and Purupuru (rumble pack) emulator that is intended for use in Dreamcast portables and custom Dreamcast controllers. It can be used with either the Raspberry Pi Pico or a custom MaplePad PCB (see hardware folder.) 

Note: MaplePad is still a WIP, and is not recommended for general use at this time.

With MaplePad you can: cycle through 8 238-block internal VMUs with custom icons and colors at your leisure...

<img src="vmu.png" width="750">

...use an I2C or SPI OLED display to see the VMU screen in color and at 2x integer scale...

<img src="purupuru.png" width="750">

...and enjoy rumble that is 1:1 with the Performance TremorPak in most retail software (still some minor bugs!)

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

MaplePad is forked from [Charlie Cole's Pop'n Music Controller.](https://github.com/charcole/Dreamcast-PopnMusic)

Special thanks: [Charlie Cole](https://github.com/charcole), [Colton Pawielski](https://github.com/cepawiel) and [Wesk](https://www.youtube.com/channel/UCYAwbbBxi5_LK8WVrD10SUw).
