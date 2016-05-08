#!/usr/bin/env python3

import argparse
import cherrypy
import os
import logging
import json
from ws4py.server.cherrypyserver import WebSocketPlugin, WebSocketTool
from ws4py.manager import WebSocketManager
import ws4py

import interface
import message


class FocusStackRoot:
    """
        This is the webserver root of CherryPy.

        :param stack_interface: The interface object that communicates to the
            serial port.

    """
    def __init__(self, stack_interface, preset_dir):
        self.stack = stack_interface
        self.preset_dir = preset_dir

    @cherrypy.expose
    def ws(self):
        # hangle this path with the websocket handler.
        handler = cherrypy.request.ws_handler
    ws._cp_config = {'tools.staticdir.on': False}

    @cherrypy.expose
    @cherrypy.tools.json_out()
    def get_serial_ports(self):
        return interface.get_potential_ports()

    @cherrypy.expose
    @cherrypy.tools.json_out()
    def get_preset_list(self):
        presets = []
        for f in os.listdir(self.preset_dir):
            if (f.endswith(".json")):
                presets.append(f[:-5])
        return presets

    @cherrypy.expose
    @cherrypy.tools.json_out()
    def get_preset_content(self, name):
        preset_path = os.path.join(self.preset_dir, name + ".json")

        if (not os.path.abspath(preset_path).startswith(self.preset_dir)):
            # if this occurs we try to retrieve something outside the dir.
            raise cherrypy.HTTPError(403, "Outside of directory...")

        try:
            with open(preset_path, 'r') as f:
                data = json.load(f)
                failed = False
        except ValueError as e:
            print("Failed to load preset.")
            print(e)
            failed = True
        if failed:
            raise cherrypy.HTTPError(500, "Failed to load json preset.")
        return data

    @cherrypy.expose
    @cherrypy.tools.json_out()
    def set_preset_content(self, name, data):
        preset_path = os.path.join(self.preset_dir, name + ".json")

        if (not os.path.abspath(preset_path).startswith(self.preset_dir)):
            # if this occurs we try to retrieve something outside the dir.
            raise cherrypy.HTTPError(403, "Outside of directory...")

        try:
            with open(preset_path, 'w') as f:
                data = json.dump(json.loads(data), f, indent=4, sort_keys=True)
                failed = False
        except ValueError as e:
            print("Failed to save preset.")
            print(e)
            failed = True
        if failed:
            raise cherrypy.HTTPError(500, "Failed to save json preset.")
        return self.get_preset_list()


class WebsocketHandler(ws4py.websocket.WebSocket):
    """
        The object created for each websocket. It mainly passes on the
        messages it receives via the websocket to the stack interface.

        :param stack_interface: The interface object that communicates to the
            serial port.
        :type stack_interface: `interface.StackInterface` instance.
        :param websocket_manager: The websocket manager which keeps track of
            the websockets.
        :type websocket_manager: `ws4py.manager.WebSocketManager`.
    """
    def __init__(self, stack_interface, websocket_manager, *args, **kwargs):
        super(WebsocketHandler, self).__init__(*args, **kwargs)
        self.stacker = stack_interface
        self.manager = websocket_manager

    def received_message(self, msg):
        """
            Handles messages received via the websocket, so they are coming
            from the browser.
        """
        decoded_json = json.loads(str(msg))
        msgtype, msgdata = decoded_json
        if (msgtype == "serial"):
            # got a serial command, we have to pass this on to the stack object

            # check if we have a serial connection.
            if (not self.stacker.is_serial_connected()):
                self.send(json.dumps(json.dumps(["no_serial"])))
                return
            if (isinstance(msgdata["msg_type"], str)):
                msgdata["msg_type"] = message.msg_type_id[msgdata["msg_type"]]

            # convert the JSON into a message and put it.
            msg = message.Msg()
            try:
                msg.from_dict(msgdata)
                data = msg
                self.stacker.put_message(data)
            except TypeError as e:
                cherrypy.log("Failed sending message: {}".format(e))

        if (msgtype == "connect_serial"):
            # we have to connet the stack interface to the right serial port.
            # if this succeeds we have to broadcast the new serial port to all
            # the connected websockets.
            res = self.stacker.connect(msgdata["device"])
            if (res):
                response = ["connect_success",
                            self.stacker.get_serial_parameters()]
                self.manager.broadcast(json.dumps(response))
            else:
                response = ["connect_fail", msgdata]
                self.send(json.dumps(response))

        if (msgtype == "get_serial_status"):
            if (not self.stacker.is_serial_connected()):
                self.send(json.dumps(["no_serial"]))
            else:
                response = ["connect_success",
                            self.stacker.get_serial_parameters()]
                self.send(json.dumps(response))

    def closed(self, code, reason=None):
        print("Websocket was closed.")
        self.shutdown()

    def shutdown(self):
        print("Socket shutdown")

    @classmethod
    def with_parameters(cls, stack_instance, manager):
        """
            Factory method to wrap this class for use with the default
            websocket arguments. This allows passing the `stack_interface` and
            `manager` arguments to every initialization of the websocket
            handler.
        """
        def factory(*args, **kwargs):
            z = cls(stack_instance, manager, *args, **kwargs)
            return z
        return factory


if __name__ == "__main__":
    # Running as main; should start a server.

    parser = argparse.ArgumentParser(description="Focus Stacking"
                                     " webinterface.")
    parser.add_argument('--port', '-p',
                        help="The port used to listen.",
                        type=int,
                        default=8080)
    parser.add_argument('--host', '-l',
                        help="The interface on which to listen.",
                        type=str,
                        default="127.0.0.1")

    parser.add_argument('--presetdir', '-d',
                        help="Folder which holds preset files.",
                        type=str,
                        default="./presets/")

    # parse the arguments.
    args = parser.parse_args()

    # set up the interface logger.
    interface_logger = logging.getLogger("interface")
    interface_logger.setLevel(logging.DEBUG)

    ch = logging.StreamHandler()
    ch.setLevel(logging.DEBUG)
    formatter = logging.Formatter('%(name)s - %(asctime)s - %(levelname)s'
                                  ' - %(message)s')
    ch.setFormatter(formatter)
    interface_logger.addHandler(ch)

    # create the preset folder if it does not exist:
    preset_dir = os.path.abspath(args.presetdir)
    if (not os.path.isdir(preset_dir)):
        print("Preset folder {} did not exist, creating.".format(preset_dir))
        os.makedirs(preset_dir)

    # Add the websocket requirements.
    cherrypy.tools.websocket = WebSocketTool()

    a = WebSocketPlugin(cherrypy.engine)
    a.manager = WebSocketManager()
    a.subscribe()

    stack_interface = interface.StackInterface()
    stack_interface.daemon = True
    stack_interface.start()
    server_tree = FocusStackRoot(stack_interface, preset_dir=preset_dir)

    # create a broadcast function which relays messages received over the
    # serial port to the websockets via the websocketmanager.
    def broadcaster():
        m = stack_interface.get_message()
        if m:
            payload = dict(m)
            payload["msg_type"] = message.msg_type_name[payload["msg_type"]]
            msg = ["serial", payload]
            a.manager.broadcast(json.dumps(msg))

    # use this very fancy cherrypy monitor to run our broadcaster.
    cherrypy.process.plugins.Monitor(cherrypy.engine,
                                     broadcaster,
                                     frequency=0.01).subscribe()

    cherrypy.config.update({"server.socket_host": args.host,
                            "server.socket_port": args.port})

    static_folder = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                 "static")

    cherrypy.log("Static folder dir: {}".format(static_folder))

    config = {"/": {"tools.staticdir.on": True,
                    "tools.staticdir.dir": static_folder,
                    "tools.staticdir.index": "index.html"},
              "/ws": {"tools.websocket.on": True,
                      "tools.websocket.handler_cls":
                      WebsocketHandler.with_parameters(stack_interface,
                                                       a.manager)}
              }
    cherrypy.quickstart(server_tree, '/', config=config)
