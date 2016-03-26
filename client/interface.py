#!/usr/bin/env python3

import serial
import argparse
import logging
import threading
import time
import queue

import message


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
            logging.debug("Succesfully connected to {}.".format(serial_port))

        except serial.SerialException as e:
            logging.warn("Failed to connect to {}".format(serial_port))

    def stop(self):
        self.running = False
        if (self.ser):
            self.ser.close()

    def run(self):
        self.running = True
        while (self.running):
            if (self.ser == None):
                self.stop()

            # try to read message from serial port
            if (self.ser.inWaiting()):
                try:
                    buffer = bytearray(64)
                    d = self.ser.readinto(buffer)

                    # if we got the correct number of bytes put it in the queue.
                    if (d == self.packet_size):
                        msg = message.Msg.read(buffer)
                        self.rx.put_nowait(msg)
                    else:
                        logging.warn("Received incomplete packet, discarded.")
                except serial.SerialException:
                    self.stop()

            # try to put a message on the serial port from the queue
            try:
                msg = self.tx.get_nowait()
                self.ser.write(bytes(msg))
            except queue.Empty:
                pass # there was no data there.
            except serial.SerialException:
                self.stop()

            # small sleep to prevent this loop from running too fast.
            time.sleep(0.001)

    def get_message(self):
        try:
            msg = self.rx.get_nowait()
            return msg
        except queue.Empty:
            return None

    def put_message(self, msg):
        self.tx.put_nowait(msg)

    def wait_for_message(self):
        while(True):
            try:
                time.sleep(0.01)
                m = self.get_message()
                if (m):
                    # print(m)
                    break
            except KeyboardInterrupt:
                break;
        return m
        

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Interact with focus stacking")
    parser.add_argument('--port', '-p', help="The serial port to connect to.",
                        default="/dev/ttyACM0")  

    subparsers = parser.add_subparsers(dest="command")

    for i in range(0, len(message.msg_type_lookup)):
        command = message.msg_type_lookup[i]
        command_parser = subparsers.add_parser(command)


    args = parser.parse_args()

    # no command
    if (args.command == None):
        parser.print_help()
        parser.exit()

    # we are retrieving something.
    if (args.command.startswith("get_")):
        msg = message.Msg()
        msg.msg_type = getattr(msg.type, args.command)

        a = StackInterface()
        a.connect(args.port)
        a.start()

        a.put_message(msg)
        m = a.wait_for_message()
        print(m)
        a.stop()
        a.join()

