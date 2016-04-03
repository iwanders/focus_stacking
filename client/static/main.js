var Stack = function () {
    this.connected = false;
    this.dispatchers = {
        fallback: function(command, payload){
            console.log("Unhandled command: " + command + " payload:");
            console.log(payload);
        }
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
    // try to reconnect.
    setTimeout(function() {
      self.reconnect();
    }, 1000)
};
Stack.prototype.onopen = function() {
    this.connected = true;
    Stack.prototype.get_serial_status.call(this);
};
Stack.prototype.onmessage = function(msg) {
    decoded = $.parseJSON(msg.data);
    var command = decoded[0];
    var payload = decoded[1];

    // try to dispatch registered functions
    if (command in this.dispatchers) {
        this.dispatchers[command](command, payload);
    } else {
        this.dispatchers.fallback(command, payload);
    }
};

Stack.prototype.connect_serial = function (device) {
    this.send("connect_serial", {device:device});
}

Stack.prototype.get_serial_status = function(){
    this.send("get_serial_status", "");
}

Stack.prototype.attachCommandHandler = function (command, fun){
    this.dispatchers[command] = fun;
}

var stacker = new Stack();

/*
    Attach hooks and do stuff...
*/
$( document ).ready(function() {
    console.log( "ready!" );

    stacker.open($(location).attr('host') + "/ws");

    stacker.attachCommandHandler("no_serial", function (command, payload){
        console.log("no_serial handler");
        console.log(payload);
    });

    stacker.attachCommandHandler("connect_success", function (command, payload){
        console.log("connect_success handler");
        console.log(payload);
    });

    // Deal with the dropdown
    $('#navbar_port_dropdown').bind("show.bs.dropdown", function (){
        console.log("navbar_port_dropdown clicked.");

        // get the serial ports that are possible.
        $.getJSON("get_serial_ports", function( data ) {
            // clear the current
            $('#navbar_port_dropdown ul').empty();

            // create the new items.
            var style_additives = ""
            $.each( data, function( key, val ) {
                // if we are to highlight it.
                if (val.likely) {
                    style_additives = "alert-success"
                } else {
                    style_additives = ""
                }
                var entry = $('<li><a href="#" class=' + style_additives + ' data-device="' + val.device + '">' + val.device + ' - ' + val.manufacturer +  '</a></li>');
                $(entry).select('a').on("click", function (){
                    // console.log( $( event.data).text() );
                    // $.getJSON("connect_to_serial", {device:val.device}, function( data ) {
                       // console.log(data);
                    // });

                    stacker.connect_serial(val.device);
                });
                entry.appendTo('#navbar_port_dropdown ul');
            });

        });
    });


});