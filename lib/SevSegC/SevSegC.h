// This is C port with reduced functionality.
// There is support for displaying integers and floats only.

/* SevSeg Library
 *
 * Copyright 2020 Dean Reading
 *
 * This library allows an Arduino to easily display numbers on a
 * 7-segment display without a separate 7-segment display controller.
 *
 * See the included readme for instructions.
 * https://github.com/DeanIsMe/SevSeg
 */

#ifndef MAXNUMDIGITS
#define MAXNUMDIGITS 8 // Can be increased, but the max number is 2^31
#endif

#ifndef NUM_SEGMENTS
// Define 7 for display without dot/period
// (if length of segmentPins array is 7)
#define NUM_SEGMENTS 8
#endif

#ifndef DIGIT_ON_VAL
#define DIGIT_ON_VAL HIGH
#endif

#ifndef SEGMENT_ON_VAL
#define SEGMENT_ON_VAL HIGH
#endif

#ifndef DIGIT_OFF_VAL
#define DIGIT_OFF_VAL LOW
#endif

#ifndef SEGMENT_OFF_VAL
#define SEGMENT_OFF_VAL LOW
#endif

#ifndef SevSeg_h
#define SevSeg_h

#include "Arduino.h"

typedef struct SevSeg
{
  uint8_t digitPins[MAXNUMDIGITS];
  uint8_t segmentPins[NUM_SEGMENTS];
  uint8_t numDigits;

  uint8_t prevUpdateIdx;            // The previously updated segment or digit
  uint8_t digitCodes[MAXNUMDIGITS]; // The active setting of each segment of each digit
} SevSeg;

void sevseg_begin(SevSeg *sevseg,
                  uint8_t numDigitsIn,
                  const uint8_t digitPinsIn[],
                  const uint8_t segmentPinsIn[]);

void sevseg_refreshDisplay(SevSeg *sevseg);

void sevseg_setNumber(SevSeg *sevseg, int32_t numToShow, int8_t decPlaces);
void sevseg_setNumberF(SevSeg *sevseg, float numToShow, int8_t decPlaces);

void sevseg_blank(SevSeg *sevseg);

#endif // SevSeg_h
