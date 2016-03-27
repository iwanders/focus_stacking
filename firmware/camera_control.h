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
#ifndef FIRMWARE_CAMERA_CONTROL_H_
#define FIRMWARE_CAMERA_CONTROL_H_

#include "Arduino.h"

class CameraControl{
 public:
  virtual void startPhoto() = 0;
  virtual void run() = 0;
  virtual void stop() = 0;
  virtual bool finishedPhoto() = 0;
};

class CameraOptocoupler: public CameraControl {
  /*
    Class to make a photograph, it first holds the focus pin high, then also
    the trigger pin, then releases both.
  */
 private:
    bool taking_photo_;
    uint8_t focus_pin_;
    uint8_t shutter_pin_;
    elapsedMillis duration_;

    // config
    uint32_t focus_duration_;
    uint32_t shutter_duration_;


 public:
    // config for this object
    typedef struct {
      uint32_t focus_duration;
      uint32_t shutter_duration;
    } config_t;

    // initialises the object with the focus and shutter pins.
    void begin(uint8_t focus_pin, uint8_t shutter_pin) {
      focus_pin_ = focus_pin;
      shutter_pin_ = shutter_pin;
      taking_photo_ = false;
      pinMode(focus_pin_, OUTPUT);
      pinMode(shutter_pin_, OUTPUT);
    }

    // Sets the duration to press the focus pin before the shutter is pressed
    // In milliseconds.
    void setFocusDuration(uint32_t focus_duration) {
      focus_duration_ = focus_duration;
    }

    // Sets the duration the shutter pin is held down after the focus duration.
    // In milliseconds
    void setShutterDuration(uint32_t shutter_duration) {
      shutter_duration_ = shutter_duration;
    }

    void startPhoto();  // to initialise taking the photo
    void run();         // needs to be called frequently in order to toggle pins
    bool finishedPhoto();  // returns true when the photo is done.
    void stop();  // immediately stop everything we are doing.

    void photoBlocking();  // performs a photo in blocking manner.

    // Sets the config based on the provided configuration struct.
    void setConfig(config_t config) {
      setFocusDuration(config.focus_duration);
      setShutterDuration(config.shutter_duration);
    }

    // Gets the current config.
    config_t getConfig(){
      return {focus_duration_, shutter_duration_};
    }
};

#endif  // FIRMWARE_CAMERA_CONTROL_H_
