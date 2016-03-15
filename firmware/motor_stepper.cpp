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
#include "motor_stepper.h"

void MotorStepper::move(int32_t steps) {
  step_goal_ = (steps < 0) ? -steps : steps;
  direction_ = (steps < 0) ? HIGH : LOW;

  step_current_ = 0;

  wait_before_step_ = calcDelay(0);
  digitalWrite(pin_dir_, direction_);
}

uint32_t MotorStepper::calcDelay(uint32_t step) {
  if (step < (step_goal_ / 2)) {
    // first half of motion
    if (step < ramp_length_) {
      // in ramp part
      return map(step, 0, ramp_length_, max_width_, min_width_);
    } else {
      // in constant velocity
      return min_width_;
    }
  } else {
    // second half of motion
    if (step > (step_goal_ - ramp_length_)) {
      // in ramp part
      return map(step, (step_goal_ - ramp_length_), step_goal_,
                                                  min_width_, max_width_);
    } else {
      // in constant velocity
      return min_width_;
    }
  }
}

void MotorStepper::isr() {
  if (step_current_ < step_goal_) {
    // if we are not done
    if ((last_step_ < wait_before_step_)) {
      return;  // not time to step yet.
    }
    last_step_ = 0;

    step_current_++;
    digitalWriteFast(pin_step_, HIGH);  // put step high
    // step needs to be atleast 1 usec high, calcDelay is long enough.
    wait_before_step_ = calcDelay(step_current_);
    digitalWriteFast(pin_step_, LOW);  // put step low.
  }
}

// returns 0 if movement is done
// returns number of steps to go otherwise.
uint32_t MotorStepper::stepsToGo() {
  uint32_t current_steps;
  noInterrupts();  // prevent the ISR from firing while we read the variable.
  current_steps = step_current_;
  interrupts();
  return (current_steps < step_goal_) ? (step_goal_ - current_steps) : 0;
}

#ifdef MOTOR_STEPPER_BACKGROUND_USED
MotorSteppersBackgroundClass MotorSteppersBackground;
void motor_steppers_background_ISR() {
  MotorSteppersBackground.isr();
}

#endif  // CORE_TEENSY
