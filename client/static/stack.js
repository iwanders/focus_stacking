var Stack = function () {
    this.connected = false;
    this.dispatchers = {
        fallback: function(command, payload){
            console.log("Unhandled command: " + command + " payload:");
            console.log(payload);
        },
        
        onopen: function(){
            console.log("Websocket open");
        },
        onclose: function(){
            console.log("Websocket closed");
        },
        
    };
};

Stack.prototype.open = function(address){
    var self = this;
    this.address = address;
    try {
        this.socket = new WebSocket("ws://" + address);
        this.socket.onclose = function(){
            Stack.prototype.onclose.call(self);
        };
        this.socket.onopen = function(){
            Stack.prototype.onopen.call(self);
        };
        this.socket.onmessage = function(msg){
            Stack.prototype.onmessage.call(self, msg);
        };
    } catch(exception){
        console.log("Failure: " + exception);
    }
}

Stack.prototype.reconnect = function(){
    this.open(this.address);
}

Stack.prototype.send = function(msgtype, data) {
    if (this.connected == true) {
        console.log("Sending");
        this.socket.send(JSON.stringify([msgtype, data]));
        console.log(JSON.stringify([msgtype, data]));
    } else {
        console.log("Sending without websocket port.");
    }
};

Stack.prototype.onclose = function() {
    this.connected = false;
    var self = this;
    this.dispatchers.onclose();
    // try to reconnect.
    setTimeout(function() {
      self.reconnect();
    }, 1000)
};
Stack.prototype.onopen = function() {
    this.connected = true;
    this.dispatchers.onopen();
    Stack.prototype.getSerialStatus.call(this);
};
Stack.prototype.onmessage = function(msg) {
    decoded = $.parseJSON(msg.data);
    var commandtype = decoded[0];
    var payload = decoded[1];

    var command;
    if (commandtype == "serial"){
        // console.log("Serial command");
        // console.log(payload);
        command = "serial_" + payload["msg_type"]
    } else {
        command = commandtype
    }
    // try to dispatch registered functions
    if (command in this.dispatchers) {
        this.dispatchers[command](command, payload);
    } else {
        this.dispatchers.fallback(command, payload);
    }
};

Stack.prototype.connectSerial = function (device) {
    this.send("connect_serial", {device:device});
}

Stack.prototype.getSerialStatus = function(){
    this.send("get_serial_status", "");
}

Stack.prototype.attachCommandHandler = function (command, fun){
    this.dispatchers[command] = fun;
}

// commands starting with serial_ are in underscore naming convention
// so that they are clearly distinguishable.
Stack.prototype.serial_get_version = function(){
    this.send("serial", {"msg_type":"get_version"});
}

Stack.prototype.serial_get_config = function(){
    this.send("serial", {"msg_type":"get_config"});
}

