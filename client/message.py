#!/usr/bin/env python3

# The MIT License (MIT)
#
# Copyright (c) 2016 Ivor Wanders
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import ctypes
from collections import namedtuple

# enum-like construction of msg_type_t
msg_type_t = namedtuple("msg_type", ["nop",
                                     "set_config",
                                     "get_config",
                                     "action_stack",
                                     "action_motor",
                                     "action_photo",
                                     "action_stop",
                                     "get_version",
                                     "get_status"])
# can do msg_type.nop or msg_type.get_config now.
msg_type = msg_type_t(*range(0, len(msg_type_t._fields)))

# Depending on the message type, we interpret the payload as to be this field:
msg_type_field = {msg_type_t._fields.index("set_config"): "config",
                  msg_type_t._fields.index("get_config"): "config",
                  msg_type_t._fields.index("action_motor"): "action_motor",
                  msg_type_t._fields.index("get_version"): "version",
                  msg_type_t._fields.index("get_status"): "status"}

# Reverse lookup for msg type, that is id->name
msg_type_name = dict(enumerate(msg_type_t._fields))

# Reverse lookup for msg type, that is name->id
msg_type_id = dict((k, v) for v, k in enumerate(msg_type_t._fields))


# Convenience mixin to allow construction of struct from a byte like object.
class Readable:
    @classmethod
    def read(cls, byte_object):
        a = cls()
        ctypes.memmove(ctypes.addressof(a), bytes(byte_object),
                       min(len(byte_object), ctypes.sizeof(cls)))
        return a


# Mixin to allow conversion of a ctypes structure to and from a dictionary.
class Dictionary:
    # Implement the iterator method such that dict(...) results in the correct
    # dictionary.
    def __iter__(self):
        for k, t in self._fields_:
            if (issubclass(t, ctypes.Structure)):
                yield (k, dict(getattr(self, k)))
            else:
                yield (k, getattr(self, k))

    # Implement the reverse method, with some special handling for dict's and
    # lists.
    def from_dict(self, dict_object):
        for k, t in self._fields_:
            set_value = dict_object[k]
            if (isinstance(set_value, dict)):
                v = t()
                v.from_dict(set_value)
                setattr(self, k, v)
            elif (isinstance(set_value, list)):
                v = getattr(self, k)
                for j in range(0, len(set_value)):
                    v[j] = set_value[j]
                setattr(self, k, v)
            else:
                setattr(self, k, set_value)

    def __str__(self):
        return str(dict(self))


# Structs for the various parts in the stacking firmware. These correspond to
# the structures as defined in the header files.
class ConfigMotorStepper(ctypes.LittleEndianStructure, Dictionary):
    _fields_ = [("min_width", ctypes.c_uint),
                ("max_width", ctypes.c_uint),
                ("ramp_length", ctypes.c_uint)]


class ConfigCameraOptocoupler(ctypes.LittleEndianStructure, Dictionary):
    _fields_ = [("focus_duration", ctypes.c_uint),
                ("shutter_duration", ctypes.c_uint)]


class ConfigStackControl(ctypes.LittleEndianStructure, Dictionary):
    _fields_ = [("stack_count", ctypes.c_uint),
                ("delay_before_photo", ctypes.c_uint),
                ("delay_after_photo", ctypes.c_uint),
                ("move_steps", ctypes.c_int)]


class ConfigStackInterface(ctypes.LittleEndianStructure, Dictionary):
    _fields_ = [("status_interval", ctypes.c_uint),
                ("buzzer_frequency", ctypes.c_uint16),
                ("buzzer_duration", ctypes.c_uint16),
                ("ui_transmission_ratio", ctypes.c_float)]


# A message config consists of this:
class MsgConfig(ctypes.LittleEndianStructure, Dictionary):
    _fields_ = [("motor", ConfigMotorStepper),
                ("camera", ConfigCameraOptocoupler),
                ("stack", ConfigStackControl),
                ("interface", ConfigStackInterface)]


# status messages for the parts.
class StatusMotorStepper(ctypes.LittleEndianStructure, Dictionary):
    _fields_ = [("steps_to_go", ctypes.c_uint)]


class StatusCameraOptocoupler(ctypes.LittleEndianStructure, Dictionary):
    _fields_ = [("taking_photo", ctypes.c_bool)]


class StatusStackControl(ctypes.LittleEndianStructure, Dictionary):
    _fields_ = [("current_state", ctypes.c_byte),
                ("current_sub_state", ctypes.c_byte),
                ("current_step", ctypes.c_uint),
                ("stack_count", ctypes.c_uint),
                ("current_duration", ctypes.c_uint),
                ("is_stack_finished", ctypes.c_bool),
                ("is_idle", ctypes.c_bool)]


# The status message for the whole status message.
class MsgStatus(ctypes.LittleEndianStructure, Dictionary):
    _fields_ = [("motor", StatusMotorStepper),
                ("camera", StatusCameraOptocoupler),
                ("stack", StatusStackControl)]


# other messages
class MsgVersion(ctypes.LittleEndianStructure, Dictionary):
    _fields_ = [("unstaged", ctypes.c_byte),
                ("staged", ctypes.c_byte),
                ("hash", ctypes.c_byte * (41))]

    def __iter__(self):
        for k, t in self._fields_:
            if (issubclass(t, ctypes.Structure)):
                yield (k, dict(getattr(self, k)))
            elif (k == "hash"):
                yield (k, [a for a in getattr(self, k)])
            else:
                yield (k, getattr(self, k))


class ActionMotor(ctypes.LittleEndianStructure, Dictionary):
    _fields_ = [("steps", ctypes.c_int)]


# create the composite message.
class _MsgBody(ctypes.Union):
    _fields_ = [("config", MsgConfig),
                ("action_motor", ActionMotor),
                ("version", MsgVersion),
                ("status", MsgStatus),
                ("raw", ctypes.c_byte * (64-4))]


# Class which represents all messages. That is; it holds all the structs.
class Msg(ctypes.LittleEndianStructure, Readable):
    type = msg_type
    _fields_ = [("msg_type", ctypes.c_uint),
                ("_body", _MsgBody)]
    _anonymous_ = ["_body"]

    # Pretty print the message according to its type.
    def __str__(self):
        if (self.msg_type in msg_type_field):
            payload_text = str(getattr(self, msg_type_field[self.msg_type]))
            message_field = msg_type_name[self.msg_type]
        else:
            message_field = msg_type_name[self.msg_type]
            payload_text = "-"
        return "<Msg {}: {}>".format(message_field, payload_text)

    # We have to treat the mixin slightly different here, since we there is
    # special handling for the message type and thus the body.
    def __iter__(self):
        for k, t in self._fields_:
            if (k == "_body"):
                if (self.msg_type in msg_type_field):
                    message_field = msg_type_field[self.msg_type]
                    body = dict(getattr(self, msg_type_field[self.msg_type]))
                else:
                    message_field = "raw"
                    body = [a for a in getattr(self, message_field)]
                yield (message_field, body)
            elif (issubclass(t, ctypes.Structure)):
                yield (k, dict(getattr(self, k)))
            else:
                yield (k, getattr(self, k))

    def from_dict(self, dict_object):
        # Walk through the dictionary, as we do not know which elements from
        # the struct we would need.
        for k, set_value in dict_object.items():
            if (isinstance(set_value, dict)):
                v = getattr(self, k)
                v.from_dict(set_value)
                setattr(self, k, v)
            elif (isinstance(set_value, list)):
                v = getattr(self, k)
                for j in range(0, len(set_value)):
                    v[j] = set_value[j]
                setattr(self, k, v)
            else:
                setattr(self, k, set_value)


if __name__ == "__main__":
    def print_sizeof(identifier, size):
        print("Sizeof {: >30s}: {: >3d}".format(identifier, size))

    print_sizeof("ConfigMotorStepper", ctypes.sizeof(ConfigMotorStepper))
    print_sizeof("ConfigStackInterface", ctypes.sizeof(ConfigStackInterface))
    print_sizeof("ConfigStackControl", ctypes.sizeof(ConfigStackControl))
    print_sizeof("MsgConfig", ctypes.sizeof(MsgConfig))
    print_sizeof("Msg", ctypes.sizeof(Msg))

    msg = Msg()
    msg.msg_type = msg.type.set_config
    msg.config.motor.min_width = 1337
    msg.config.motor.max_width = 1
    msg.config.motor.ramp_length = 2
    msg_d = dict(msg)
    motor_dict = msg_d["config"]["motor"]

    if (motor_dict["min_width"] != msg.config.motor.min_width):
        print("No match")
    if (motor_dict["max_width"] != msg.config.motor.max_width):
        print("No match")
    if (motor_dict["ramp_length"] != msg.config.motor.ramp_length):
        print("No match")

    # lets try to break it.
    import random
    for i in range(0, len(msg_type)):
        buffer = bytearray([random.randint(0, 255) for a in range(0, 64)])
        a = Msg.read(buffer)
        a.msg_type = i
        print("A: {}".format(str(a)))
        a_dict = dict(a)
        print("A dict: {}".format(str(a_dict)))
        # print(a_dict)
        b = Msg()
        b.from_dict(a_dict)
        print("b: {}".format(str(b)))

        print("\n")
        if (dict(a) != dict(b)):
            print("Broken stuff...")
