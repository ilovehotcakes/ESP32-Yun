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
.desktop-only-elements {
    display: none;
}
.mobile-only-elements {
    display: inline-block;
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
        <button id="stop-button" class="button" onclick="motorStop()">STOP</button>
        <button id="close-button" class="button" onclick="motorMove(100)">CLOSE</button>
    </div>

    <div class="desktop-only-elements">
        <button id="forward-button" class="button" onmousedown="motorForward()" onmouseup="motorStop()">FORWARD</button>
        <button id="backward-button" class="button" onmousedown="motorBackward()" onmouseup="motorStop()">BACKWARD</button>
    </div>
    
    <div class="mobile-only-elements">
        <button id="forward-button" class="button" ontouchstart="motorForward()" ontouchend="motorStop()">FORWARD</button>
        <button id="backward-button" class="button" ontouchstart="motorBackward()" ontouchend="motorStop()">BACKWARD</button>
    </div>

    <div>
        <button id="set-min-button" class="button" onmousedown="motorSetMin()" onmouseup="motorStop()">SET MIN</button>
        <button id="set-max-button" class="button" onmousedown="motorSetMax()" onmouseup="motorStop()">SET MAX</button>
    </div>

    <div>
        <input id="percentage-slider" class="slider" onchange="motorMove(this)" type="range" value="%SLIDER%" min="0" max="100" step="1">
    </div>
    <script src="https://code.jquery.com/jquery-3.6.0.min.js"></script>
    <script>
const percentage_slider_ = document.getElementById('percentage-slider');

window.addEventListener('load', () => {
    isMobileDevice();
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

function isMobileDevice() {
    var userAgent = navigator.userAgent || navigator.vendor || window.opera;

    // Is mobile device
    if (/android/i.test(userAgent) || /iPhone|iPad|iPod/i.test(userAgent)) {
        return true;
    }

    // Is desktop device
    $('.mobile-only-elements').hide();
    $('.desktop-only-elements').show();
    return false;
}

function motorMove(element) {
    const xhr = new XMLHttpRequest();
    if (element == '0' || element == '100') {
        xhr.open('GET', '/motor?percent=' + element, true);
    } else {
        xhr.open('GET', '/motor?percent=' + percentage_slider_.value, true);
    }
    xhr.send();
}

function motorStop() {
    const xhr = new XMLHttpRequest();
    xhr.open('GET', '/motor?stop=1', true);
    xhr.send();
}

function motorForward() {
    const xhr = new XMLHttpRequest();
    xhr.open('GET', '/motor?forward=1', true);
    xhr.send();
}

function motorBackward() {
    const xhr = new XMLHttpRequest();
    xhr.open('GET', '/motor?backward=1', true);
    xhr.send();
}

function motorSetMin() {
    const xhr = new XMLHttpRequest();
    xhr.open('GET', '/motor?set-min=1', true);
    xhr.send();
}

function motorSetMax() {
    const xhr = new XMLHttpRequest();
    xhr.open('GET', '/motor?set-max=1', true);
    xhr.send();
}


// wifi name
// wifi password
// step
// openclose
// velocity
// current
// acceleration
// sg
// tcoolsthresh
// sg thresh
// fastmode
// fastmode thresh
// direction
// microstep
    </script>
</body>
</html>)rawliteral";