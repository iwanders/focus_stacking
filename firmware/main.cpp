#include "Arduino.h"

#define MOTOR_DIRECTION_PIN 15
#define MOTOR_STEPS_PIN 16
#define ledPin 13



class MotorStepper{
  /*
  */

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
    MotorStepper(uint8_t pin_dir, uint8_t pin_step){
      pin_dir_ = pin_dir;
      pin_step_ = pin_step;
    }

    void move(int32_t steps){
      step_goal_ = (steps < 0) ? -steps : steps;
      direction_ = (steps < 0) ? HIGH : LOW;

      step_current_ = 0;

      wait_before_step_ = calcDelay(0);
      digitalWrite(pin_dir_, direction_);
      
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

    uint32_t calcDelay(uint32_t step){
      if (step < (step_goal_ / 2)){
        // first half of motion
        if (step < ramp_length_){
          // in ramp part
          return map(step, 0, ramp_length_, max_width_, min_width_);
        } else {
          // in constant velocity
          return min_width_;
        }
      } else {
        // second half of motion
        if (step > (step_goal_ - ramp_length_)){
          // in ramp part
          return map(step, (step_goal_ - ramp_length_), step_goal_, min_width_, max_width_);
        } else {
          // in constant velocity
          return min_width_;
        }
  
      }
    }

    void isr(){
      if (step_current_ < step_goal_){
          // if we are not done
          if ((last_step_ < wait_before_step_)){
            return; // not time to step yet.
          }
          last_step_ = 0;

          step_current_++;
          digitalWriteFast(pin_step_, HIGH); // put step high
          // step needs to be atleast 1 usec high, calcDelay is long enough.
          wait_before_step_ = calcDelay(step_current_);
          digitalWriteFast(pin_step_, LOW); // put step low.
      }
    }

    // returns 0 if movement is done
    // returns number of steps to go otherwise.
    uint32_t stepsToGo(){
      uint32_t current_steps;
      noInterrupts();
      current_steps = step_current_;
      interrupts();
      return (current_steps < step_goal_) ? (step_goal_ - current_steps) : 0;
    }
};



IntervalTimer myTimer;
MotorStepper myMotor(MOTOR_DIRECTION_PIN, MOTOR_STEPS_PIN);

void MotorStepperISR(){
  myMotor.isr();
}
      



void rotate(int steps, float speed){
  //rotate a specific number of microsteps (8 microsteps per step) - (negitive for reverse movement)
  //speed is any number from .01 -> 1 with 1 being fastest - Slower is stronger
  int dir = (steps > 0)? HIGH:LOW;
  steps = abs(steps);

  digitalWrite(MOTOR_DIRECTION_PIN,dir); 

  float usDelay = (1/speed) * 70;

  for(int i=0; i < steps; i++){
    digitalWrite(MOTOR_STEPS_PIN, HIGH);
    delayMicroseconds(usDelay); 

    digitalWrite(MOTOR_STEPS_PIN, LOW);
    delayMicroseconds(usDelay);
  }
} 


extern "C" int main(void) {
    
    Serial.begin(9600);
    pinMode(MOTOR_DIRECTION_PIN,OUTPUT);
    pinMode(MOTOR_STEPS_PIN,OUTPUT);
  
    pinMode(ledPin, OUTPUT);
    myTimer.begin(MotorStepperISR, 200); // every x usec


    myMotor.setMinWidth(1000);
    myMotor.setMaxWidth(4000);
    myMotor.setRampLength(100);

    uint32_t togo;
    elapsedMillis printTimer = 0;
    int16_t distance = 500;

    tone(1, 2000,100);
    while (1) {
      if (printTimer > 20){
        printTimer = 0;
        togo = myMotor.stepsToGo();
        Serial.print("Togo: "); Serial.println(togo);
        if (togo == 0){
          delay(1000);
          myMotor.move(distance);
          distance = -distance;
        }
      }
      
    }


}
