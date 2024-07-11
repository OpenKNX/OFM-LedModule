# OFM-LedModule

## Hardware support

At the moment we support following PWM hardware:

- PCA9685
- RP2040 PWM

You need to provide and include file named `LedModuleHWConfig.h` to make definitions for your hardware setup.

### PCA9685

`include\LedModuleHWConfig.h`:

```cpp
// e.g.: "LED-UP1-6x24V"
#define LEDMODULE_DIMMER_PCA9685                            // Use PCA9685 dimmer implementation
#define LEDMODULE_HARDWARE_NAME         "LED-UP1-6x24V"     // Name of the hardware
#define LEDMODULE_WIRE Wire1                                // I2C interfaced to talk to PCA9685
#define LEDMODULE_WIRE_SDA              14u                 // Use GPIO14 as I2C1 SDA | UP1 RP2040
#define LEDMODULE_WIRE_SCL              15u                 // Use GPIO15 as I2C1 SCL | UP1 RP2040
#define LEDMODULE_WIRE_CLOCK_FREQ       1000000             // I2C clock speed to be used after initialization
#define LEDMODULE_MAX_LIGHT_CHANNELS    6                   // Defines how many channels the hardware has
```

### RP2040 PWM

`include\LedModuleHWConfig.h`:

```cpp
// e.g. "LED-UP1-4x24V"
#define LEDMODULE_DIMMMER_RP2040                            // Use RP2040 dimmer implementation
#define LEDMODULE_HARDWARE_NAME         "LED-UP1-4x24V"     // Name of the hardware
#define LEDMODULE_MAX_LIGHT_CHANNELS    4                   // Defines how many channels the hardware has
```

To use RP2040 PWM peripherals for dimming, you have to declare an array with your GPIO pins to be used as channels in one of your c/cpp files.
These GPIOs are then automatically initialized and used for dimming.

`LedModuleHWConfig.cpp`:

```cpp
// e.g. "LED-UP1-4x24V"
uint8_t dimPins[LEDMODULE_MAX_LIGHT_CHANNELS] = {24, 22, 20, 18};
```