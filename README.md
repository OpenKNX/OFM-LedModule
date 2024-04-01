# OFM-LedModule

## Hardware support

At the moment we support following PWM hardware:

- PCA9685
- (not developed yet, but will come soon) RP2040 PWM

### PCA9685

`include\ledmodule.h`:

```cpp
// e.g.: "LED-UP1-6x24V"
#define LEDMODULE_MAX_LIGHT_CHANNELS 6 // Defines how many channels the hardware has
#define LEDMODULE_PWMDRIVER "PCA9685PW"
#define LEDMODULE_WIRE1_SDA       14u  // Use GP14 as I2C1 SDA | UP1 RP2040
#define LEDMODULE_WIRE1_SCL       15u  // Use GP15 as I2C1 SCL | UP1 RP2040
```

### RP2040 PWM

`include\ledmodule.h`:

```cpp
// e.g. "LED-UP1-4x24V"
#define LEDMODULE_MAX_LIGHT_CHANNELS 4 // Defines how many channels the hardware has
#define LEDMODULE_PWMDRIVER "RP2040"
#define LEDMODULE_CHANNEL_1 24
#define LEDMODULE_CHANNEL_2 22
#define LEDMODULE_CHANNEL_3 20
#define LEDMODULE_CHANNEL_4 18
```
