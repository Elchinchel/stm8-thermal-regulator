/*
    Small version of https://github.com/GyverLibs/NanoDS18B20 ported to C

    Supports only one sensor on data pin.
    Has no CRC data validation.

    Original authors:
    Egor 'Nich1con' Zakharov & AlexGyver, alex@alexgyver.ru
    MIT License
*/

#ifndef nanoDS18B20_h
#define nanoDS18B20_h

#include <Arduino.h>
#include "nanoOneWire.h"

typedef struct NanoDS18B20
{
    char dsPin;

// TODO: document NO* memory saving
#ifndef nanods_NOPARASITE
    bool parasitePowered;
#endif

    int16_t _buf;
    uint8_t _tempUpdating;
} NanoDS18B20;

#ifndef nanods_NOPARASITE
void microds_init(NanoDS18B20 *device, uint8_t dsPin, bool parasitePowered);
#else
void microds_init(NanoDS18B20 *device, uint8_t dsPin);
#endif

#ifndef nanods_NORES
void microds_setResolution(NanoDS18B20 *device, uint8_t res);
#endif

bool microds_requestTemp(NanoDS18B20 *device);
bool microds_readTemp(NanoDS18B20 *device);
float microds_getTemp(NanoDS18B20 *device);

#endif
