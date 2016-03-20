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
#include "./component_test.h"



void testMotor() {
  MotorStepper motor_stepper;


  // begin the stepper with the correct pins
  motor_stepper.begin(MOTOR_DIRECTION_PIN,
                      MOTOR_STEPS_PIN);

  // set the widths of pulses and ramp length
  motor_stepper.setMinWidth(1000);
  motor_stepper.setMaxWidth(4000);
  motor_stepper.setRampLength(100);


  // if we use the MotorSteppersBackground class to call the motor, add it.
  #ifdef MOTOR_STEPPER_BACKGROUND_USED
  MotorSteppersBackground.addMotor(&motor_stepper);
  MotorSteppersBackground.setInterval(MOTOR_STEPPER_INTERVAL);
  #endif

  uint32_t to_go = 0;
  elapsedMillis printTimer = 0;
  int16_t distance = 500;
  while (1) {
    #ifndef MOTOR_STEPPER_BACKGROUND_USED
    motor_stepper.run();  // call isr by hand if not using background stepper.
    #endif
    if (printTimer > 50) {
      printTimer = 0;
      to_go = motor_stepper.stepsToGo();
      Serial.print("To go: ");
      Serial.println(to_go);
      if (to_go == 0) {
        #ifdef MOTOR_STEPPER_BACKGROUND_USED
          Serial.println("Using MotorSteppersBackground to move the motor!");
        #endif
        delay(1000);
        motor_stepper.move(distance);
        distance = -distance;
      }
    }
  }
}


void testCamera() {
  CameraOptocoupler camera;
  camera.begin(CAMERA_FOCUS_PIN, CAMERA_SHUTTER_PIN);
  camera.setFocusDuration(1000);
  camera.setShutterDuration(1000);
  while (1) {
    Serial.print(millis()); Serial.println(" blocking:");
    camera.photoBlocking();
    delay(1000);
    Serial.print(millis()); Serial.println(" nonblocking:");
    camera.startPhoto();
    while (!camera.finishedPhoto()) {
      delay(100);
      camera.run();
    }
  }
}

void testStacking() {
  delay(1000);
  Serial.println("Setup of motor.");
  MotorStepper motor_stepper;

  // begin the stepper with the correct pins
  motor_stepper.begin(MOTOR_DIRECTION_PIN,
                      MOTOR_STEPS_PIN);

  // set the widths of pulses and ramp length
  motor_stepper.setMinWidth(1000);
  motor_stepper.setMaxWidth(4000);
  motor_stepper.setRampLength(100);


  Serial.println("Setup of camera.");
  CameraOptocoupler camera;
  camera.begin(CAMERA_FOCUS_PIN, CAMERA_SHUTTER_PIN);
  camera.setFocusDuration(1000);
  camera.setShutterDuration(1000);

  Serial.println("Setup of stacker.");
  StackControl stacker;

  stacker.setMotor(&motor_stepper);
  stacker.setCamera(&camera);

  stacker.setDelayBeforePhoto(0);
  stacker.setDelayAfterPhoto(0);

  stacker.setMoveSteps(100);
  stacker.setStackCount(5);


  Serial.println("Start stacking.");
  delay(1000);
  stacker.stack();
  while (!stacker.isStackFinished()) {
    stacker.run();
  }
  Serial.println("Done stacking.");
}
