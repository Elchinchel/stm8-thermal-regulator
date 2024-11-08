#include <EEPROM.h>
#include <Arduino.h>
#include <SevSegC.h>
#include <nanoDS18B20_C.h>

typedef struct Button
{
  uint8_t pin;
  uint8_t timer;
  uint8_t counter;

  bool changed; // set by readButton, unset by handleButtonClick
} Button;

typedef struct ButtonClick
{
  bool once;
  bool hold;
  bool pressed;
} ButtonClick;

typedef struct TempControlSlot
{
  int16_t low;
  int16_t high;
} TempControlSlot;

SevSeg display;
NanoDS18B20 tempSensor;

uint8_t outputPin = 3;
uint8_t buttonUpPin = 1;
uint8_t buttonDownPin = 0;
uint8_t tempSensorPin = 2;
uint8_t digitPins[3] = {15, 14, 13};
uint8_t segmentPins[8] = {11, 12, 8, 6, 5, 10, 9, 7};

#define TempUpdate_READY 0
#define TempUpdate_TIMEOUT 10 // specified in iterations
uint8_t tempUpdateStep;
float tempUpdatePrev;

// temperature defined as temp*10
#define TempControl_ONCE_STEP 1
#define TempControl_HOLD_STEP 5
#define TempControl_SLOTS_COUNT 6
#define TempControl_MAX_TEMP 1000
#define TempControl_MIN_TEMP -400
#define TempControl_DEFAULT_HIGH 300
#define TempControl_DEFAULT_LOW 200
#define TempControl_EEPROM_CURRENT_SLOT_ADDR 0
#define TempControl_EEPROM_SLOTS_ADDR 10
uint8_t tempControlCurrentSlot;
TempControlSlot tempControlSlots[TempControl_SLOTS_COUNT];

// THRESHOLD is how much iterations button pin should be
// high to be considered pressed. Higher values
// cause higher input lag, but better
#define Button_DELTA 5
#define Button_THRESHOLD 15
Button buttonUp;
Button buttonDown;

#define OutputLevel_ON LOW // assume PNP transistor on output pin
#define OutputLevel_OFF HIGH
#define OutputDutyCycle_DURATION 10000 // in iterations
// This was not implemented cause lack of flash memory
// #define OutputDutyCycle_MAX 1
// #define OutputDutyCycle_MIN 0
float outputHighCycleDuration;

#define MenuState_DEFAULT 0
#define MenuState_SHOW_TEMP 1
#define MenuState_SET_LOW 11
#define MenuState_SET_HIGH 12
#define MenuState_SET_SLOT 20
#define MenuActive_MAX 120
#define MenuTempSet_FLASH_START 75
#define MenuTempSet_FLASH_DELAY 15
uint8_t menuState;
uint8_t menuActiveCounter;

#define ITERATION_DURATION 200
uint32_t currentIteration;
uint64_t prevIterationStart;

float numberOnDisplay;

void initSlots();
TempControlSlot *currentTempSlot();
void displayNumber(float value, bool integer);

void setup()
{
  tempUpdatePrev = 0;
  tempUpdateStep = TempUpdate_READY;

  EEPROM_get(TempControl_EEPROM_CURRENT_SLOT_ADDR, tempControlCurrentSlot);
  EEPROM_get(TempControl_EEPROM_SLOTS_ADDR, tempControlSlots);

  if (tempControlCurrentSlot >= TempControl_SLOTS_COUNT)
    tempControlCurrentSlot = 0;

  uint8_t currentSlot = tempControlCurrentSlot;
  initSlots();
  tempControlCurrentSlot = currentSlot;

  buttonUp.pin = buttonUpPin;
  buttonUp.timer = 0;
  buttonUp.counter = 0;
  buttonUp.changed = false;
  buttonDown.pin = buttonDownPin;
  buttonDown.timer = 0;
  buttonDown.counter = 0;
  buttonDown.changed = false;

  menuState = MenuState_DEFAULT;
  menuActiveCounter = 0;

  outputHighCycleDuration = 0;

  currentIteration = 1;

  pinMode(outputPin, OUTPUT_OD);
  digitalWrite(outputPin, OutputLevel_OFF);

  pinMode(buttonUpPin, INPUT_PULLUP);
  pinMode(buttonDownPin, INPUT_PULLUP);

  sevseg_begin(
      &display,
      3,
      digitPins,
      segmentPins);
  microds_init(&tempSensor, tempSensorPin);

  displayNumber(tempControlCurrentSlot + 1, true);
}

// if all slots is zero, set them to default
// if any slot out of max and min, fix that
void initSlots()
{
  for (tempControlCurrentSlot = 0; tempControlCurrentSlot < TempControl_SLOTS_COUNT; tempControlCurrentSlot++)
  {
    TempControlSlot *slot = currentTempSlot();
    if (slot->high != 0 || slot->low != 0)
      return;
  }

  for (tempControlCurrentSlot = 0; tempControlCurrentSlot < TempControl_SLOTS_COUNT; tempControlCurrentSlot++)
  {
    TempControlSlot *slot = currentTempSlot();
    slot->high = TempControl_DEFAULT_HIGH;
    slot->low = TempControl_DEFAULT_LOW;
  }
}

bool isNthIteration(uint32_t n)
{
  return (currentIteration % n == 0);
}

TempControlSlot *currentTempSlot()
{
  TempControlSlot *slot = &tempControlSlots[tempControlCurrentSlot];

  // // there is should be check for low too
  // if (!(TempControl_MAX_TEMP > slot->high > TempControl_MIN_TEMP))
  // {
  //   slot->high = TempControl_DEFAULT_HIGH;
  //   slot->low = TempControl_DEFAULT_LOW;
  // }

  return slot;
}

void displayNumber(float value, bool integer)
{
  if (value == numberOnDisplay)
    return;

  numberOnDisplay = value;
  if (value > -9.9 && !integer)
  {
    sevseg_setNumberF(&display, value, 1);
  }
  else
  {
    sevseg_setNumber(&display, (int32_t)value, -1);
  }
}

void displayBlank()
{
  if (numberOnDisplay == -1111)
    return;
  numberOnDisplay = -1111;
  sevseg_blank(&display);
}

void displayFlashingNumber(float value, bool integer)
{
  if (
      menuActiveCounter <= (MenuTempSet_FLASH_START) &&
      menuActiveCounter % (MenuTempSet_FLASH_DELAY * 2) <= MenuTempSet_FLASH_DELAY)
  {
    displayBlank();
  }
  else
  {
    displayNumber(value, integer);
  }
}

void displayTemperature()
{
  displayNumber(tempUpdatePrev, false);
}

void updateTemperature()
{
  if (tempUpdateStep == TempUpdate_READY)
  {
    microds_requestTemp(&tempSensor);
    tempUpdateStep++;
    return;
  }

  tempUpdateStep++;
  if (tempUpdateStep > TempUpdate_TIMEOUT)
  {
    tempUpdateStep = TempUpdate_READY;
    return;
  }

  if (!microds_readTemp(&tempSensor))
    return;

  tempUpdateStep = false;
  float temp = microds_getTemp(&tempSensor);

  TempControlSlot *slot = currentTempSlot();

  // if range is zero or greater,
  // output will be turned OFF when temp rises (heating mode)
  // if range is less than zero,
  // output will be turned ON when temp rises (cooling mode)
  float diff = (slot->low / 10.0) - temp;
  float range = (slot->high - slot->low) / 10.0;
  float outputDutyCycle = 1 + (diff / range);
  outputHighCycleDuration = outputDutyCycle * OutputDutyCycle_DURATION;

  tempUpdatePrev = temp;
  displayTemperature();
}

void readButton(Button *button)
{
  bool btnPressed = !digitalRead(button->pin);
  if (btnPressed)
  {
    if (button->counter < Button_THRESHOLD + Button_DELTA)
      button->counter++;

    if (button->counter == Button_THRESHOLD)
    {
      if (menuActiveCounter == 0)
        menuActiveCounter++;
      button->changed = true;
      button->counter = Button_THRESHOLD + Button_DELTA;
    }
  }
  else
  {
    if (button->counter > 0)
      button->counter--;

    if (button->counter == Button_THRESHOLD - Button_DELTA)
    {
      button->changed = true;
      button->counter = 0;
    }
  }
}

void handleButtonClick(Button *button, ButtonClick *click)
{
  click->pressed = (button->counter > Button_THRESHOLD);
  if (button->changed)
  {
    button->changed = false;
    button->timer = 0;
    if (click->pressed)
      click->once = true;
  }
  else
  {
    if (click->pressed)
    {
      if (isNthIteration(10))
        button->timer++;
      if (button->timer > 125)
      {
        button->timer = 100;
        click->hold = true;
      }
    }
  }
}

void displayMenu_setTemperature(ButtonClick *upClick, ButtonClick *downClick, int16_t *temp)
{
  if (upClick->once)
    *temp += TempControl_ONCE_STEP;
  if (downClick->once)
    *temp -= TempControl_ONCE_STEP;

  if (upClick->hold)
    *temp += TempControl_HOLD_STEP;
  if (downClick->hold)
    *temp -= TempControl_HOLD_STEP;

  if (*temp > TempControl_MAX_TEMP)
    *temp = TempControl_MAX_TEMP;
  if (*temp < TempControl_MIN_TEMP)
    *temp = TempControl_MIN_TEMP;

  displayFlashingNumber(*temp / 10.0, false);

  if (menuActiveCounter == 1)
    EEPROM_put(10, tempControlSlots);
}

void displayMenu_setSlot(ButtonClick *upClick, ButtonClick *downClick)
{
  if (upClick->once || upClick->hold)
    tempControlCurrentSlot += 1;

  if (downClick->once || downClick->hold)
    tempControlCurrentSlot -= (tempControlCurrentSlot > 0 ? 1 : sizeof(tempControlCurrentSlot) - TempControl_SLOTS_COUNT);

  if (tempControlCurrentSlot >= TempControl_SLOTS_COUNT)
    tempControlCurrentSlot = 0;

  displayFlashingNumber(tempControlCurrentSlot + 1, true);

  if (menuActiveCounter == 1)
    EEPROM_put(TempControl_EEPROM_CURRENT_SLOT_ADDR, tempControlCurrentSlot);
}

void displayMenu_dispatcher()
{
  ButtonClick upClick = {false, false, false};
  ButtonClick downClick = {false, false, false};
  handleButtonClick(&buttonUp, &upClick);
  handleButtonClick(&buttonDown, &downClick);

  if (!upClick.once && !downClick.once && !upClick.hold && !downClick.hold)
  {
    if (isNthIteration(100) && menuActiveCounter > 0)
      menuActiveCounter--;
    if (menuActiveCounter == 0)
    {
      menuState = MenuState_DEFAULT;
      displayTemperature();
      return;
    }
  }
  else
  {
    menuActiveCounter = MenuActive_MAX;
  }

  if (menuState == MenuState_SET_HIGH)
  {
    displayMenu_setTemperature(&upClick, &downClick, &(currentTempSlot()->high));
    return;
  }
  if (menuState == MenuState_SET_LOW)
  {
    displayMenu_setTemperature(&upClick, &downClick, &(currentTempSlot()->low));
    return;
  }
  if (menuState == MenuState_SET_SLOT)
  {
    displayMenu_setSlot(&upClick, &downClick);
    return;
  }

  if (upClick.pressed && downClick.pressed && menuState == MenuState_SHOW_TEMP)
  {
    menuState = MenuState_SET_SLOT;
    return;
  }

  if (upClick.once)
  {
    displayNumber(currentTempSlot()->high / 10.0, false);
    menuState = MenuState_SHOW_TEMP;
  }
  else if (downClick.once)
  {
    displayNumber(currentTempSlot()->low / 10.0, false);
    menuState = MenuState_SHOW_TEMP;
  }
  else if (upClick.hold)
  {
    menuState = MenuState_SET_HIGH;
  }
  else if (downClick.hold)
  {
    menuState = MenuState_SET_LOW;
  }
}

void loop()
{
  // this isn't precise and don't had to be
  int16_t sleepTime = ITERATION_DURATION - (micros() - prevIterationStart);
  if (sleepTime > 20)
  {
    delayMicroseconds(sleepTime);
  }
  prevIterationStart = micros();
  currentIteration++;

  if (isNthIteration(5))
  {
    sevseg_refreshDisplay(&display);
  }

  // do nothing (display all segments) on startup or move on overflow
  if (currentIteration < 1000)
  {
    if (currentIteration == 0)
      currentIteration = 1000;
    return;
  }

  readButton(&buttonUp);
  readButton(&buttonDown);

  if (menuActiveCounter > 0)
  {
    displayMenu_dispatcher();
  }
  else if (isNthIteration(1000)) // every 200ms
  {
    updateTemperature();
  }

  if (isNthIteration(10))
  {
    uint32_t cycleIteration = currentIteration % OutputDutyCycle_DURATION;
    bool outputOn = (cycleIteration < outputHighCycleDuration);
    digitalWrite(outputPin, outputOn ? OutputLevel_ON : OutputLevel_OFF);
  }
}
