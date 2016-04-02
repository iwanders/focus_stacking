#!/usr/bin/env python3

import cherrypy
import os
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

    @cherrypy.expose
    @cherrypy.tools.json_out()
    def connect_to_serial(self, device):
        result = self.stack.connect(device)
        return {"result": result}


class WebsocketHandler(ws4py.websocket.WebSocket):

    def __init__(self, stack_interface, *args, **kwargs):
        super(WebsocketHandler, self).__init__(*args, **kwargs)
        cherrypy.log("Test")
        self.stacker = stack_interface

    def received_message(self, msg):
        decoded_json = json.loads(str(msg))
        msgtype, msgdata = decoded_json

        if (msgtype == "serial"):
            msg = message.Msg()
            msg.from_dict(msgdata)
            print(msg)
            data = msg
        else:
            data = msgdata

        self.stacker.put_command((msgtype, data))

    def closed(self, code, reason=None):
        print("Websocket was closed.")
        self.shutdown()

    def shutdown(self):
        print("Socket shutdown")

    @classmethod
    def with_callback(cls, stack_instance):
        def factory(*args, **kwargs):
            z = cls(stack_instance, *args, **kwargs)
            return z
        return factory


if __name__ == "__main__":

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
            mtype, mdata = m
            msg = {"type": mtype, "data": dict(mdata)}
            a.broadcast(json.dumps(msg))

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
                      WebsocketHandler.with_callback(stack_interface)}
              }
    cherrypy.quickstart(server_tree, '/', config=config)
