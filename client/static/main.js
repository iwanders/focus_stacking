

var stacker = new Stack();
var current_config = {
                        motor_min_width: 500,
                        motor_max_width: 4000,
                        motor_ramp_length:100,

                        camera_shutter_duration: 500,
                        camera_focus_duration: 500,

                        stack_delay_after_photo: 1000,
                        stack_delay_before_photo: 1000,
                        stack_stack_count: 5,
                        stack_move_steps: 100,

                        interface_status_interval: 100,

                        ui_transmission_ratio:1,
                };

var config_element_relations = {
    motor_min_width : {selector:'#motor_min_width input', key: "motor_min_width", parser:parseFloat},
    motor_max_width : {selector:'#motor_max_width input', key: "motor_max_width", parser:parseFloat},
    motor_ramp_length : {selector:'#motor_ramp_length input', key: "motor_ramp_length", parser:parseFloat},

    camera_shutter_duration : {selector:'#camera_shutter_duration input', key: "camera_shutter_duration", parser:parseFloat},
    camera_focus_duration : {selector:'#camera_focus_duration input', key: "camera_focus_duration", parser:parseFloat},
}

function getAllElements(){
    $.each(config_element_relations, function (k, v){
        current_config[v.key] = v.parser($(v.selector).val());
    });
}

function setAllElements(){
    $.each(config_element_relations, function (k, v){
        $(v.selector).val(current_config[v.key]);
        $(v.selector).trigger("change");
    });
}

/*
    Attach hooks and do stuff...
*/

/*
    On page ready we do the following:
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

    $.each(config_element_relations, function (k, v){
        console.log("k");
        console.log(k);
        console.log("v");
        console.log(v);

        $(v.selector).change(function () {
            var number = v.parser($(this).val());
            console.log("Change number: " + number);
            if (number == current_config[v.key]){
                console.log("identical");
                $(v.selector).parent().addClass('has-success');
            } else {
                console.log("different");
                $(v.selector).parent().removeClass('has-success');
            }
        });

    });

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
        stacker.serial_get_version();
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
        //current_config["config"] = payload["config"]
        var cfg = payload["config"];
        console.log("Got config");
        console.log(cfg);
        console.log(stacker.flatten_config(cfg));
        console.log(current_config);
        $.extend(current_config, stacker.flatten_config(cfg))
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
                    stacker.connectSerial(val.device);
                });
                entry.appendTo('#navbar_port_dropdown ul');
            });

        });
    });


    $('#motor_config_upload').click(function (event){
        console.log(event);
        getAllElements();
        stacker.serial_set_config(stacker.unflatten_config(current_config));
        stacker.serial_get_config();
        this.blur();
    });

    $('#motor_config_move').click(function (event){
        console.log(event);
        var steps = parseFloat($('#motor_config_move_steps').val());
        stacker.serial_action_motor(steps);
        this.blur();
    });


});