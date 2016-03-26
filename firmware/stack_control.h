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
#ifndef FIRMWARE_STACK_CONTROL_H_
#define FIRMWARE_STACK_CONTROL_H_

#include "Arduino.h"
#include "./motor_control.h"
#include "./camera_control.h"


// #define STACK_CONTROL_DEBUG
#ifdef STACK_CONTROL_DEBUG
  #define SCDBG(a) Serial.print(a);
  #define SCDBGln(a) SCDBG(a); SCDBG(" at: "); Serial.println(micros());
#else
  #define SCDBG(a)
  #define SCDBGln(a)
#endif


/*

  The order of actions is
    1. Delay before photo
    2. Photo
    3. Delay after photo
    4. Movement
    -> move to 1.

  This is the most natural because it starts with a photo at the current
  position and guarantees the delay before the photo while vibrations dampen
  out.
*/

class StackControl{
 public:
  typedef struct {
    uint32_t stack_count;
    uint32_t delay_before_photo;
    uint32_t delay_after_photo;
    int32_t move_steps;
  } config_t;

  enum sub_state {
      start_delay_before_photo = 0,
      delay_before_photo = 1,
      start_photo = 2,
      photo = 3,
      pause_after_photo = 4,
      start_delay_after_photo = 5,
      delay_after_photo = 6,
      start_movement = 7,
      movement = 8,
      pause_after_movement = 9,
      next_step = 10
    };

  enum state {
      halted = 0,
      should_pause = 1,
      running = 2
    };


  // add the motor to the Stack Controller.
  void setMotor(MotorControl* motor) {
    motor_ = motor;
  }

  // add the camera to the Stack Controller.
  void setCamera(CameraControl* camera) {
    camera_ = camera;
  }


  // Set the number of stacks to make.
  void setStackCount(uint32_t stack_count) {
    stack_count_ = stack_count;
  }

  // Set the delay before the photo in milliseconds
  void setDelayBeforePhoto(uint32_t delay_before_photo) {
    delay_before_photo_ = delay_before_photo;
  }

  // Set the delay after the photo in milliseconds
  void setDelayAfterPhoto(uint32_t delay_after_photo) {
    delay_after_photo_ = delay_after_photo;
  }

  // Set the steps to move the motor between the photos
  void setMoveSteps(int32_t steps) {
    move_steps_ = steps;
  }

  // Starts the stack procedure with the current settings.
  void stack();

  // This method should be called often, it manages the state machine and calls
  // the motor and camera run methods.
  void run();

  // This returns true if the stacking is finished.
  bool isStackFinished();

  bool isIdle();


  void setConfig(config_t config) {
    setStackCount(config.stack_count);
    setDelayBeforePhoto(config.delay_before_photo);
    setDelayAfterPhoto(config.delay_after_photo);
    setMoveSteps(config.move_steps);
  }

  config_t getConfig() {
    return {stack_count_, delay_before_photo_, delay_after_photo_, move_steps_};
  }

  void move(int32_t steps);

 protected:
  MotorControl* motor_;
  CameraControl* camera_;

  // config
  uint32_t stack_count_;
  uint32_t delay_before_photo_;  // in milliseconds
  uint32_t delay_after_photo_;  // in milliseconds
  int32_t move_steps_;

  // operation
  uint32_t current_step_;  // counter which keeps track of the number of steps
  StackControl::sub_state  sub_state_;  // keeps track of state in this step
  StackControl::state state_ = halted;  // whether we are stacking or halted.
  elapsedMillis duration_;  // elapsed duration for the delays and such.
};

#endif  // FIRMWARE_STACK_CONTROL_H_
