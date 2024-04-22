window.addEventListener('load', function() {
    const websocket = new WebSocket(`ws://${window.location.hostname}/ws`);
    let percentage_slider_ = document.getElementById('percentage-slider');

    websocket.onopen = function(event) {
        console.log('Connection established');

        document.getElementById('stop-button').addEventListener('click', function() { websocket.send('-1'); });
        document.getElementById('open-button').addEventListener('click', function() { websocket.send('0'); });
        document.getElementById('close-button').addEventListener('click', function() { websocket.send('100'); });
        percentage_slider_.addEventListener('change', function() { websocket.send(percentage_slider_.value); });

        websocket.send('-100');  // To get initial motor position
    };
    websocket.onclose = function(event) {
        console.log('Connection closed');
    };
    websocket.onerror = function(error) {
        console.log('error');
    };
    websocket.onmessage = function(event) {
        var data = event.data
        console.log(data);
        if (data > -1) {
            percentage_slider_.value = data;
        }
    };
});
