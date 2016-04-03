#!/usr/bin/env python3

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
    def __init__(self, stack_interface):
        self.stack = stack_interface

    @cherrypy.expose
    def ws(self):
        handler = cherrypy.request.ws_handler
    ws._cp_config = {'tools.staticdir.on': False}

    @cherrypy.expose
    @cherrypy.tools.json_out()
    def get_serial_ports(self):
        return interface.get_potential_ports()


class WebsocketHandler(ws4py.websocket.WebSocket):

    def __init__(self, stack_interface, websocket_manager, *args, **kwargs):
        super(WebsocketHandler, self).__init__(*args, **kwargs)
        self.stacker = stack_interface
        self.manager = websocket_manager

    def received_message(self, msg):
        decoded_json = json.loads(str(msg))
        msgtype, msgdata = decoded_json

        print(msg)

        if (msgtype == "serial"):
            # check if we have a serial connection.
            if (not self.stacker.is_serial_connected()):
                self.send(json.dumps(json.dumps(["no_serial"])))
                return
            if (isinstance(msgdata["msg_type"], str)):
                msgdata["msg_type"] = message.msg_type_id[msgdata["msg_type"]]

            # convert the JSON into a message and put it.
            msg = message.Msg()
            msg.from_dict(msgdata)
            data = msg
            self.stacker.put_message(data)

        if (msgtype == "connect_serial"):
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
        def factory(*args, **kwargs):
            z = cls(stack_instance, manager, *args, **kwargs)
            return z
        return factory


if __name__ == "__main__":

    # set up the interface logger.
    interface_logger = logging.getLogger("interface")
    interface_logger.setLevel(logging.DEBUG)

    ch = logging.StreamHandler()
    ch.setLevel(logging.DEBUG)
    formatter = logging.Formatter('%(name)s - %(asctime)s - %(levelname)s'
                                  ' - %(message)s')
    ch.setFormatter(formatter)
    interface_logger.addHandler(ch)

    # Add the websocket requirements.
    cherrypy.tools.websocket = WebSocketTool()

    a = WebSocketPlugin(cherrypy.engine)
    a.manager = WebSocketManager()
    a.subscribe()

    stack_interface = interface.StackInterface()
    stack_interface.daemon = True
    stack_interface.start()
    server_tree = FocusStackRoot(stack_interface)

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

    cherrypy.config.update({"server.socket_host": "127.0.0.1",
                            "server.socket_port": 8080})

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
