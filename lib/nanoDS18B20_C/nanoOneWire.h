// oneWire implementation for stm8s103f3 with default clock,
// sduino framework and SDCC
//
// Timing depends on MCU type, clock, function overheads,
// etc, so tuning delays with oscilloscope
// is highly recommended if device seems not responding.
// To do so, connect oscilloscope to pulled up pin instead
// of DS18B20. Call readBit 2 times in series and
// redefine NOP_MICROSECOND in such way that
// distance between two voltage drops became around 70 microseconds.

#ifndef _microOneWire_h
#define _microOneWire_h
#include <Arduino.h>

bool oneWire_reset(uint8_t pin);

bool oneWire_readBit(uint8_t pin);
uint8_t oneWire_read(uint8_t pin);

#ifndef nanods_NOPARASITE
void oneWire_write(uint8_t data, uint8_t pin, bool leavePowered);
#else
void oneWire_write(uint8_t data, uint8_t pin);
#endif

#endif
