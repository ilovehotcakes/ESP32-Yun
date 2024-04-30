const char index_html[] = R"rawliteral(<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <!-- go to https://base64.guru/converter/encode/image/ico to convert favicon to base64 image -->
    <link rel="shortcut icon" href="data:image/x-icon;base64,UklGRl4AAABXRUJQVlA4TFIAAAAvOQATADdAEECSeT5D7DLJGgSCEK0zw3hHkAlYrIYc+lfxOkUw//E/JqtVKghkBkUBQDgiiCCECDTRP8XtPT8R/Z8A6DKqpOf1E0oug7T/ip4C" />
    <title>ESP32 Motorcover</title>
    <style>
html {
    font-family: Helvetica;
    display: inline-block;
    margin: 0px auto;
    text-align: center;
}
body {
    margin-top: 50px;
    -webkit-user-select: none;
    touch-action: pan-x pan-y;
}
h1 {
    color: #444444;
    margin: 50px auto;
}
p {
    font-size: 19px;
    color: #888;
}
.button {
    display: inline-block;
    width: 100px;
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
        <button id="open-button" class="button" onclick="motorMove(0)">OPEN</button>
        <button id="stop-button" class="button" onclick="motorStop(this)">STOP</button>
        <button id="close-button" class="button" onclick="motorMove(100)">CLOSE</button>
    </div>
    
    <div>
        <input id="percentage-slider" class="slider" onchange="motorMove(this)" type="range" value="%SLIDER%" min="0" max="100" step="1">
    </div>
    <script>
const percentage_slider_ = document.getElementById('percentage-slider');

window.addEventListener('load', () => {
    try {
        const websocket = new WebSocket(`ws://${window.location.hostname}/ws`);

        websocket.onopen = (event) => {
            console.log(`Connection established with ${window.location.hostname}`);
            console.log(event);
        };

        websocket.onclose = (event) => {
            console.log(`Connection with ${window.location.hostname} closed`);
            console.log(event);
        };

        websocket.onerror = (error) => {
            console.log("Websocket error");
            console.log(error);
        };

        websocket.onmessage = (event) => {
            const data = event.data;
            console.log(data);
            if (data > -1) {
                percentage_slider_.value = data;
            }
        };
    } catch (error) {
        console.log("Failed to connect to websocket")
        console.log(error)
    }
});

function motorMove(element) {
    const xhr = new XMLHttpRequest();
    if (element == '0' || element == '100') {
        xhr.open('GET', '/motor?position=' + element, true);
    } else {
        xhr.open('GET', '/motor?position=' + percentage_slider_.value, true);
    }
    xhr.send();
}

function motorStop(element) {
    const xhr = new XMLHttpRequest();
    xhr.open('GET', '/motor?stop=1', true);
    xhr.send();
}    </script>
</body>
</html>)rawliteral";