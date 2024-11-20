#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

// true to setup and print debug messages over UART0 (pin 1, 115200 8N1)
// Warning: enabling debug messages drastically degrades communication performance
#define SHOW_DEBUG_MESSAGES false

// true to enable USB CDC (serial) interface to directly control the maple bus
#define USB_CDC_ENABLED true

// Adjust the CPU clock frequency here (133 MHz is maximum documented stable frequency)
#define CPU_FREQ_KHZ 133000

// The minimum amount of time we check for an open line before taking control of it
// Set to 0 to completely disable this check
#define MAPLE_OPEN_LINE_CHECK_TIME_US 10

// Amount of time in nanoseconds at which each bit transmits (value should be divisible by 3)
// 480 ns achieves just over 2 mbps, just as the Dreamcast does
#define MAPLE_NS_PER_BIT 480

// Added percentage on top of the expected write completion duration to use for timeout
#define MAPLE_WRITE_TIMEOUT_EXTRA_PERCENT 20

// Estimated nanoseconds before peripheral responds - this is used for scheduling only
#define MAPLE_RESPONSE_DELAY_NS 50

// Maximum amount of time waiting for the beginning of a response when one is expected
#define MAPLE_RESPONSE_TIMEOUT_US 1000

// Estimated nanoseconds per bit to receive data - this is used for scheduling only
// 1750 was selected based on the average time it takes a Dreamcast controller to transmit each bit
#define MAPLE_RESPONSE_NS_PER_BIT 1750

// Maximum amount of time in microseconds to pass in between received words before read is canceled
// Dreamcast controllers sometimes have a ~180 us gap between words, so 300 us accommodates for that
#define MAPLE_INTER_WORD_READ_TIMEOUT_US 300

// The pin which sets IO direction for each player (-1 to disable)
#define P1_DIR_PIN -1
//#define P2_DIR_PIN -1
//#define P3_DIR_PIN -1
//#define P4_DIR_PIN -1

// True if DIR pin is HIGH for output and LOW for input; false if opposite
#define DIR_OUT_HIGH true

// The start pin of the two-pin bus for each player
#define P1_BUS_START_PIN 11
//#define P2_BUS_START_PIN 12
//#define P3_BUS_START_PIN 18
//#define P4_BUS_START_PIN 20

// LED pin number for USB activity or -1 to disable
// When USB connected:
//   Default: ON
//   When controller key pressed: OFF
//   When controller disconnecting: Flashing slow
// When USB disconnected:
//   Default: OFF
//   When controller key pressed: Flashing quick
#define USB_LED_PIN 25

// LED pin number for simple USB activity or -1 to disable
// ON when USB connected; OFF when disconnected
#define SIMPLE_USB_LED_PIN -1

// The current firmware version
#define CURRENT_FW_VERSION VER_2

#endif // __CONFIGURATION_H__
