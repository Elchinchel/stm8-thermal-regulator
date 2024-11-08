/* SevSeg Library
 *
 * Copyright 2020 Dean Reading
 *
 * This library allows an Arduino to easily display numbers and letters on a
 * 7-segment display without a separate 7-segment display controller.
 *
 * See the included readme for instructions.
 * https://github.com/DeanIsMe/SevSeg
 */

#include <Arduino.h>

#include <SevSegC.h>

#define BLANK_IDX 10 // Must match with 'digitCodeMap'
#define DASH_IDX 11
#define PERIOD_IDX 12

static const int32_t powersOf10[] = {
    1, // 10^0
    10,
    100,
    1000,
    10000,
    100000,
    1000000,
    10000000,
    100000000,
    1000000000}; // 10^9

// digitCodeMap indicate which segments must be illuminated to display
// each number.
static const uint8_t digitCodeMap[] = {
    // GFEDCBA  Segments      7-segment map:
    0b00111111, // 0   "0"          AAA
    0b00000110, // 1   "1"         F   B
    0b01011011, // 2   "2"         F   B
    0b01001111, // 3   "3"          GGG
    0b01100110, // 4   "4"         E   C
    0b01101101, // 5   "5"         E   C
    0b01111101, // 6   "6"          DDD
    0b00000111, // 7   "7"
    0b01111111, // 8   "8"
    0b01101111, // 9   "9"
    0b00000000, // 10  ' '  BLANK
    0b01000000, // 11  '-'  DASH
    0b10000000, // 12  '.'  PERIOD
};

void sevseg_findDigits(SevSeg *sevseg, int32_t numToShow, int8_t decPlaces, uint8_t digits[]);
void sevseg_setDigitCodes(SevSeg *sevseg, const uint8_t digits[], int8_t decPlaces);
void sevseg_setNewNum(SevSeg *sevseg, int32_t numToShow, int8_t decPlaces);
void sevseg_segmentOn(SevSeg *sevseg, uint8_t segmentNum);
void sevseg_segmentOff(SevSeg *sevseg, uint8_t segmentNum);
void sevseg_digitOn(SevSeg *sevseg, uint8_t digitNum);
void sevseg_digitOff(SevSeg *sevseg, uint8_t digitNum);

// begin
/******************************************************************************/
// Saves the input pin numbers to the class and sets up the pins to be used.
// Use current-limiting resistors on digit pins.
void sevseg_begin(
    SevSeg *sevseg,
    uint8_t numDigitsIn,
    const uint8_t digitPinsIn[],
    const uint8_t segmentPinsIn[])
{
  sevseg->prevUpdateIdx = 0;
  sevseg->numDigits = numDigitsIn;

  // Limit the max number of digits to prevent overflowing
  if (sevseg->numDigits > MAXNUMDIGITS)
  {
    sevseg->numDigits = MAXNUMDIGITS;
  }

  // Save the input pin numbers to sevseg variables
  for (uint8_t segmentNum = 0; segmentNum < NUM_SEGMENTS; segmentNum++)
  {
    sevseg->segmentPins[segmentNum] = segmentPinsIn[segmentNum];
  }

  for (uint8_t digitNum = 0; digitNum < sevseg->numDigits; digitNum++)
  {
    sevseg->digitPins[digitNum] = digitPinsIn[digitNum];
  }

  // Set the pins as outputs, and turn them off
  for (uint8_t digitNum = 0; digitNum < sevseg->numDigits; digitNum++)
  {
    pinMode(sevseg->digitPins[digitNum], OUTPUT);
    digitalWrite(sevseg->digitPins[digitNum], DIGIT_OFF_VAL);
  }

  for (uint8_t segmentNum = 0; segmentNum < NUM_SEGMENTS; segmentNum++)
  {
    pinMode(sevseg->segmentPins[segmentNum], OUTPUT);
    digitalWrite(sevseg->segmentPins[segmentNum], SEGMENT_OFF_VAL);
  }

  sevseg_blank(sevseg); // Initialise the display
}

// refreshDisplay
/******************************************************************************/
// Turns on the segments specified in 'digitCodes[]'
// For resistors on *digits* we will cycle through all 8 segments (7 + period),
//    turning on the *digits* as appropriate for a given segment, before moving on
//    to the next segment.
void sevseg_refreshDisplay(SevSeg *sevseg)
{
  /**********************************************/
  // RESISTORS ON DIGITS, UPDATE WITHOUT DELAYS

  // Turn all lights off for the previous segment
  sevseg_segmentOff(sevseg, sevseg->prevUpdateIdx);

  sevseg->prevUpdateIdx++;
  if (sevseg->prevUpdateIdx >= NUM_SEGMENTS) {
    sevseg->prevUpdateIdx = 0;
  }

  // Illuminate the required digits for the new segment
  sevseg_segmentOn(sevseg, sevseg->prevUpdateIdx);
}

// segmentOn
/******************************************************************************/
// Turns a segment on, as well as all corresponding digit pins
// (according to digitCodes[])
void sevseg_segmentOn(SevSeg *sevseg, uint8_t segmentNum)
{
  digitalWrite(sevseg->segmentPins[segmentNum], SEGMENT_ON_VAL);
  for (uint8_t digitNum = 0; digitNum < sevseg->numDigits; digitNum++)
  {
    if (sevseg->digitCodes[digitNum] & (1 << segmentNum))
    { // Check a single bit
      digitalWrite(sevseg->digitPins[digitNum], DIGIT_ON_VAL);
    }
  }
}

// segmentOff
/******************************************************************************/
// Turns a segment off, as well as all digit pins
void sevseg_segmentOff(SevSeg *sevseg, uint8_t segmentNum)
{
  for (uint8_t digitNum = 0; digitNum < sevseg->numDigits; digitNum++)
  {
    digitalWrite(sevseg->digitPins[digitNum], DIGIT_OFF_VAL);
  }
  digitalWrite(sevseg->segmentPins[segmentNum], SEGMENT_OFF_VAL);
}

// digitOn
/******************************************************************************/
// Turns a digit on, as well as all corresponding segment pins
// (according to digitCodes[])
void sevseg_digitOn(SevSeg *sevseg, uint8_t digitNum)
{
  digitalWrite(sevseg->digitPins[digitNum], DIGIT_ON_VAL);
  for (uint8_t segmentNum = 0; segmentNum < NUM_SEGMENTS; segmentNum++)
  {
    if (sevseg->digitCodes[digitNum] & (1 << segmentNum))
    { // Check a single bit
      digitalWrite(sevseg->segmentPins[segmentNum], SEGMENT_ON_VAL);
    }
  }
}

// digitOff
/******************************************************************************/
// Turns a digit off, as well as all segment pins
void sevseg_digitOff(SevSeg *sevseg, uint8_t digitNum)
{
  for (uint8_t segmentNum = 0; segmentNum < NUM_SEGMENTS; segmentNum++)
  {
    digitalWrite(sevseg->segmentPins[segmentNum], SEGMENT_OFF_VAL);
  }
  digitalWrite(sevseg->digitPins[digitNum], DIGIT_OFF_VAL);
}

// setNumber
/******************************************************************************/
// Receives an integer and passes it to 'setNewNum'.
void sevseg_setNumber(SevSeg *sevseg, int32_t numToShow, int8_t decPlaces)
{ // int32_t
  sevseg_setNewNum(sevseg, numToShow, decPlaces);
}

// setNumberF
/******************************************************************************/
// Receives a float, prepares it, and passes it to 'setNewNum'.
void sevseg_setNumberF(SevSeg *sevseg, float numToShow, int8_t decPlaces)
{ // float
  int8_t decPlacesPos = constrain(decPlaces, 0, MAXNUMDIGITS);
  numToShow = numToShow * powersOf10[decPlacesPos];
  // Modify the number so that it is rounded to an integer correctly
  numToShow += (numToShow >= 0.f) ? 0.5f : -0.5f;
  sevseg_setNewNum(sevseg, (int32_t)numToShow, (int8_t)decPlaces);
}

// setNewNum
/******************************************************************************/
// Changes the number that will be displayed.
void sevseg_setNewNum(SevSeg *sevseg, int32_t numToShow, int8_t decPlaces)
{
  uint8_t digits[MAXNUMDIGITS];
  sevseg_findDigits(sevseg, numToShow, decPlaces, digits);
  sevseg_setDigitCodes(sevseg, digits, decPlaces);
}

// blank
/******************************************************************************/
void sevseg_blank(SevSeg *sevseg)
{
  for (uint8_t digitNum = 0; digitNum < sevseg->numDigits; digitNum++)
  {
    sevseg->digitCodes[digitNum] = digitCodeMap[BLANK_IDX];
  }
  sevseg_segmentOff(sevseg, 0);
  sevseg_digitOff(sevseg, 0);
}

// findDigits
/******************************************************************************/
// Decides what each digit will display.
// Enforces the upper and lower limits on the number to be displayed.
// digits[] is an output
void sevseg_findDigits(SevSeg *sevseg, int32_t numToShow, int8_t decPlaces, uint8_t digits[])
{
  const int32_t maxNum = powersOf10[sevseg->numDigits] - 1;
  const int32_t minNum = -(powersOf10[sevseg->numDigits - 1] - 1);

  // If the number is out of range, just display dashes
  if (numToShow > maxNum || numToShow < minNum)
  {
    for (uint8_t digitNum = 0; digitNum < sevseg->numDigits; digitNum++)
    {
      digits[digitNum] = DASH_IDX;
    }
  }
  else
  {
    uint8_t digitNum = 0;

    // Convert all number to positive values
    if (numToShow < 0)
    {
      digits[0] = DASH_IDX;
      digitNum = 1; // Skip the first iteration
      numToShow = -numToShow;
    }

    // Find all digits for base's representation, starting with the most
    // significant digit
    for (; digitNum < sevseg->numDigits; digitNum++)
    {
      int32_t factor = powersOf10[sevseg->numDigits - 1 - digitNum];
      digits[digitNum] = numToShow / factor;
      numToShow -= digits[digitNum] * factor;
    }

    // Find unnnecessary leading zeros and set them to BLANK
    if (decPlaces < 0) {
      decPlaces = 0;
    }

    for (digitNum = 0; digitNum < (sevseg->numDigits - 1 - decPlaces); digitNum++)
    {
      if (digits[digitNum] == 0)
      {
        digits[digitNum] = BLANK_IDX;
      }
      // Exit once the first non-zero number is encountered
      else if (digits[digitNum] <= 9)
      {
        break;
      }
    }
  }
}

// setDigitCodes
/******************************************************************************/
// Sets the 'digitCodes' that are required to display the input numbers
void sevseg_setDigitCodes(SevSeg *sevseg, const uint8_t digits[], int8_t decPlaces)
{

  // Set the digitCode for each digit in the display
  for (uint8_t digitNum = 0; digitNum < sevseg->numDigits; digitNum++)
  {
    sevseg->digitCodes[digitNum] = digitCodeMap[digits[digitNum]];
    // Set the decimal point segment
    if (decPlaces >= 0)
    {
      if (digitNum == sevseg->numDigits - 1 - decPlaces)
      {
        sevseg->digitCodes[digitNum] |= digitCodeMap[PERIOD_IDX];
      }
    }
  }
}

/// END ///
