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
                document.getElementById("percent_slider").value = data;
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
    $('.mobile-only').hide();
    $('.desktop-only').show();
    return false;
}

function dropdown () {
    document.getElementById("controls").classList.toggle('hide-hlp');
    document.getElementById("advanced_controls").classList.toggle('hide-hlp');
}

function gotoWifiPage () {
    document.getElementById("wifi_page").classList.remove('wifi-page-initial');
    console.log(document.getElementById("wireless_radio").checked);
}

function gotoHomePage () {
    document.getElementById("wifi_page").classList.add('wifi-page-initial');
}

function syncSettings() {
    const xhr = new XMLHttpRequest();
    if (document.getElementById("sync_settings").checked) {
        xhr.open('GET', '/motor?sync-settings=1', true);
        document.getElementById("current_open_setting").classList.remove('opening-setting-txt-hide');
        document.getElementById("velocity_open_setting").classList.remove('opening-setting-txt-hide');
        document.getElementById("acceleration_open_setting").classList.remove('opening-setting-txt-hide');
    } else {
        xhr.open('GET', '/motor?sync-settings=0', true);
        document.getElementById("current_open_setting").classList.add('opening-setting-txt-hide');
        document.getElementById("velocity_open_setting").classList.add('opening-setting-txt-hide');
        document.getElementById("acceleration_open_setting").classList.add('opening-setting-txt-hide');
    }
    xhr.send();
}

function openSettingDialog(setting_name, input_step) {
    const lowercase_name = setting_name.toLowerCase() + "";
    document.getElementById("setting_dialog").action = '/motor?';
    document.getElementById("opening_setting_input").step = input_step;
    document.getElementById("opening_setting_input").name = 'opening-' + lowercase_name;
    document.getElementById("closing_setting_input").step = input_step;
    document.getElementById("closing_setting_input").name = 'closing-' + lowercase_name;
    document.getElementById("dialog_setting_prompt").innerText = 'Enter new value for "' 
                                                            + document.getElementById(lowercase_name + "_setting_name").innerText + '"';
    document.getElementById("dialog_setting_name").innerText = 'Enter ' + setting_name;
    document.getElementById("opening_setting_input").placeholder = document.getElementById(lowercase_name + "_open_setting").innerText;
    document.getElementById("closing_setting_input").placeholder = document.getElementById(lowercase_name + "_close_setting").innerText;
    document.getElementById("setting_dialog").classList.add('setting-dlg-show');
    document.getElementById("opening_setting_input").select();
}

function cancel() {
    document.getElementById("setting_dialog").classList.remove('setting-dlg-show');
}

function motorMove(element) {
    console.log(element);
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

function print(element) {
    console.log(element);
}