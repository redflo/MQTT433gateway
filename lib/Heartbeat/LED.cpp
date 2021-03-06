/*
  Heartbeat - Library for flashing LED
  Project home: https://github.com/puuu/MQTT433gateway/

  The MIT License (MIT)

  Copyright (c) 2016 Puuu

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <Arduino.h>

#include "LED.h"

static inline uint8_t _pin_val(const bool value) {
  return ((value) ? HIGH : LOW);
}

LED::LED(uint8_t pin, bool activeHigh) {
  _pin = pin;
  pinMode(_pin, OUTPUT);
  _activeHigh = activeHigh;
  off();
}

void LED::on() { digitalWrite(_pin, _pin_val(_activeHigh)); }

void LED::off() { digitalWrite(_pin, _pin_val(!_activeHigh)); }

void LED::toggle() {
  if (getState()) {
    off();
  } else {
    on();
  }
}

void LED::setState(bool state) {
  if (state) {
    on();
  } else {
    off();
  }
}

bool LED::getState() { return digitalRead(_pin) == _activeHigh; }

LED::~LED() { pinMode(_pin, INPUT); }

LEDOpenDrain::LEDOpenDrain(uint8_t pin) : LED(pin) {}

void LEDOpenDrain::on() {
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);  // open drain is always active low
  _state = true;
}

void LEDOpenDrain::off() {
  pinMode(_pin, INPUT_PULLUP);
  _state = false;
}

bool LEDOpenDrain::getState() { return _state; }
