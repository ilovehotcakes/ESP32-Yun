const char index_html[] = R"rawliteral(<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>ESP32 Motorcover</title>
    <style>
html{font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}
body{margin-top: 50px; -webkit-user-select: none; touch-action: pan-x pan-y;}
h1{color: #444444;margin: 50px auto;}
p{font-size: 19px;color: #888;}
#state{font-weight: bold;color: #444;}
.button {
    display: inline-block;
    width: 25%;
    margin: 0px 5px;
    padding: 10px;
    font-size: 20px;
    text-align: center;
    cursor: pointer;
    outline: none;
    color: #fff;
    background-color: #04AA6D;
    border: none;
    border-radius: 15px;
    box-shadow: 0 9px #999;
}
.button:active {
    background-color: #3e8e41;
    box-shadow: 0 5px #666;
    transform: translateY(4px);
    transform: translateY(4px);
}
.slider {
    -webkit-appearance: none;
    appearance: none;
    width: 300px;
    margin: 50px 0px;
    height: 50px;
    background: #d3d3d3;
    outline: none;
}
.slider::-webkit-slider-thumb {
    -webkit-appearance: none;
    appearance: none;
    width: 15px; /* Set a specific slider handle width */
    height: 35px; /* Slider handle height */
    background: #04AA6D; /* Green background */
    cursor: pointer;
}
    </style>
</head>
<body>
    <h1>ESP32 Motorcover</h1>
    <div>
        <button id="open-button" class="button">OPEN</button>
        <button id="stop-button" class="button">STOP</button>
        <button id="close-button" class="button">CLOSE</button>
    </div>
    <input id="percentage-slider" class="slider" type="range" min="0" max="100">

    <script>
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
    </script>
</body>
</html>)rawliteral";