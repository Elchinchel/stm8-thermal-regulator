# STM8 based thermal regulator

This is code for thermal regulator with
stm-8 as controller, ds18b20 as thermal sensor,
three digit 7-segment display for indication
and two control buttons.

Based on Sduino framework and
intended to be compiled with SDCC in Platformio IDE.

## Configuration
    For meaning of buttons and output cycle see control section of this readme.

Pins are configured with *pin variables in the beginning of [main.c](src/main.c#L31-L36)\
11 pins is used for display (7 + 1 period for segments, 3 for digits),
2 for each button,
1 for temperature sensor and 1 for heater/cooler output
(by default output pin configured as Open Drain and
has low level when output is on, so PNP BJT or
P-channel MOSFET might be used as output amplifier).

Some logic behaviour can be tuned with constants defined after pin variables, below description of the most useful ones:
(temperature defined as t°C * 10, i.e. 100 means 10°C)
| Variable name   | Meaning    |
|--------------- | --------------- |
| TempControl_ONCE_STEP | How much temperature changes when button pressed once (default 0.1°C) |
| TempControl_HOLD_STEP | How much temperature changes when button is held (default 0.5°C) |
| TempControl_SLOTS_COUNT | Number of slots |
| OutputDutyCycle_DURATION | How many iterations one output cycle takes (by default 20 seconds) |
| MenuActive_MAX | How many iterations menu will remain opened |
| MenuTempSet_FLASH_START | After this count of iterations without user input display will start to blink |
| ITERATION_DURATION | If iteration took less than this microseconds, loop will wait before next iteration |

## Control

    How to set the temperature and understand what this thing is doing

### Set temperature and slot
Number of current temperature slot is displayed for short time
when the power is turned on. After that, current temperature from sensor is shown.
Slot is pair of HIGH and LOW temperatures, so user can switch between slots instead of setting temperature every time when it needs to be changed.\
HIGH or LOW temperatures shown by short press on UP or DOWN button respectively. If button is held, temperature starts to change. Now button can be released: UP button increases temperature, DOWN decreases. After some time if no button pressed, display will start to blink, then show current temperature. This means temperature was set.
When both UP and DOWN buttons held, current slot is set up.
If this explanation sounds too confusing, maybe [the diagram](menu-flowchart.png) will help clarify this out.

### Output logic

By default high temperature set to 30 and low to 20. When HIGH temp is upper than LOW, output ON when temperature falling (this is HEATING mode), when LOW upper than HIGH, output ON when temperature rising (COOLING mode).
In range between high and low output is pulse-width modulated.
Duty cycle is 100% in the begin of range and 0% in the end.
Scale is linear. For example on default settings (range is 30°C to 20°C),
when sensor temperature is 22.5°C, output will be repeatedly
turned on for 15 seconds and off for 5 seconds.

## Notes
One Wire protocol require certain timings, but timings of
current implementation depend on many various factors.
If thermal sensor not responding, check comments in [nanoOneWire.h](lib/nanoDS18B20_C/nanoOneWire.h)

### Ported Libraries

There are two libraries, which i ported from C++ to C for this project:
[SevSeg](lib/SevSegC/SevSegC.h) from [DeanIsMe](https://github.com/DeanIsMe/SevSeg/tree/master)
and
[microDS18B20](lib/nanoDS18B20_C/nanoOneWire.h) from [AlexGyver](https://github.com/GyverLibs/microDS18B20)
