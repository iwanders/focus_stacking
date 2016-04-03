

var stacker = new Stack();
var current_config = {
                        config: {
                            motor:{
                                min_width: 500,
                                max_width: 4000,
                                ramp_length:100,
                            },
                            camera:{
                                shutter_duration: 500,
                                focus_duration: 500,
                            },
                            stack:{
                                delay_after_photo: 1000,
                                delay_before_photo: 1000,
                                stack_count: 5,
                                move_steps: 100
                            },
                            inferface:{
                                status_interval: 100,
                            }
                        },
                        misc: {
                                transmission_ratio:1
                        }
                    };


function setMotorElements(){
    $('#motor_min_width').val(current_config["config"]["motor"]["min_width"]);
    $('#motor_max_width').val(current_config["config"]["motor"]["max_width"]);
    $('#motor_ramp_length').val(current_config["config"]["motor"]["ramp_length"]);
}
function setAllElements(){
    setMotorElements();
}


/*
    Attach hooks and do stuff...
*/

/*
    On page reade we do the following:
        connect to the websocket. 
            - If this fails, display a noticed.
            - If this is successful, remove notice.
        When the websocket is open, we obtain the serial status.
            - If no_serial was obtained; display notice.
            - If a serial port was available, connect_success is sent from the server.
        If connect_success is received:
            - display a warning that the version retrieval is taking long.
            - try to obtain the version
        If the version if obtained:
            - remove the version warning, we are finally good to go.
            - sent a get_config command to retrieve the config.
*/

$( document ).ready(function() {
    console.log( "ready!" );
    stacker.open($(location).attr('host') + "/ws");

    stacker.attachCommandHandler("no_serial", function (command, payload){
        $('#no_serial_error').text("Not connected to a serial port. Connect to a serial port first.");
        $('#no_serial_error').show();
        
    });

    stacker.attachCommandHandler("connect_fail", function (command, payload){
        $('#no_serial_error').text("Could not connect to \"" + payload["device"] + "\", do you have the permission to do so?");
        $('#no_serial_error').show();
    });

    stacker.attachCommandHandler("connect_success", function (command, payload){
        console.log("Successfully got serial");
        $('#no_serial_error').hide();
        // we are not yet sure that we have a correct serial port.
        $('#no_version_response').show()
        stacker.serial_get_version();;
    });

    stacker.attachCommandHandler("serial_get_version", function (command, payload){
        console.log("Successfully got version");
        $('#no_version_response').hide();
        var hash = payload["version"]["hash"];
        // console.log(hash.map(String.fromCharCode).join('')); // why doesnt this work??
        var hashstring = ""
        for (var i = 0; i < hash.length; i++) {
            hashstring += String.fromCharCode(hash[i]);
            //Do something
        }
        var lookup = {0: "clean", 1:"dirty", 2:"unknown"}
        if (payload["version"]["staged"] != 0) {
            hashstring += "<br /> Repo Staged: " + lookup[payload["version"]["staged"]];
        }
        if (payload["version"]["unstaged"] != 0) {
            hashstring += "<br /> Repo Unstaged: " + lookup[payload["version"]["unstaged"]];
        }
        $('#firmware_version').html(hashstring);

        stacker.serial_get_config();
    });

    stacker.attachCommandHandler("serial_get_config", function (command, payload) {
        current_config["config"] = payload["config"]
        setAllElements();
    });


    stacker.attachCommandHandler("onopen", function (){
        $('#no_websocket_error').hide();
    });

    stacker.attachCommandHandler("onclose", function (){
        $('#no_websocket_error').show();
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
                var entry = $('<li><a href="#" class=' + style_additives + ' >' + val.device + ' - ' + val.manufacturer +  '</a></li>');
                $(entry).select('a').on("click", function (){
                    // console.log( $( event.data).text() );
                    // $.getJSON("connect_to_serial", {device:val.device}, function( data ) {
                       // console.log(data);
                    // });

                    stacker.connectSerial(val.device);
                });
                entry.appendTo('#navbar_port_dropdown ul');
            });

        });
    });


});