#include "nanoOneWire.h"

#ifndef NOP_MICROSECOND
#define NOP_MICROSECOND() \
    nop();                \
    nop();                \
    nop();                \
    nop();                \
    nop();                \
    nop();                \
    nop();                \
    nop()
#endif

#ifndef __ow_delay_us
#define __ow_delay_us_used uint16_t _i_left
#define __ow_delay_us(count) \
    _i_left = count;         \
    while (_i_left--)        \
    {                        \
        NOP_MICROSECOND();   \
    }
#endif

// Pull data line low and see if device will do the same in response
bool oneWire_reset(uint8_t pin)
{
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    // delayMicroseconds overhead is around 30us,
    // so in total time more than 480us
    // (it uses less memory, so i use it where timing not critical)
    delayMicroseconds(450);

    pinMode(pin, INPUT);
    __ow_delay_us_used;
    __ow_delay_us(65);
    bool devicePulledLow = !digitalRead(pin);
    delayMicroseconds(400);

    return devicePulledLow;
}

// If leavePowered is true, set high before exit
// (for CONVERT T and COPY SCRATCHPAD commands)
#ifndef nanods_NOPARASITE
void oneWire_write(uint8_t data, uint8_t pin, bool leavePowered)
#else
void oneWire_write(uint8_t data, uint8_t pin)
#endif
{
    __ow_delay_us_used;
    digitalWrite(pin, LOW);
    for (uint8_t i = 8; i; i--)
    {
        pinMode(pin, OUTPUT);
        if (data & 1)
        {
#ifndef nanods_NOPARASITE
            NOP_MICROSECOND();
            if (i != 1 || !leavePowered)
            {
                pinMode(pin, INPUT);
                delayMicroseconds(40);
            }
            else
            {
                digitalWrite(pin, HIGH);
            }
#else
            NOP_MICROSECOND();
            pinMode(pin, INPUT);
            delayMicroseconds(40);
#endif
        }
        else
        {
            delayMicroseconds(40);
            pinMode(pin, INPUT);
        }
        data >>= 1;
        __ow_delay_us(5);
    }
}

// Do 1 READ time slot
bool oneWire_readBit(uint8_t pin)
{
    __ow_delay_us_used;

    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
    __ow_delay_us(2);
    pinMode(pin, INPUT);

    bool resp = digitalRead(pin);
    __ow_delay_us(40);
    return resp;
}

// Do 8 READ time slots
uint8_t oneWire_read(uint8_t pin)
{
    uint8_t data = 0;
    for (uint8_t i = 8; i; i--)
    {
        data >>= 1;
        if (oneWire_readBit(pin))
            data |= (1 << 7);
    }
    return data;
}