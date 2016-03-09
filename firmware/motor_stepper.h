#ifndef FIRMWARE_MOTOR_STEPPER_H_
#define FIRMWARE_MOTOR_STEPPER_H_

#include "Arduino.h"

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


void motor_stepperISR();
extern IntervalTimer motor_stepper_timer;

class MotorStepper{
 private:
  uint8_t pin_dir_;   // direction pin
  uint8_t pin_step_;  // step pin

  uint32_t step_goal_;                  // goal step count
  bool direction_;                      // direction of movement.
  volatile uint32_t step_current_;      // current step count
  volatile uint32_t wait_before_step_;  // wait this time in usec before step
  elapsedMicros last_step_;             // timestamp of last step


  uint32_t min_width_;  // minimum width of a single step pulse in usec.
  uint32_t max_width_;  // maximum width of a single step pulse in usec.
  uint32_t ramp_length_;  // reach maximum speed in this amount of steps.

  uint32_t calcDelay(uint32_t step);  // calculates the time between the steps

 public:
  // Set the pins to be used, directory is made HIGH for reverse movement.
  // The step pin is pulsed high for a very short duration (several usec).
  // timer_interval specifies the interval of the timer in microseconds.
  void begin(uint8_t pin_dir, uint8_t pin_step, uint32_t timer_interval) {
    pin_dir_ = pin_dir;
    pin_step_ = pin_step;
    pinMode(pin_dir_, OUTPUT);
    pinMode(pin_step_, OUTPUT);

    // start a intervalTimer for the actual stepping.
    motor_stepper_timer.begin(motor_stepperISR, timer_interval);
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

  void isr();
  // routine which sets the direction & step pin if it should.

  uint32_t stepsToGo();  // returns the number of steps still to go.

  // move this number of steps
  void move(int32_t steps);
};

extern MotorStepper motor_stepper;

#endif  // FIRMWARE_MOTOR_STEPPER_H_
