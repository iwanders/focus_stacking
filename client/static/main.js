var Stack = function () {
};

Stack.prototype.open = function(address){
    var self = this;
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

Stack.prototype.send = function(msgtype, data) {
    console.log("In send");
    console.log(this);
    if (this.connected == true) {
        console.log("Sending");
        this.socket.send(JSON.stringify([msgtype, data]));
        console.log(JSON.stringify([msgtype, data]));
    } else {
        console.log("Sending without websocket port.");
    }
};

Stack.prototype.onclose = function() {
    console.log("closing");
    this.connected = false;
};
Stack.prototype.onopen = function() {
    console.log("onopen:");
    console.log(this);
    this.connected = true;
    Stack.prototype.get_serial_status.call(this);
};
Stack.prototype.onmessage = function(msg) {
    console.log("onmessage this:")
    console.log(this);
    console.log("onmessage" + msg.data);
};

Stack.prototype.connect_serial = function (device) {
    this.send("connect_serial", {device:device});
}

Stack.prototype.test = function(){
    console.log("in test");
    console.log(this);
    this.send("serial", {'msg_type':2});
}

Stack.prototype.get_serial_status = function(){
    console.log("In get_serial_status");
    console.log(this);
    this.send("get_serial_status", "");
}

var stacker = new Stack();

/*
    Attach hooks and do stuff...
*/
$( document ).ready(function() {
    console.log( "ready!" );

    stacker.open($(location).attr('host') + "/ws");

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