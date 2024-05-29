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

    if (document.getElementById("sync_settings").checked) {
        showOpeningSettings();
    }
});

function print(element) {
    console.log(element);
}

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

function openSettingDialog(setting_name, input_step) {
    const lowercase_name = setting_name.toLowerCase();
    const opening_setting_input = document.getElementById("opening_setting_input");
    const closing_setting_input = document.getElementById("closing_setting_input");
    document.getElementById("setting_dialog").action = '/motor?';
    opening_setting_input.step = input_step;
    opening_setting_input.name = 'opening-' + lowercase_name;
    closing_setting_input.step = input_step;
    closing_setting_input.name = 'closing-' + lowercase_name;
    document.getElementById("dialog_setting_prompt").innerText = 'Enter new value for "' 
                                                            + document.getElementById(lowercase_name + "_setting_name").innerText + '"';
    document.getElementById("dialog_setting_name").innerText = 'Enter ' + setting_name;
    opening_setting_input.placeholder = document.getElementById(lowercase_name + "_open_setting").innerText;
    closing_setting_input.placeholder = document.getElementById(lowercase_name + "_close_setting").innerText;
    document.getElementById("setting_dialog").classList.add('setting-dlg-show');
    opening_setting_input.select();
}

function cancelForm() {
    document.getElementById("setting_dialog").classList.remove('setting-dlg-show');
}

function submitForm() {
    const open_setting = document.getElementById("opening_setting_input");
    const close_setting = document.getElementById("closing_setting_input");
    const action = document.getElementById("setting_dialog").action;
    const request = action + open_setting.name + '=' + open_setting.value + '&' + close_setting.name + '=' + close_setting.value;
    const xhr = new XMLHttpRequest();
    xhr.open('GET', request);
    xhr.send();
    cancelForm();
}

function motorHttpRequest(param, value=1) {
    const xhr = new XMLHttpRequest();
    xhr.open('GET', '/motor?' + param + '=' + value, true);
    xhr.send();
}

function motorMove(element) {
    motorHttpRequest('percent=' + element.value);
}

function showOpeningSettings() {
    document.getElementById("current_open_setting").classList.add('opening-setting-txt-show');
    document.getElementById("velocity_open_setting").classList.add('opening-setting-txt-show');
    document.getElementById("acceleration_open_setting").classList.add('opening-setting-txt-show');
}

function hideOpeningSettings() {
    document.getElementById("current_open_setting").classList.remove('opening-setting-txt-show');
    document.getElementById("velocity_open_setting").classList.remove('opening-setting-txt-show');
    document.getElementById("acceleration_open_setting").classList.remove('opening-setting-txt-show');
}

function syncSettings() {
    if (document.getElementById("sync_settings").checked) {
        motorHttpRequest('sync-settings');
        showOpeningSettings();
    } else {
        motorHttpRequest('sync-settings', 0);
        hideOpeningSettings();
    }
}

function checkboxHttpRequest(param) {
    if (document.getElementById(param).checked) {
        motorHttpRequest(param);
    } else {
        motorHttpRequest(param, 0);
    }
}