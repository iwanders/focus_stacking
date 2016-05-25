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


import argparse
from collections import namedtuple
import json
import logging
import serial
import serial.tools.list_ports
import sys
import threading
import time
import queue

import message

logger = logging.getLogger(__name__)


class StackInterface(threading.Thread):
    """
        Class to handle communication with the serial port. It uses a separate
        thread to handle the communication with the serial port. Internally the
        object uses two queues, one to be sent on the serial port and one that
        contains the messages received from the serial port.

        Placing messages on the queue to be sent is done with the `put_message`
        method. Reading received messages is done with the `get_message`
        method.

        :param packet_size: The size of all messages in bytes.
        :type packet_size: int
    """
    def __init__(self, packet_size=64):
        super().__init__()
        self.ser = None
        self.running = False

        self.packet_size = packet_size

        self.rx = queue.Queue()
        self.tx = queue.Queue()

    def connect(self, serial_port, baudrate=9600, **kwargs):
        """
            Connects the object to a serial port.

            :param serial_port: The path to the serial port to connect to.
            :type serial_port: str
            :param baudrate: The baudrate to use.
            :type baudrate: int
        """
        packet_read_timeout = self.packet_size * (1.1 / baudrate)

        try:
            self.ser = serial.Serial(serial_port,
                                     baudrate=baudrate,
                                     timeout=packet_read_timeout,
                                     **kwargs)
            logger.debug("Succesfully connected to {}.".format(serial_port))
            return True
        except serial.SerialException as e:
            logger.warn("Failed to connect to {}".format(serial_port))
            return False

    def stop(self):
        """
            Cleanly shuts down the running thread and closes the serial port.
        """
        self.running = False
        time.sleep(0.001)
        if (self.ser):
            self.ser.close()

    def _process_rx(self):
        # try to read message from serial port
        try:
            if (self.ser.inWaiting()):
                buffer = bytearray(64)
                d = self.ser.readinto(buffer)

                # Did we get the correct number of bytes? If so queue it.
                if (d == self.packet_size):
                    msg = message.Msg.read(buffer)
                    self.rx.put_nowait(msg)
                else:
                    logging.warn("Received incomplete packet, discarded.")
        except (serial.SerialException, OSError) as e:
            self.ser.close()
            self.ser = None

    def _process_tx(self):
        # try to put a message on the serial port from the queue
        try:
            msg = self.tx.get_nowait()
        except queue.Empty:
            pass  # there was no data there.
            return

        if (self.ser is None):
            logging.warn("Trying to send on a closed serial port.")

        try:
            logger.debug("Processing {}".format(msg))
            self.ser.write(bytes(msg))
        except serial.SerialException:
            self.ser.close()
            self.ser = None

    def run(self):
        # this method is called when the thread is started.
        self.running = True
        while (self.running):
            # small sleep to prevent this loop from running too fast.
            time.sleep(0.001)

            self._process_tx()  # read from the tx queue

            if (self.ser is None):
                continue
            self._process_rx()  # read from serial port

    def is_serial_connected(self):
        """
            Returns whether this object is connected to a serial port.

            :returns: boolean
        """
        return True if (self.ser is not None) and (
                                    self.ser.isOpen()) else False

    def get_serial_parameters(self):
        """
            Returns a dictionary holding the device and baudrate of the current
            serial connection.

            :returns: dict containing a "device" and "baudrate" field.
        """
        return {"device": self.ser.port, "baudrate": self.ser.baudrate}

    def put_message(self, message):
        """
            Places a message on the queue that is to be sent to the serial
            port.

            :param message: The message to be transmitted on the serial port.
            :type message: Some object which is a valid input argument to
                the `bytes` function.
        """
        self.tx.put_nowait(message)

    def get_message(self):
        """
            Gets a message from the queue that is received on the serial port.

            :returns: A `bytes` instance.
        """
        try:
            msg = self.rx.get_nowait()
            return msg
        except queue.Empty:
            return None

    # for command line tool
    def wait_for_message(self):
        while(True):
            try:
                time.sleep(0.01)
                m = self.get_message()
                if (m):
                    # print(m)
                    break
            except KeyboardInterrupt:
                break
        return m


# function that returns a dictionary of potential ports
def get_potential_ports():
    """
        Gets a list of potential ports. Basically it wraps
        `serial.tools.list_ports.comports` from pyserial.

        :returns: A list of dict's with information about each port.
    """
    ports = []
    for ser in serial.tools.list_ports.comports():
        likely = ser.manufacturer == "Teensyduino"
        ports.append({"name": ser.name,
                      "device": ser.device,
                      "manufacturer": ser.manufacturer,
                      "product": ser.product,
                      "likely": likely})
    return ports


if __name__ == "__main__":
    # Create a simple commandline interface.

    parser = argparse.ArgumentParser(description="Control Focus Stacking.")
    parser.add_argument('--port', '-p', help="The serial port to connect to.",
                        default="/dev/ttyACM0")
    parser.add_argument('--verbose', '-v', help="Print all communication.",
                        action="store_true", default=False)

    subparsers = parser.add_subparsers(dest="command")

    # add subparsers for each command.
    for i in range(0, len(message.msg_type_name)):
        command = message.msg_type_name[i]
        command_parser = subparsers.add_parser(command)
        if (command.startswith("set_") or command.startswith("action_")):
            if (i in message.msg_type_field):
                fieldname = message.msg_type_field[i]
                tmp = message.Msg()
                tmp.msg_type = i
                config = str(getattr(tmp, fieldname)).replace("'", '"')
                helpstr = "Json representing configuration {}".format(config)
                command_parser.add_argument('config', help=helpstr)

    # parse the arguments.
    args = parser.parse_args()

    # no command
    if (args.command is None):
        parser.print_help()
        parser.exit()
        sys.exit(1)

    # create a message of the right type to be sent.
    msg = message.Msg()
    msg.msg_type = getattr(msg.type, args.command)

    # create the interface and connect to a serial port
    a = StackInterface()
    a.connect(args.port)
    a.start()  # start the interface

    # we are retrieving something.
    if (args.command.startswith("get_")):
        if (args.verbose):
            print("Sending: {}".format(bytes(msg)))
        a.put_message(msg)
        m = a.wait_for_message()
        if (args.verbose):
            print("Retrieved: {}".format(bytes(m)))
        print(m)
        a.stop()
        a.join()
        sys.exit(0)

    # sending something
    if (args.command.startswith("set_") or args.command.startswith("action_")):
        command_id = getattr(message.msg_type, args.command)
        if (command_id in message.msg_type_field):
            fieldname = message.msg_type_field[command_id]
            d = {fieldname: json.loads(args.config)}
            msg.from_dict(d)
        print("Sending {}".format(msg))
        if (args.verbose):
            print("Sending: {}".format(bytes(msg)))

        # send the message and wait until it is really gone.
        a.put_message(msg)
        while(not a.tx.empty()):
            time.sleep(0.1)

        a.stop()
        a.join()
        sys.exit(0)
