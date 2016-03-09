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
#include "Arduino.h"
#include "motor_stepper.h"

#define MOTOR_DIRECTION_PIN 15
#define MOTOR_STEPS_PIN 16
#define MOTOR_STEPPER_INTERVAL 200

void testMotor() {
  uint32_t togo;
  elapsedMillis printTimer = 0;
  int16_t distance = 500;
  while (1) {
    if (printTimer > 20) {
      printTimer = 0;
      togo = motor_stepper.stepsToGo();
      Serial.print("Togo: ");
      Serial.println(togo);
      if (togo == 0) {
        delay(1000);
        motor_stepper.move(distance);
        distance = -distance;
      }
    }
  }
}



extern "C" int main(void) {
  Serial.begin(9600);

  // begin the stepper with the correct pins
  motor_stepper.begin(MOTOR_DIRECTION_PIN,
                      MOTOR_STEPS_PIN,
                      MOTOR_STEPPER_INTERVAL);

  motor_stepper.setMinWidth(1000);
  motor_stepper.setMaxWidth(4000);
  motor_stepper.setRampLength(100);

  testMotor();
}
