# OFM-LedModule

## Hardware support

At the moment we support following PWM hardware:

- PCA9685
- RP2040 PWM

You need to add some definitions in `hardware.h` for your hardware setup.

### PCA9685

`include\hardware.h`:

```cpp
// e.g.: "LED-UP1-6x24V"
#define LEDMODULE_DIMMER_PCA9685                            // Use PCA9685 dimmer implementation
#define LEDMODULE_WIRE                  Wire1               // I2C interface to talk to PCA9685
#define LEDMODULE_WIRE_SDA              14u                 // Use GPIO14 as I2C1 SDA | UP1 RP2040
#define LEDMODULE_WIRE_SCL              15u                 // Use GPIO15 as I2C1 SCL | UP1 RP2040
#define LEDMODULE_WIRE_CLOCK_FREQ       1000000             // I2C clock speed to be used after initialization
#define LEDMODULE_MAX_LIGHT_CHANNELS    6                   // Defines how many channels the hardware has
```

### RP2040 PWM

`include\hardware.h`:

```cpp
// e.g. "LED-UP1-4x24V"
#define LEDMODULE_DIMMMER_RP2040                            // Use RP2040 dimmer implementation
#define LEDMODULE_MAX_LIGHT_CHANNELS    4                   // Defines how many channels the hardware has
#define LEDMODULE_PWM_PINS              10, 11, 12, 13      // GPIO pins to be used for PWM
```

To use RP2040 PWM peripherals for dimming, OFM-LedModule defines an array with your GPIO pins to be used as dim channels in `LedModuleConfig.cpp`:

```cpp
// e.g. "LED-UP1-4x24V"
uint8_t dimPins[LEDMODULE_MAX_LIGHT_CHANNELS] = {LEDMODULE_PWM_PINS};
```

These GPIOs are then automatically initialized and used for dimming.