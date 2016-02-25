#include "Arduino.h"
#include "motor_stepper.h"

#define MOTOR_DIRECTION_PIN 15
#define MOTOR_STEPS_PIN 16
#define ledPin 13



extern "C" int main(void) {
    
    Serial.begin(9600);
    pinMode(MOTOR_DIRECTION_PIN,OUTPUT);
    pinMode(MOTOR_STEPS_PIN,OUTPUT);
    motor_stepper.begin(MOTOR_DIRECTION_PIN, MOTOR_STEPS_PIN);

    pinMode(ledPin, OUTPUT);
    motor_stepper_timer.begin(motor_stepperISR, 200); // every x usec


    motor_stepper.setMinWidth(1000);
    motor_stepper.setMaxWidth(4000);
    motor_stepper.setRampLength(100);

    uint32_t togo;
    elapsedMillis printTimer = 0;
    int16_t distance = 500;

    tone(1, 2000,100);
    while (1) {
      if (printTimer > 20){
        printTimer = 0;
        togo = motor_stepper.stepsToGo();
        Serial.print("Togo: "); Serial.println(togo);
        if (togo == 0){
          delay(1000);
          motor_stepper.move(distance);
          distance = -distance;
        }
      }
      
    }


}
