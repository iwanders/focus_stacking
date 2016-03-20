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
#ifndef FIRMWARE_STACK_INTERFACE_H_
#define FIRMWARE_STACK_INTERFACE_H_

#include "Arduino.h"
#include "./motor_stepper.h"
#include "./camera_control.h"
#include "./stack_control.h"

/*
  The StackInterface allows for interaction with a StackControl object.

  It listens to commands from the serial port and processes these.
*/


#define STACK_INTERFACE_DEBUG
#ifdef STACK_INTERFACE_DEBUG
  #define SIDBG(a) Serial.print(a);
  #define SIDBGln(a) Serial.println(a);
#else
  #define SIDBG(a)
  #define SIDBGln(a)
#endif

class StackInterface{
 public:
  // message types for the messages over the serial port.
  enum msg_type {
    nop = 0,
    set_config = 1,
    get_config = 2
  };

  // config struct for this class.
  typedef struct {
    uint32_t status_interval;
  } config_t;

  // A config message which holds all config structs for all classess.
  typedef struct {
    MotorStepper::config_t motor;
    CameraOptocoupler::config_t camera;
    StackControl::config_t stack;
    StackInterface::config_t interface;
  } msg_config_t;

  // A struct which represents a message.
  typedef struct {
    msg_type type;  // aligned on 4 byte boundary
    union {
      msg_config_t config;
      uint8_t raw[64 - 4];
    };
  } msg_t;


  // add the motor to the Interface.
  void setMotor(MotorStepper* motor) {
    motor_ = motor;
  }

  // add the camera to the Interface.
  void setCamera(CameraOptocoupler* camera) {
    camera_ = camera;
  }

  // add the stacker to the Interface.
  void setStacker(StackControl* stacker) {
    stack_ = stacker;
  }

  // set the interval by which we should emit status messages, in milliseconds.
  void setStatusInterval(uint32_t status_interval) {
    status_interval_ = status_interval;
  }

  // Load configuration from a config struct.
  void setConfig(config_t config) {
    setStatusInterval(config.status_interval);
  }
  config_t getConfig() {
    return {status_interval_};
  }

  // should be called often to process serial input and the like.
  void run();

 protected:
  // pointers to the relevant objects
  MotorStepper* motor_;
  CameraOptocoupler* camera_;
  StackControl* stack_;


  // state
  elapsedMillis duration_;
  uint32_t status_interval_;

  // Emit a status over the serial port
  void sendStatus();

  // process a command message.
  void processCommand(const msg_t* msg);
};



#endif  // FIRMWARE_STACK_INTERFACE_H_
