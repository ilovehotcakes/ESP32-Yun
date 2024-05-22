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
                document.getElementById("percent-slider").value = data;
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
    xhr.open('GET', '/motor?percent=' + element.value, true);
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