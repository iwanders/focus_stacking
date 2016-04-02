
/*
    Attach hooks and do stuff...
*/
$( document ).ready(function() {
    console.log( "ready!" );

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
                    $.getJSON("connect_to_serial", {device:val.device}, function( data ) {
                        console.log(data);
                    });
                });
                entry.appendTo('#navbar_port_dropdown ul');
            });

        });
    });


});