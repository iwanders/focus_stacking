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


#ifndef VERSION_WORKING_DIRECTORY_CHANGES_UNSTAGED
  #define VERSION_WORKING_DIRECTORY_CHANGES_UNSTAGED 2
#endif

#ifndef VERSION_WORKING_DIRECTORY_CHANGES_STAGED
  #define VERSION_WORKING_DIRECTORY_CHANGES_STAGED 2
#endif

// https://gcc.gnu.org/onlinedocs/cpp/Stringification.html
#define make_xstr(s) make_str(s)
#define make_str(s) #s
#ifndef VERSION_GIT_HASH
  #define VERSION_GIT_HASH "unknown"
#endif


/*
  The StackInterface allows for interaction with a StackControl object.

  It listens to commands from the serial port and processes these.
*/


// #define STACK_INTERFACE_DEBUG
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
    get_config = 2,
    action_stack = 3,
    action_motor = 4,
    action_camera = 5,
    action_stop = 6,
    get_version = 7,
    get_status = 8,
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

  typedef struct {
    uint8_t unstaged;
    uint8_t staged;
    uint8_t hash[41];
  } msg_version_t;

  typedef struct {
    int32_t steps;
  } msg_action_motor_t;

  typedef struct {
    MotorStepper::status_t motor;
    CameraOptocoupler::status_t camera;
    StackControl::status_t stack;
  } msg_status_t;

  // A struct which represents a message.
  typedef struct {
    msg_type type;  // aligned on 4 byte boundary
    union {
      msg_config_t config;
      msg_action_motor_t action_motor;
      msg_version_t version;
      msg_status_t status;
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

  // sets the stored configs in the StackInterface to those of the components.
  void retrieveConfigs();

  void setStartStackPin(uint8_t start_stack_pin){
    start_stack_pin_ = start_stack_pin;
    pinMode(start_stack_pin_, INPUT_PULLUP);
  }

 protected:
  // pointers to the relevant objects
  MotorStepper* motor_;
  CameraOptocoupler* camera_;
  StackControl* stack_;

  // store the stack configuration of the various components.
  MotorStepper::config_t config_motor_;
  CameraOptocoupler::config_t config_camera_;
  StackControl::config_t config_stack_;

  // state
  elapsedMillis duration_;
  uint32_t status_interval_;

  // pin etc.
  uint8_t start_stack_pin_;

  // Emit a status over the serial port
  void sendStatus();

  // process a command message.
  void processCommand(const msg_t* msg);


  void setConfigs();
};



#endif  // FIRMWARE_STACK_INTERFACE_H_
