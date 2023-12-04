# MaplePad🍁<br/>

<img style="border-width:0" src="images/maplepad_logo_shadow.png" width="300">

MaplePad is an all-in-one Dreamcast controller, VMU, and Purupuru (rumble pack) emulator for Dreamcast portables and custom Dreamcast controllers. It runs on RP2040 and is usable with the Raspberry Pi Pico as well as custom MaplePad PCBs. See the [hardware folder](https://github.com/mackieks/MaplePad/tree/main/hardware) for a wiring diagram.

**Note:** MaplePad is still a WIP. You may experience issues with [Windows CE games](https://segaretro.org/Windows_CE). In almost all problematic titles, disabling VMU and rumble through the MaplePad menu will make the game playable. Check out the [Compatibility List](https://docs.google.com/spreadsheets/d/1JzTGN29Ci8SeuSGkQHLN1p6ayQNWUcsNw77SMujkjbs/edit?usp=sharing) for details!

## Features
With MaplePad you can cycle through 8 200-block internal VMUs with custom icons and colors, use an I2C or SPI OLED display to see the VMU screen in color and at 2x integer scale, and enjoy rumble that is 1:1 with the Performance TremorPak in most retail software (still some minor bugs!)

<img src="images/vmu.png" width="750">

### Feature List:
- [x] Full FT<sub>0</sub> (controller) support including analog joystick and triggers
- [x] Full FT<sub>1</sub> (storage) support for savegames with 1600 blocks of space
- [x] Multipaging for memory card (8 separate 200-block memory cards)
- [x] Full FT<sub>2</sub> (LCD) support with SSD1331 96\*64 color SPI OLED for VMU display (monochrome SSD1306 128\*64 I2C OLED also supported)
- [x] Customizable color palettes for all 8 internal memory cards
- [x] Robust FT<sub>8</sub> (vibration) functionality (WIP)
- [x] Robust FT<sub>3</sub> (timer/RTC) reporting for compatibility purposes (no RTC)
- [x] Basic menu on SSD1306 and SSD1331 OLED for configuring MaplePad behavior (WIP)

### To-do: 
Release v1.6 is gated by the following TODOs
- [ ] Finish menu (button test, VMU palette editor, misc. bugs)
- [ ] Address issues with non-CE games (Crazy Taxi, Blue Stinger, Powerstone 2)
- [ ] Finish FT<sub>8</sub> (vibration) continuous vibration and AST

Future TODOs
- [ ] Fix compatibility with Windows CE games
- [ ] Implement 'fancy' VMU color palettes (gradients, animated backgrounds, etc.)
- [ ] Implement option for DC boot animation on OLED
- [ ] Add external RTC for true FT<sub>3</sub> (timer/RTC) support
- [ ] Implement FT<sub>4</sub> (microphone) support

## Project Showcase
*StrikerDC MaplePad mod by Wesk*

<img src="images/striker3.jpg" width="250"> <img src="images/striker1.jpg" width="250"> <img src="images/striker2.jpg" width="250">

*MaplePad Arcade Controller with VMU by GamesCare. [link](https://gamescare.com.br/produto/controle-arcade-dreamcast-tela-de-vmu-e-8-vmus-virtuais-na-placa-maple-board/), [video](https://www.youtube.com/watch?v=b0IbSASR3B4/)*

<img src="images/IMG_20221216_191824_896.jpg" height="190"> <img src="images/20221206_124644.jpg" height="190">

*Mini plug'n'play virtual memory card by jounge. [link](https://tieba.baidu.com/p/8465994390)*

<img src="images/mem1.jpg" height="150"> <img src="images/mem2.jpg" height="150"> <img src="images/mem3.jpg" height="150">

*Various JAMMA Dreamcast adapters available on AliExpress. [link 1](https://www.aliexpress.us/item/3256805216279752.html), [link 2](https://www.aliexpress.us/item/3256804674679708.html), [video](https://www.youtube.com/shorts/UciW3vM-KWo) (flashing lights warning!)*

<img src="images/jamma_adapter_lcd.jpg" height="190"> <img src="images/jamma_adapter.jpg" height="190"> <img src="images/jamma_adapter_basic.jpg" height="190"> 

*Giant Dreamcast VMU + Arcade Stick by CrazyJojo (code modified). [video](https://www.youtube.com/watch?v=bEA_On7P_g8)*

<img src="images/主机演示2[00_05_12][20221030-111434].png" height="170"> <img src="images/从机演示2生化[00_08_05][20221030-111628].png" height="170"> <img src="images/从机演示4[00_01_19][20221030-111601].png" height="170"> 

## Dumping VMUs to PC
You can use [picotool](https://github.com/raspberrypi/picotool) to dump VMUs manually. Here's the process:

- Put RP2040 into programming mode with BOOTSEL button and connect it to your PC
- Use picotool to dump whichever VMU page you want: `picotool save -r 10020000 10040000 dump1.bin`
![image](https://user-images.githubusercontent.com/49252894/211163335-2463ae14-043e-40be-aa93-1a09b1a620f9.png)
- VMUs start at 0x10020000 and each one is 0x20000 long. So to save page 7, for example, you'd use 
`picotool save -r 100E0000 10100000 dump7.bin`
- Open dump in [VMU Explorer](https://segaretro.org/VMU_Explorer)
![image](https://user-images.githubusercontent.com/49252894/211163284-d4100301-11ad-459c-8d29-5afbde9b49f5.png)

## License
<a rel="license" href="http://creativecommons.org/licenses/by/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by/4.0/80x15.png" /></a><br />This work is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by/4.0/">Creative Commons Attribution 4.0 International License</a>.

Share — copy and redistribute the material in any medium or format <br />
Adapt — remix, transform, and build upon the material for any purpose, even commercially. <br />
Attribution — You must give appropriate credit, provide a link to the license, and indicate if changes were made. <br />

MaplePad is forked from [Charlie Cole's Pop'n Music Controller.](https://github.com/charcole/Dreamcast-PopnMusic)

Special thanks: [Charlie Cole](https://github.com/charcole), [Colton Pawielski](https://github.com/cepawiel) and [Wesk](https://www.youtube.com/channel/UCYAwbbBxi5_LK8WVrD10SUw).
