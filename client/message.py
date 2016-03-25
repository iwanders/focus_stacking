import ctypes
from collections import namedtuple

# enum-like construction of msg_type_t
msg_type_t = namedtuple("msg_type", ["nop", "set_config", "get_config"])
msg_type = msg_type_t(*range(0, len(msg_type_t._fields)))
# can do msg_type.nop now.

# Depending on the message type, we interpret the payload as to be this field:
msg_type_field = {msg_type_t._fields.index("set_config"): "config",
                  msg_type_t._fields.index("get_config"): "config"}

# Reverse lookup for msg type, that is id->name
msg_type_lookup = dict((v, k) for v, k in list(enumerate(msg_type_t._fields)))


# Convenience mixin to allow construction of struct from a byte like object.
class readable():
    @classmethod
    def read(self, byte_object):
        a = self()
        ctypes.memmove(ctypes.addressof(a), bytes(byte_object),
                       min(len(byte_object), ctypes.sizeof(self)))
        return a


# Pretty printing the structure.
class printable():
    def __str__(self):
        output = "{"
        for k, t in self._fields_:
            output += "{}: {}, ".format(k, getattr(self, k))
        return output[:-2] + "}"


# Structs for the various parts in the stacking firmware.
class ConfigMotorStepper(ctypes.LittleEndianStructure, printable):
    _fields_ = [("min_width", ctypes.c_uint),
                ("max_width", ctypes.c_uint),
                ("ramp_length", ctypes.c_uint)]


class ConfigCameraOptocoupler(ctypes.LittleEndianStructure, printable):
    _fields_ = [("focus_duration", ctypes.c_uint),
                ("shutter_duration", ctypes.c_uint)]


class ConfigStackControl(ctypes.LittleEndianStructure, printable):
    _fields_ = [("stack_count", ctypes.c_uint),
                ("delay_before_photo", ctypes.c_uint),
                ("delay_after_photo", ctypes.c_uint),
                ("move_steps", ctypes.c_int)]


class ConfigStackInterface(ctypes.LittleEndianStructure, printable):
    _fields_ = [("status_interval", ctypes.c_uint)]


# A message config consists of this:
class MsgConfig(ctypes.LittleEndianStructure, printable):
    _fields_ = [("motor", ConfigMotorStepper),
                ("camera", ConfigCameraOptocoupler),
                ("stack", ConfigStackControl),
                ("interface", ConfigStackInterface)]


class _MsgBody(ctypes.Union):
    _fields_ = [("config", MsgConfig), ("raw", ctypes.c_byte * (64-4))]


class Msg(ctypes.LittleEndianStructure, readable):
    type = msg_type
    _fields_ = [("msg_type", ctypes.c_uint),
                ("_body", _MsgBody)]
    _anonymous_ = ["_body"]

    # Pretty print the message according to its type.
    def __str__(self):
        payload_text = str(getattr(self, msg_type_field[self.msg_type]))
        message_field = msg_type_lookup[self.msg_type]
        return "<Msg {}: {}>".format(message_field, payload_text)


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
    print(msg)
    msg_bytes = bytes(msg)
    print(msg_bytes)

    msg_load = Msg.read(msg_bytes)
    print(msg_load)
