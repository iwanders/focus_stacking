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


// Test the motor by moving it forward and backwards
void testMotor() {
  MotorStepper motor_stepper;

  // begin the stepper with the correct pins
  motor_stepper.begin(MOTOR_DIRECTION_PIN, MOTOR_STEPS_PIN);

  // set the widths of pulses and ramp length
  motor_stepper.setMinWidth(MOTOR_DEFAULT_MIN_WIDTH);
  motor_stepper.setMaxWidth(MOTOR_DEFAULT_MAX_WIDTH);
  motor_stepper.setRampLength(MOTOR_DEFAULT_RAMP_LENGTH);


  // if we use the MotorSteppersBackground class to call the motor, add it.
  #ifdef MOTOR_STEPPER_BACKGROUND_USED
  MotorSteppersBackground.addMotor(&motor_stepper);
  MotorSteppersBackground.setInterval(MOTOR_STEPPER_BACKGROUND_INTERVAL);
  #endif

  uint32_t to_go = 0;
  elapsedMillis printTimer = 0;
  int16_t distance = 500;
  while (true) {
    #ifndef MOTOR_STEPPER_BACKGROUND_USED
    motor_stepper.run();  // Call isr by hand if not using background stepper.
    #endif
    if (printTimer > 50) {  // Every 50 milliseconds print & check movement.
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

// Tests the camera functionality.
void testCamera() {
  CameraOptocoupler camera;
  camera.begin(CAMERA_FOCUS_PIN, CAMERA_SHUTTER_PIN);

  // set durations
  camera.setFocusDuration(CAMERA_DEFAULT_FOCUS_DURATION);
  camera.setShutterDuration(CAMERA_DEFAULT_SHUTTER_DURATION);

  while (true) {
    Serial.print(millis()); Serial.println(" blocking:");
    camera.photoBlocking();  // this one is boring.
    delay(1000);
    Serial.print(millis()); Serial.println(" nonblocking:");
    camera.startPhoto();  // initialise the taking of the photo
    while (!camera.finishedPhoto()) {  // while not finished
      delay(100);
      camera.run();  // perform the button pressess
    }
  }
}

// Test the stack sequence...
void testStacking() {
  delay(1000);
  Serial.println("Setup of motor.");
  MotorStepper motor_stepper;

  // begin the stepper with the correct pins
  motor_stepper.begin(MOTOR_DIRECTION_PIN,
                      MOTOR_STEPS_PIN);

  // set the widths of pulses and ramp length
  motor_stepper.setMinWidth(MOTOR_DEFAULT_MIN_WIDTH);
  motor_stepper.setMaxWidth(MOTOR_DEFAULT_MAX_WIDTH);
  motor_stepper.setRampLength(MOTOR_DEFAULT_RAMP_LENGTH);

  #ifdef MOTOR_STEPPER_BACKGROUND_USED
  MotorSteppersBackground.addMotor(&motor_stepper);
  MotorSteppersBackground.setInterval(MOTOR_STEPPER_BACKGROUND_INTERVAL);
  #endif

  Serial.println("Setup of camera.");
  CameraOptocoupler camera;
  camera.begin(CAMERA_FOCUS_PIN, CAMERA_SHUTTER_PIN);
  camera.setFocusDuration(CAMERA_DEFAULT_FOCUS_DURATION);
  camera.setShutterDuration(CAMERA_DEFAULT_SHUTTER_DURATION);

  Serial.println("Setup of stacker.");
  StackControl stacker;

  stacker.setMotor(&motor_stepper);
  stacker.setCamera(&camera);

  stacker.setDelayBeforePhoto(STACK_DEFAULT_DELAY_BEFORE_PHOTO);
  stacker.setDelayAfterPhoto(STACK_DEFAULT_DELAY_AFTER_PHOTO);

  stacker.setMoveSteps(STACK_DEFAULT_MOVE_STEPS);
  stacker.setStackCount(STACK_DEFAULT_STACK_COUNT);


  Serial.println("Start stacking.");
  delay(1000);
  stacker.stack();  // start a stack sequence
  while (!stacker.isStackFinished()) {  // while not finished
    stacker.run();  // do the things necessary to create photos and move.
  }
  Serial.println("Done stacking.");
}


void testInterface() {
  delay(3000);

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

  // Serial.print("Sizeof msg_t: ");
  // Serial.println(sizeof(StackInterface::msg_t));

  // Serial.print("Sizeof msg_config_t: ");
  // Serial.println(sizeof(StackInterface::msg_config_t));

  // Serial.print("Sizeof msg_type: ");
  // Serial.println(sizeof(StackInterface::msg_type));

  // Serial.println();
  while (true) {
    interface.run();
  }
}
