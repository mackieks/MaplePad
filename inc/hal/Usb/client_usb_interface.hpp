#include <stdint.h>

extern "C" {
void set_usb_descriptor_number_of_gamepads(uint8_t num);
uint8_t get_usb_descriptor_number_of_gamepads();
}