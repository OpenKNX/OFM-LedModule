#pragma once

#include "OpenKNX.h"

#if !defined(LEDMODULE_DIMMER_PCA9685) & !defined(LEDMODULE_DIMMMER_RP2040)
    #warning Define LEDMODULE_DIMMER_PCA9685 or LEDMODULE_DIMMMER_RP2040 in LedModuleHWConfig.h
    #define LEDMODULE_PWMDRIVER "Not Configured!"
#endif

#ifndef LEDMODULE_MAX_LIGHT_CHANNELS
    #define LEDMODULE_MAX_LIGHT_CHANNELS 1
    #warning LEDMODULE_MAX_LIGHT_CHANNELS not defined, using default value (1)
#endif

#ifndef LEDMODULE_PWM_FREQ
    #define LEDMODULE_PWM_FREQ 1000
    #warning LEDMODULE_PWM_FREQ not defined, using default value (1000)
#endif

#ifndef LEDMODULE_WIRE
    #define LEDMODULE_WIRE Wire1
    #ifdef LEDMODULE_DIMMER_PCA9685 // only show warning when using PCA dimmer
        #warning LEDMODULE_WIRE not defined, using default value (Wire1)
    #endif
#endif

#if !defined(LEDMODULE_PWM_PINS) & defined(LEDMODULE_DIMMER_PCA9685)
    #define LEDMODULE_PWM_PINS 0 // dummy value to define pins arry for compiler
#endif

#define LEDMODULE_HARDWARE_NAME "OFM-LedModule"

/**
 * @brief HW dimming is based on I2C PWM driver IC PCA9685PW
 */
#ifdef LEDMODULE_DIMMER_PCA9685
    #define LEDMODULE_PWMDRIVER "PCA9685PW"
    #define LEDMODULE_FLASH_SIZE 128 // TODO: find proper values
    #define LEDMODULE_FLASH_OFFSET 128

    #ifndef LEDMODULE_WIRE_CLOCK_FREQ
        #define LEDMODULE_WIRE_CLOCK_FREQ 100000 // default to 100kHz to support all devices
        #warning LEDMODULE_WIRE_CLOCK_FREQ not defined, using default value (100kHz)
    #endif
#endif

/**
 * @brief HW dimming is based on RP2040 PWM peripheral
 */
#ifdef LEDMODULE_DIMMMER_RP2040
    #define LEDMODULE_PWMDRIVER "RP2040"
    #define LEDMODULE_FLASH_SIZE 128 // TODO: find proper values
    #define LEDMODULE_FLASH_OFFSET 128

    extern uint8_t dimPins[LEDMODULE_MAX_LIGHT_CHANNELS];
#endif
