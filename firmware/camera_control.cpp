/*
  The MIT License (MIT)

  Copyright (c) 2016 Ivor Wanders

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
#include "camera_control.h"

void CameraOptocoupler::photoBlocking() {
    digitalWrite(focus_pin_, HIGH);
    // Serial.print(millis());
    // Serial.println(" focus_pin_ HIGH");
    delay(focus_duration_);
    // Serial.print(millis());
    // Serial.println(" shutter_pin_ HIGH");
    digitalWrite(shutter_pin_, HIGH);
    delay(shutter_duration_);
    // Serial.print(millis());
    // Serial.print(" focus_pin_ LOW");
    // Serial.println(" shutter_duration_ LOW");
    digitalWrite(focus_pin_, LOW);
    digitalWrite(shutter_duration_, LOW);
}


void CameraOptocoupler::startPhoto() {
  duration_ = 0;
  taking_photo_ = true;
}

void CameraOptocoupler::run() {
  if (duration_ < focus_duration_) {
    // Serial.print(millis());
    // Serial.println(" focus_pin_ HIGH");
    digitalWriteFast(focus_pin_, HIGH);
  } else if (duration_ < (focus_duration_ + shutter_duration_)) {
    // Serial.print(millis());
    // Serial.println(" shutter_pin_ HIGH");
    digitalWriteFast(shutter_pin_, HIGH);
  } else if (duration_ > (focus_duration_ + shutter_duration_)) {
    // Serial.print(millis());
    // Serial.print(" focus_pin_ LOW");
    // Serial.println(" shutter_pin_ LOW");
    digitalWriteFast(focus_pin_, LOW);
    digitalWriteFast(shutter_pin_, LOW);
    taking_photo_ = false;
  }
}

bool CameraOptocoupler::finishedPhoto(){
  return !taking_photo_;
}