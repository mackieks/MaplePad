# MaplePad 

Replacement Dreamcast controller PCB using RP2040. EAGLE source, Gerbers, BOM and assembly drawing available in this directory. Photo below is of the previous revision which lacked 

<img src="maplepad_pcb.png" width="500">

Recommended board fabrication specs: 
- 0.8mm 2-layer PCB
- ENIG (for chipscale BGAs)
- Red soldermask - optional, but in keeping with the Japanese maple theme :) 

JLCPCB may grow the drills under the flash to meet their spec-- this is OK. Also please note that the extremely small castellations often result in many 'crushed barrels' that require manual cleanup.

<img src="maplepad_pro.png" width="350">

Features:
- [x] 18.5 x 22.4mm 2-layer PCB
- [x] USB-C and PROG button for programming
- [x] Full Dreamcast controller compatibility
- [x] Analog joystick and triggers
- [x] VMU emulation (8-page VMU, 1600 blocks total)
- [x] SPI/I2C OLED for VMU screen
- [x] Rumble (Purupuru) support (WIP)
- [x] Power from 1.8V - 5.5V

What it doesn't support:
- [ ] Dual analog sticks
- [ ] Dreamcast Microphone
- [ ] Dreamcast Keyboard, Mouse, or Fishing Controller

A future version may support these features.
