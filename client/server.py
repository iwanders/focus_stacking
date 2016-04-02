#!/usr/bin/env python3

import cherrypy
import os
import json
from ws4py.server.cherrypyserver import WebSocketPlugin, WebSocketTool
from ws4py.manager import WebSocketManager
import ws4py

import interface


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

    def __init__(self, callback, *args, **kwargs):
        super(WebsocketHandler, self).__init__(*args, **kwargs)
        cherrypy.log("Test")
        self.callback = callback

    def received_message(self, message):
        print("Got message from websocket {}.".format(message))
        self.callback("Calling fun: {}".format(message))
        self.send("Echo: {}".format(message))

    def closed(self, code, reason=None):
        print("Websocket was closed.")
        self.shutdown()

    def shutdown(self):
        print("Socket shutdown")

    @classmethod
    def with_callback(cls, callbackfun):
        def factory(*args, **kwargs):
            z = cls(callbackfun, *args, **kwargs)
            return z
        return factory


if __name__ == "__main__":

    # Add the websocket requirements.
    cherrypy.tools.websocket = WebSocketTool()

    a = WebSocketPlugin(cherrypy.engine)
    a.manager = WebSocketManager()
    a.subscribe()

    stack_interface = interface.StackInterface()
    server_tree = FocusStackRoot(stack_interface)

    def broadcaster():
        m = stack_interface.get_message()
        if m:
            msg = {"type": m.type, "data": m.data}
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
                      WebsocketHandler.with_callback(print)}
              }
    cherrypy.quickstart(server_tree, '/', config=config)
