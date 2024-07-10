#pragma once

#include "OpenKNX.h"

//TODO: remove these definitions from the module and allow definition in OAM

#define LEDMODULE_HARDWARE_NAME "LED-UP1-4x24V"
#define LEDMODULE_FLASH_SIZE 128
#define LEDMODULE_FLASH_OFFSET 128

//#define LEDMODULE_PWMDRIVER "PCA9685PW"
#define LEDMODULE_WIRE1_SDA 1
#define LEDMODULE_WIRE1_SCL 2
#define LEDMODULE_PWMDRIVER "RP2040"
#define LED_UP1_4x24_PINS           28,29,19,20
#define LED_UP1_4x24_NUM_CHANNELS   4

extern uint8_t dimmPins[4];