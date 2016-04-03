#!/usr/bin/env python3

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
    def __init__(self, packet_size=64):
        super().__init__()
        self.ser = None
        self.running = False

        self.packet_size = packet_size

        self.rx = queue.Queue()
        self.tx = queue.Queue()

    def connect(self, serial_port, baudrate=9600, **kwargs):
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
        self.running = False
        time.sleep(0.001)
        if (self.ser):
            self.ser.close()

    def process_rx(self):
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
        except serial.SerialException as e:
            self.ser.close()
            self.ser = None

    def process_tx(self):
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
        self.running = True
        while (self.running):
            # small sleep to prevent this loop from running too fast.
            time.sleep(0.001)

            self.process_tx()  # read from the tx queue

            if (self.ser is None):
                continue
            self.process_rx()  # read from serial port

    def is_serial_connected(self):
        return True if (self.ser is not None) and (
                                    self.ser.isOpen()) else False

    def get_serial_parameters(self):
        return {"device":self.ser.port, "baudrate":self.ser.baudrate}

    # adds a message to the to be sent queue. (to serial)
    def put_message(self, message):
        self.tx.put_nowait(message)

    # try to get a message from the received queue. (from serial)
    def get_message(self):
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
    parser = argparse.ArgumentParser(description="Control Focus Stacking.")
    parser.add_argument('--port', '-p', help="The serial port to connect to.",
                        default="/dev/ttyACM0")

    subparsers = parser.add_subparsers(dest="command")

    for i in range(0, len(message.msg_type_lookup)):
        command = message.msg_type_lookup[i]
        command_parser = subparsers.add_parser(command)
        if (command.startswith("set_") or command.startswith("action_")):
            if (i in message.msg_type_field):
                fieldname = message.msg_type_field[i]
                tmp = message.Msg()
                tmp.msg_type = i
                config = str(getattr(tmp, fieldname)).replace("'", '"')
                helpstr = "Json representing configuration {}".format(config)
                command_parser.add_argument('config', help=helpstr)

    args = parser.parse_args()

    # no command
    if (args.command is None):
        parser.print_help()
        parser.exit()
        sys.exit(1)

    msg = message.Msg()
    msg.msg_type = getattr(msg.type, args.command)

    a = StackInterface()
    a.connect(args.port)
    a.start()

    # we are retrieving something.
    if (args.command.startswith("get_")):
        a.put_message(msg)
        m = a.wait_for_message()
        print(m)
        a.stop()
        a.join()
        sys.exit(0)

    # sending something
    if (args.command.startswith("set_") or command.startswith("action_")):
        command_id = getattr(message.msg_type, args.command)
        if (command_id in message.msg_type_field):
            fieldname = message.msg_type_field[command_id]
            d = {fieldname: json.loads(args.config)}
            msg.from_dict(d)
        print("Sending {}".format(msg))

        # send the message and wait until it is really gone.
        a.put_message(msg)
        while(not a.tx.empty()):
            time.sleep(0.1)

        a.stop()
        a.join()
        sys.exit(0)
