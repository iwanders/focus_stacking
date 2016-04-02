#!/usr/bin/env python3

import os
import cherrypy
from ws4py.server.cherrypyserver import WebSocketPlugin, WebSocketTool
from ws4py.manager import WebSocketManager
import ws4py


class Root(object):
    def __init__(self):
        pass

    @cherrypy.expose
    def ws(self):
        handler = cherrypy.request.ws_handler
    ws._cp_config = {'tools.staticdir.on': False}


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

    # just here to test broadcasting from server.
    import threading
    import time

    def broadcaster():
        while(True):
            print("Looping broadcaster.")
            time.sleep(5)
            a.broadcast("Test: {}".format(time.time()))
    threading.Timer(0, broadcaster).start()
    # this does not die gracefully....

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
    cherrypy.quickstart(Root(), '/', config=config)
