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
#include "./stack_interface.h"


void StackInterface::run() {
  // check if we need to send the status
  if (duration_ > status_interval_) {
    duration_ = 0;
    sendStatus();
  }

  // check if we should read the serial port
  if (Serial.available()) {
    char buffer[sizeof(msg_t)] = {0};
    if (Serial.readBytes(buffer, sizeof(msg_t)) == sizeof(msg_t)) {
      // we have a command, process it.
      processCommand(reinterpret_cast<msg_t*>(buffer));
    }
  }
}

void StackInterface::sendStatus() {
  // assemble the status update...
}

void StackInterface::processCommand(const msg_t* msg) {
  switch (msg->type) {
    case nop:
      SIDBGln("Got nop.");
      break;

    case set_config:
      SIDBGln("Got set_config.");
      motor_->setConfig(msg->config.motor);
      camera_->setConfig(msg->config.camera);
      stack_->setConfig(msg->config.stack);
      this->setConfig(msg->config.interface);
      break;

    case get_config: {
        SIDBGln("Got get_config.");
        char buffer[sizeof(msg_t)] = {0};
        msg_t* response = reinterpret_cast<msg_t*>(buffer);
        response->type = get_config;
        response->config.motor = motor_->getConfig();
        response->config.camera = camera_->getConfig();
        response->config.stack = stack_->getConfig();
        response->config.interface = this->getConfig();
        SIDBGln(response->config.motor.min_width);
        Serial.write(buffer, sizeof(msg_t));
      }
      break;

    case start_stack: 
        stack_->stack();
      break;

    default:
      SIDBGln("Got unknown command.");
  }
}
