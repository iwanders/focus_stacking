#include "Arduino.h"
#include "motor_stepper.h"

#define MOTOR_DIRECTION_PIN 15
#define MOTOR_STEPS_PIN 16
#define MOTOR_STEPPER_INTERVAL 200

void testMotor() {
  uint32_t togo;
  elapsedMillis printTimer = 0;
  int16_t distance = 500;
  while (1) {
    if (printTimer > 20) {
      printTimer = 0;
      togo = motor_stepper.stepsToGo();
      Serial.print("Togo: ");
      Serial.println(togo);
      if (togo == 0) {
        delay(1000);
        motor_stepper.move(distance);
        distance = -distance;
      }
    }
  }
}



extern "C" int main(void) {
  Serial.begin(9600);

  // begin the stepper with the correct pins
  motor_stepper.begin(MOTOR_DIRECTION_PIN,
                      MOTOR_STEPS_PIN,
                      MOTOR_STEPPER_INTERVAL);

  motor_stepper.setMinWidth(1000);
  motor_stepper.setMaxWidth(4000);
  motor_stepper.setRampLength(100);

  testMotor();
}
