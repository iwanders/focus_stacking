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
#include "./config.h"
#include "./stack_control.h"
#include "./stack_interface.h"
#include "./component_test.h"

extern "C" int main(void) {
  Serial.begin(9600);
  // testMotor();
  // testCamera();
  // testStacking();
  testInterface();

  MotorStepper motor_stepper;
  StackControl stacker;
  CameraOptocoupler camera;
  StackInterface interface;

  // add components to the stacker
  stacker.setMotor(&motor_stepper);
  stacker.setCamera(&camera);

  // add all the components to the interface
  interface.setMotor(&motor_stepper);
  interface.setCamera(&camera);
  interface.setStacker(&stacker);

  // begin the stepper with the correct pins
  motor_stepper.begin(MOTOR_DIRECTION_PIN, MOTOR_STEPS_PIN);

  // set the widths of pulses and ramp length
  motor_stepper.setMinWidth(MOTOR_DEFAULT_MIN_WIDTH);
  motor_stepper.setMaxWidth(MOTOR_DEFAULT_MAX_WIDTH);
  motor_stepper.setRampLength(MOTOR_DEFAULT_RAMP_LENGTH);

  #ifdef MOTOR_STEPPER_BACKGROUND_USED
  MotorSteppersBackground.addMotor(&motor_stepper);
  MotorSteppersBackground.setInterval(MOTOR_STEPPER_INTERVAL);
  #endif

  // initialise the camera
  camera.begin(CAMERA_FOCUS_PIN, CAMERA_SHUTTER_PIN);
  camera.setFocusDuration(CAMERA_DEFAULT_FOCUS_DURATION);
  camera.setShutterDuration(CAMERA_DEFAULT_SHUTTER_DURATION);


  stacker.setDelayBeforePhoto(STACK_DEFAULT_DELAY_BEFORE_PHOTO);
  stacker.setDelayAfterPhoto(STACK_DEFAULT_DELAY_AFTER_PHOTO);

  stacker.setMoveSteps(STACK_DEFAULT_MOVE_STEPS);
  stacker.setStackCount(STACK_DEFAULT_STACK_COUNT);

  interface.setStatusInterval(INTERFACE_DEFAULT_STATUS_INTERVAL);

  while (true) {
    stacker.run();
    interface.run();
  }
}
