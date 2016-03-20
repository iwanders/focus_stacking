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
#ifndef FIRMWARE_MOTOR_STEPPER_H_
#define FIRMWARE_MOTOR_STEPPER_H_

#include "Arduino.h"
#include "./config.h"
#include "./motor_control.h"

// #define USE_MOTOR_STEPPER_BACKGROUND


#define MOTOR_STEPPER_BACKGROUND_MOTOR_ARRAY_SIZE 4
// Length of array of pointers to MotorSteppers in the MotorSteppersBackground
// object, this determines the maximum number of motors it can move.

/*
  This class is intended for driver IC's / boards which have a direction pin to
  set the direction and a step pin which needs to be latched to move one step.

  It was tested with an EasyDriver ( http://www.schmalzhaus.com/EasyDriver/ ).

  A linear speed profile is attempted; movement starts at the lowest speed with
  delays of 'max_width' between the steps and ramps up over 'ramp_length' steps
  towards the fastest movement with 'min_width' delay between the pulses.

  No manipulation is done to the number of steps for the ramp: The stepper will
  latch exactly the number of steps desired to the pin.

  The isr() method should be called 'often' enough in order to achieve smooth
  movement.

  To disable the velocity profile, just set the ramp_length to zero.

*/


class MotorStepper: public MotorControl{
 protected:
  // pins
  uint8_t pin_dir_;   // direction pin
  uint8_t pin_step_;  // step pin

  // state
  uint32_t step_goal_;                  // goal step count
  bool direction_;                      // direction of movement.
  volatile uint32_t step_current_;      // current step count
  volatile uint32_t wait_before_step_;  // wait this time in usec before step
  elapsedMicros last_step_;             // timestamp of last step


  // config
  uint32_t min_width_;  // minimum width of a single step pulse in usec.
  uint32_t max_width_;  // maximum width of a single step pulse in usec.
  uint32_t ramp_length_;  // reach maximum speed in this amount of steps.

  // internal method
  uint32_t calcDelay(uint32_t step);  // calculates the time between the steps

 public:

  typedef struct {
    uint32_t min_width;
    uint32_t max_width;
    uint32_t ramp_length;
  } config_t;

  // Set the pins to be used, directory is made HIGH for reverse movement.
  // The step pin is pulsed high for a very short duration (several usec).
  // timer_interval specifies the interval of the timer in microseconds.
  void begin(uint8_t pin_dir, uint8_t pin_step) {
    pin_dir_ = pin_dir;
    pin_step_ = pin_step;
    pinMode(pin_dir_, OUTPUT);
    pinMode(pin_step_, OUTPUT);
    step_goal_ = 0;
  }

  // Set the number of steps to be used as a ramp up towards the max speed.
  void setRampLength(uint32_t ramp_length) {
    ramp_length_ = ramp_length;
  }

  // The minimum step delay, so the fastest movement.
  void setMinWidth(uint32_t min_width) {
    min_width_ = min_width;
  }

  // The maximum step delay, so the slowest movement.
  // Since it is a stepper motor there is actually no 'slowest' movement this is
  // merely where the ramp starts.
  void setMaxWidth(uint32_t max_width) {
    max_width_ = max_width;
  }

  void run();
  // routine which sets the direction & step pin if it should.

  uint32_t stepsToGo();  // returns the number of steps still to go.

  // move this number of steps
  void move(int32_t steps);

  // immediately halt movement.
  void stop();

  void setConfig(config_t config){
    setRampLength(config.ramp_length);
    setMaxWidth(config.max_width);
    setMinWidth(config.min_width);
  }
  config_t getConfig(){
    return {min_width_, max_width_, ramp_length_};
  }
};


#if defined(CORE_TEENSY) && defined(USE_MOTOR_STEPPER_BACKGROUND)
#define MOTOR_STEPPER_BACKGROUND_USED
  /*
    Class to automate calling the ISR of various MotorStepper objects, such that
    the interval timer can be used to perform this act in the background.

    Can deal with multiple motors in one isr.

    If this class is used, one can do:

      motor_stepper.begin(MOTOR_DIRECTION_PIN,
                          MOTOR_STEPS_PIN);

      motor_stepper.setMinWidth(1000);
      motor_stepper.setMaxWidth(4000);
      motor_stepper.setRampLength(100);
      MotorSteppersBackground.addMotor(&motor_stepper);
      MotorSteppersBackground.setInterval(500);

    Which automatically calls motor_stepper.isr() every 500 usec.
  */
void motor_steppers_background_ISR();  // definition of the global isr function.
class MotorSteppersBackgroundClass {
 private:
    uint32_t isr_interval_;
    uint8_t motor_count_ = 0;
    MotorStepper* motors_[MOTOR_STEPPER_BACKGROUND_MOTOR_ARRAY_SIZE];
    IntervalTimer interval_timer_;

 public:
    void setInterval(uint32_t isr_interval) {  // sets the interval in usec.
      isr_interval_ = isr_interval;
      interval_timer_.end();
      interval_timer_.begin(motor_steppers_background_ISR, isr_interval_);
    }
    void addMotor(MotorStepper* motor) {
      motors_[motor_count_++] = motor;
    }

    void isr() {
      // the global ISR function is called by the intervaltimer, it calls this
      // method which in turn calls the isr methods of the motors it knows.
      for (uint8_t i=0; i < motor_count_; i++) {
        motors_[i]->run();
      }
    }
};
// create one instance of the MotorSteppersBackground
extern MotorSteppersBackgroundClass MotorSteppersBackground;
#endif

#endif  // FIRMWARE_MOTOR_STEPPER_H_
