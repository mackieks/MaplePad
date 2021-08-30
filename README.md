# MaplePad<br/>

Fork of charcole's Dreamcast controller emulator for RP2040 (Raspberry Pi Pico). 

Feature List:
- [x] Full FT<sub>0</sub> (controller) support including analog joystick and triggers
- [x] Full FT<sub>1</sub> (storage) support with 200-block memory card
- [x] Full FT<sub>2</sub> (LCD) support with SSD1306 128\*64 OLED for VMU display 
- [x] Basic FT<sub>3</sub> (timer/RTC) reporting for compatibility purposes

To-do: (highest priority to lowest)
- [ ] Implement FT<sub>8</sub> (vibration) support
- [ ] Finish FT<sub>3</sub> (timer/RTC) support with either dummy time values or external RTC
- [ ] Multipaging for memory card
- [ ] Implement FT<sub>4</sub> (microphone) support
