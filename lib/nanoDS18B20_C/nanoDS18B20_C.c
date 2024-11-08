#include <nanoDS18B20_C.h>
// TODO: make 2 docstrings for functions with and without parasite

// dsPin: pin which DS18B20 connected to
// `currently parasite powered connection not supported`
// `(until i buy more stable oscilloscope)`
// parasitePowered: is data pin used for power
// (check requestTemp and getTemp functions)
#ifndef nanods_NOPARASITE
void microds_init(NanoDS18B20 *device, uint8_t dsPin, bool parasitePowered)
#else
void microds_init(NanoDS18B20 *device, uint8_t dsPin)
#endif
{
    device->dsPin = dsPin;

#ifndef nanods_NOPARASITE
    device->parasitePowered = parasitePowered;
#endif

    pinMode(dsPin, INPUT);
}

#ifndef nanods_NORES
// Set resolution, valid value range: 9<=res<=12
// 9 bit -- 0.5 °С, 10 -- 0.25, 11 -- 0.125, 12 -- 0.0625
void microds_setResolution(NanoDS18B20 *device, uint8_t res)
{
    if (!oneWire_reset(device->dsPin)) // Initiate transaction
        return;

#ifndef nanods_NOPARASITE
    oneWire_write(0xCC, device->dsPin, false);                                      // SKIP ROM
    oneWire_write(0x4E, device->dsPin, false);                                      // WRITE SCRATCHPAD
    oneWire_write(0xFF, device->dsPin, false);                                      // Maximum value to Th register
    oneWire_write(0x00, device->dsPin, false);                                      // Minimum value to Tl register
    oneWire_write(((constrain(res, 9, 12) - 9) << 5) | 0x1F, device->dsPin, false); // Write configuration register
#else
    oneWire_write(0xCC, device->dsPin);
    oneWire_write(0x4E, device->dsPin);
    oneWire_write(0xFF, device->dsPin);
    oneWire_write(0x00, device->dsPin);
    oneWire_write(((constrain(res, 9, 12) - 9) << 5) | 0x1F, device->dsPin);
#endif
}
#endif

// Request temperature conversion,
// return false if transaction initiation is failed.
//
// 9 bit resolution -- 94 ms, 10 -- 188 ms, 11 -- 375 ms, 12 -- 750 ms
//
// If parasitePowered is true, pin will be left high,
// and you MUST wait for conversion period before reading temperature.
// If device has external power supply, you can poll readTemp.
bool microds_requestTemp(NanoDS18B20 *device)
{
    device->_tempUpdating = true;
    if (!oneWire_reset(device->dsPin)) // Initiate transaction
        return false;

#ifndef nanods_NOPARASITE
    oneWire_write(0xCC, device->dsPin, false);                   // SKIP ROM
    oneWire_write(0x44, device->dsPin, device->parasitePowered); // CONVERT T
#else
    oneWire_write(0xCC, device->dsPin);
    oneWire_write(0x44, device->dsPin);
#endif

    return true;
}

// Read first two registers from scratchpad (call requestTemp first)
//
// If device powered by external power source, you
// can call this function during conversion, to determine
// if sensor ready.
bool microds_readTemp(NanoDS18B20 *device)
{
#ifndef nanods_NOPARASITE
    if (!device->parasitePowered)
    {
        if (!oneWire_readBit(device->dsPin))
            return false; // Temperature conversion not done
    }
#endif

    device->_tempUpdating = false;
    if (!oneWire_reset(device->dsPin))
    {
        return false; // Sensor offline
    }

#ifndef nanods_NOPARASITE
    oneWire_write(0xCC, device->dsPin, false); // SKIP ROM
    oneWire_write(0xBE, device->dsPin, false); // READ SCRATCHPAD
#else
    oneWire_write(0xCC, device->dsPin); // SKIP ROM
    oneWire_write(0xBE, device->dsPin); // READ SCRATCHPAD
#endif

    device->_buf = oneWire_read(device->dsPin);
    device->_buf |= (oneWire_read(device->dsPin) << 8);
    return true;
}

// Get previously read temperature
// (call readTemp first)
float microds_getTemp(NanoDS18B20 *device)
{
    return (device->_buf / 16.0);
}
