window.addEventListener('load', function() {
    connect();
    isMobileDevice();
});

function connect() {
    try {
        const websocket = new WebSocket(`ws://${window.location.hostname}/ws`);

        websocket.onopen = (event) => {
            console.log(`Connection established with ${window.location.hostname}`);
            console.log(event);
        };

        websocket.onclose = (event) => {
            console.log(`Connection with ${window.location.hostname} closed`);
            console.log(event);
            setTimeout(connect(), 1000);
        };

        websocket.onerror = (error) => {
            console.log('Websocket error');
            console.log(error);
        };

        websocket.onmessage = (event) => {
            const data = JSON.parse(event.data);
            // console.log(data);
            document.getElementById('percent_slider').value = data.motor_position;
            if (data.motor.sync_settings_) {
                document.getElementById('sync_settings').checked = true;
                hideOpeningSettings();
            } else {
                document.getElementById('sync_settings').checked = false;
            }
            document.getElementById('opening_current').innerText = data.motor.open_current_;
            document.getElementById('closing_current').innerText = data.motor.clos_current_;
            document.getElementById('opening_velocity').innerText = parseFloat(data.motor.open_velocity_).toFixed(1);
            document.getElementById('closing_velocity').innerText = parseFloat(data.motor.clos_velocity_).toFixed(1);
            document.getElementById('opening_acceleration').innerText = parseFloat(data.motor.open_accel_).toFixed(1);
            document.getElementById('closing_acceleration').innerText = parseFloat(data.motor.clos_accel_).toFixed(1);
            if (data.motor.direction_) {
                document.getElementById('direction').checked = true;
            } else {
                document.getElementById('direction').checked = false;
            }
            document.getElementById('full_steps').innerText = data.motor.full_steps_;
            document.getElementById('microsteps').innerText = data.motor.microsteps_;
            if (data.motor.spreadcycl_en_) {
                document.getElementById('fastmode').checked = true;
            } else {
                document.getElementById('fastmode').checked = false;
            }
            document.getElementById('fastmode_threshold').innerText = data.motor.spreadcycl_th_;
            if (data.motor.stallguard_en_) {
                document.getElementById('stallguard').checked = true;
            } else {
                document.getElementById('stallguard').checked = false;
            }
            document.getElementById('stallguard_threshold').innerText = data.motor.stallguard_th_;
            if (data.wireless.setup_mode_) {
                document.getElementById('setup').checked = true;
            } else {
                document.getElementById('setup').checked = false;
            }
            document.getElementById('ssid').innerText = data.wireless.sta_ssid_;
            document.getElementById('password').innerText = data.wireless.sta_password_;
            document.getElementById('system_name').innerText = data.system.system_name_;
            document.getElementById('name').innerText = data.system.system_name_;
        };
    } catch (error) {
        console.log('Failed to connect to websocket');
        console.log(error);
    }

    if (!document.getElementById('sync_settings').checked) {
        showOpeningSettings();
    }
}

function isMobileDevice() {
    var userAgent = navigator.userAgent || navigator.vendor || window.opera;
    // Mobile device
    if (/android/i.test(userAgent) || /iPhone|iPad|iPod/i.test(userAgent)) {
        document.getElementById('forward_backward').removeChild(document.getElementById('desktop_elements'));
        return true;
    }
    // Desktop device
    document.getElementById('forward_backward').removeChild(document.getElementById('mobile_elements'));
    return false;
}

function toggleAdvancedControls() {
    document.getElementById('motor_controls_body').classList.toggle('hide');
    document.getElementById('advanced_controls').classList.toggle('hide');
}

function toggleMoreSettings() {
    document.getElementById('more_settings').classList.toggle('more-settings-hide');
}

function toggleSettings(setting="") {
    document.getElementById('motor_controls').classList.toggle('motor-controls-hide');
    document.getElementById('more_settings').classList.toggle('more-settings-shift');
    document.getElementById(setting).classList.toggle('default-hide');
    setTimeout(document.getElementById('more_settings_dropdown').checked = false, 1000);
    setTimeout(document.getElementById('more_settings').classList.add('more-settings-hide'), 1000);
}

function httpRequest(uri, param, value=1) {
    const xhr = new XMLHttpRequest();
    xhr.open('GET', `/${uri}?${param}=${value}`, true);
    xhr.send();
}

function motorHttpRequest(param, value=1) {
    httpRequest('motor', param, value);
}

function motorMove(element) {
    motorHttpRequest('percent', element.value);
}

function syncSettings() {
    if (document.getElementById('sync_settings').checked) {
        motorHttpRequest('sync-settings');
        hideOpeningSettings();
    } else {
        motorHttpRequest('sync-settings', 0);
        showOpeningSettings();
    }
}

function checkboxHttpRequest(uri, param) {
    if (document.getElementById(param).checked) {
        httpRequest(uri, param);
    } else {
        httpRequest(uri, param, 0);
    }
}

function openPopupDialog(uri, dialog_prompt, dialog_title, settings, dialog_hint, input_step=0) {
    const form_input = document.getElementById('form_input');
    const closing_setting = document.getElementById('closing_setting');
    const sync_setting = document.getElementById('sync_settings').checked;
    const opening_setting_input = document.getElementById('opening_setting_input');
    let closing_setting_input = document.getElementById('closing_setting_input');

    document.getElementById('dialog_prompt').innerText = `Enter new ${dialog_prompt}`;
    document.getElementById('dialog_title').innerText = `Enter ${dialog_title}`;

    closing_setting.removeChild(closing_setting_input);
    closing_setting_input = document.createElement('input');
    closing_setting_input.setAttribute('id', 'closing_setting_input');
    if (input_step == 0) {
        closing_setting_input.setAttribute('type', 'text');
    } else {
        closing_setting_input.setAttribute('type', 'number');
    }
    closing_setting.appendChild(closing_setting_input);

    if (settings.length > 1 && !sync_setting) {
        form_input.classList.remove('one-setting');
        form_input.classList.add('two-settings');
        document.getElementById('opening_setting').classList.remove('hide');
        document.getElementById('opening_setting_separator').classList.remove('hide');
        closing_setting.classList.add('one-half-pos');
        closing_setting.classList.add('one-half-height');
        closing_setting.classList.remove('whole-height');
        document.getElementById('dialog_hint').classList.add('dialog-hint-shift');

        document.getElementById('opening_setting_name').innerText = settings[0][0];
        document.getElementById('closing_setting_name').innerText = settings[1][0];
        opening_setting_input.placeholder = document.getElementById(settings[0][1]).innerText;
        opening_setting_input.name = settings[0][1].replaceAll("_", "-");
        opening_setting_input.step = input_step;
        opening_setting_input.select();
    } else {
        form_input.classList.add('one-setting');
        form_input.classList.remove('two-settings');
        document.getElementById('opening_setting').classList.add('hide');
        document.getElementById('opening_setting_separator').classList.add('hide');
        closing_setting.classList.remove('one-half-pos');
        closing_setting.classList.remove('one-half-height');
        closing_setting.classList.add('whole-height');
        document.getElementById('dialog_hint').classList.remove('dialog-hint-shift');

        document.getElementById('closing_setting_name').innerText = dialog_title;
        closing_setting_input.select();
    }

    closing_setting_input.placeholder = document.getElementById(settings.at(-1)[1]).innerText;
    if (settings.length > 1 && sync_setting) {
        closing_setting_input.name = settings.at(-1)[1].split("_")[1];
    } else {
        closing_setting_input.name = settings.at(-1)[1].replaceAll("_", "-");
    }
    closing_setting_input.step = input_step;

    document.getElementById('dialog_hint_text').innerText = dialog_hint;

    const popup_dialog = document.getElementById('popup_dialog');
    popup_dialog.uri = uri;
    popup_dialog.classList.add('popup-dialog-show');
}

function cancelForm() {
    document.getElementById('popup_dialog').classList.remove('popup-dialog-show');
}

function submitForm() {
    const opening_setting = document.getElementById('opening_setting_input');
    const closing_setting = document.getElementById('closing_setting_input');
    const uri = document.getElementById('popup_dialog').uri;
    if (closing_setting.value != "") {
        httpRequest(uri, closing_setting.name, closing_setting.value);
    }
    if (opening_setting.value != "") {
        httpRequest(uri, opening_setting.name, opening_setting.value);
    }
    cancelForm();
}

function showOpeningSettings() {
    document.getElementById('opening_current').classList.add('setting-opening-txt-show');
    document.getElementById('opening_velocity').classList.add('setting-opening-txt-show');
    document.getElementById('opening_acceleration').classList.add('setting-opening-txt-show');
}

function hideOpeningSettings() {
    document.getElementById('opening_current').classList.remove('setting-opening-txt-show');
    document.getElementById('opening_velocity').classList.remove('setting-opening-txt-show');
    document.getElementById('opening_acceleration').classList.remove('setting-opening-txt-show');
}