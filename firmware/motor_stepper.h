#ifndef MOTOR_STEPPER_H
#define MOTOR_STEPPER_H

#include "Arduino.h"



class MotorStepper{

  private:
    uint8_t pin_dir_;  // direction pin
    uint8_t pin_step_; // step pin

    uint32_t step_goal_;                // goal step count
    bool direction_;                    // direction of movement.
    volatile uint32_t step_current_;    // current step count
    volatile uint32_t wait_before_step_;// wait this time in usec before step
    elapsedMicros last_step_;           // timestamp of last step


    uint32_t min_width_; // minimum width of a single step pulse in usec.
    uint32_t max_width_; // maximum width of a single step pulse in usec.
    uint32_t ramp_length_; // reach maximum speed in this amount of steps.

  public:
    void begin(uint8_t pin_dir, uint8_t pin_step){
      pin_dir_ = pin_dir;
      pin_step_ = pin_step;
    }
    void setRampLength(uint32_t ramp_length){
      ramp_length_ = ramp_length;
    }

    void setMinWidth(uint32_t min_width){
      min_width_ = min_width;
    }
    void setMaxWidth(uint32_t max_width){
      max_width_ = max_width;
    }

    void isr();

    uint32_t stepsToGo();
    uint32_t calcDelay(uint32_t step);
    void move(int32_t steps);

};

void motor_stepperISR();
extern IntervalTimer motor_stepper_timer;
extern MotorStepper motor_stepper;
#endif