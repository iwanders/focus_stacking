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
#include "./stack_control.h"


// Start stacking.
void StackControl::stack() {
  current_step_ = 0;  // reset current step counter
  duration_ = 0;
  sub_state_ = next_step;  // ensure that we start with a delay and photo
  state_ = running;  // start stacking.
}

// takes a picture.
void StackControl::photo() {
  current_step_ = 0;  // reset current step counter
  duration_ = 0;
  sub_state_ = start_photo;  // ensure that we start with a delay and photo
  state_ = should_pause;  // start stacking.
}

void StackControl::move(int32_t steps) {
  sub_state_ = start_movement;
  current_step_ = 0;  // reset current step counter
  duration_ = 0;
  this->setMoveSteps(steps);
  state_ = should_pause;
}

void StackControl::stop() {
  state_ = halted;
  motor_->stop();
  camera_->stop();
}


void StackControl::run() {
  // If we are not running, return.
  if (state_ == halted) {
    return;
  }

  // Depending on where we are in the current state, do something:
  switch (sub_state_) {
    case start_delay_before_photo:
      SCDBG("In "); SCDBGln("start_delay_before_photo");
      duration_ = 0;  // next step uses the duration, set it to zero.
      sub_state_ = delay_before_photo;  // advance the state

    case delay_before_photo:
      // SCDBG("In ");SCDBGln("delay_before_photo");
      // if we have waited enough
      if (duration_ > delay_before_photo_) {
        sub_state_ = start_photo;  // advance the state
      }
      break;

    case start_photo:
      // start with the photo
      SCDBG("In "); SCDBGln("start_photo");
      camera_->startPhoto();
      sub_state_ = photo_busy;  // advance the state

    case photo_busy:
      camera_->run();
      // SCDBG("In ");SCDBGln("photo");
      if (camera_->finishedPhoto()) {
        // if this was the last photo...
        if (current_step_ == stack_count_) {
          SCDBG("photo done"); SCDBGln(" Current step == stack_count ");
          sub_state_ = next_step;  // skip delay and movement.
        } else {
          // if done with the photo, advance the state
          sub_state_ = pause_after_photo;
        }
      }
      break;

    case pause_after_photo:
      if (state_ == should_pause) {
        break;
      }

    case start_delay_after_photo:
      SCDBG("In "); SCDBGln("start_delay_after_photo");
      duration_ = 0;  // next step uses the duration, set it to zero.
      sub_state_ = delay_after_photo;  // advance the state

    case delay_after_photo:
      // SCDBG("In ");SCDBGln("delay_after_photo");
      // if we have waited enough
      if (duration_ > delay_after_photo_) {
        sub_state_ = start_movement;  // advance the state
      }
      break;

    case start_movement:
      SCDBG("In "); SCDBGln("start_movement");
      // Send the move instruction to the motor.
      motor_->move(move_steps_);
      sub_state_ = movement;

    case movement:
      // SCDBG("In ");SCDBGln("movement");
      #ifndef MOTOR_STEPPER_BACKGROUND_USED
        motor_->run();
      #endif
      // if done moving, advance to the next step
      if (motor_->stepsToGo() == 0) {
        sub_state_ = pause_after_movement;  // advance the state
      }
      break;

    case pause_after_movement:
      if (state_ == should_pause) {
        break;
      }

    case next_step:
      SCDBG("In "); SCDBGln("next_step");
      current_step_++;  // increment the current step counter
      sub_state_ = start_delay_before_photo;

      // if we have performed all the steps, halt.
      if (current_step_ > stack_count_) {
        state_ = halted;
      }
  }
}


bool StackControl::isStackFinished() {
  return (current_step_ > stack_count_) && (state_ == halted);
}

bool StackControl::isIdle() {
  return (state_ == halted) ||
         ((state_ == should_pause) && ((sub_state_ == pause_after_movement) ||
                                       (sub_state_ == pause_after_photo)));
}
