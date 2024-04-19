window.addEventListener('load', function() {
    var websocket = new WebSocket(`ws://${window.location.hostname}/ws`);
    websocket.onopen = function(event) {
        console.log('Connection established');
    }
    websocket.onclose = function(event) {
        console.log('Connection died');
    }
    websocket.onerror = function(error) {
        console.log('error');
    };
    websocket.onmessage = function(event) {
        if (event.data == "1") {
            document.getElementById('state').innerHTML = "ON";
            document.getElementById('stop-button').checked = true;
        } else {
            document.getElementById('state').innerHTML = "OFF";
            document.getElementById('stop-button').checked = false;
        }
    };
    
    document.getElementById('stop-button').addEventListener('click', function() { websocket.send('-1'); });
    document.getElementById('open-button').addEventListener('click', function() { websocket.send('0'); });
    document.getElementById('close-button').addEventListener('click', function() { websocket.send('100'); });
    document.getElementById('percentage-slider').addEventListener('change', function() { websocket.send(document.getElementById('percentage-slider').value); });
});
