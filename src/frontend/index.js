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
            console.log('Websocket error');
            console.log(error);
        };

        websocket.onmessage = (event) => {
            const data = event.data;
            console.log(data);
            if (data > -1) {
                document.getElementById('percent_slider').value = data;
            }
        };
    } catch (error) {
        console.log('Failed to connect to websocket');
        console.log(error);
    }

    if (document.getElementById('sync_settings').checked) {
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
    document.getElementById('controls').classList.toggle('hide-hlp');
    document.getElementById('advanced_controls').classList.toggle('hide-hlp');
}

function openSettingDialog(setting_name, prompt="", closing_setting_text, input_step, number_of_settings=1) {
    const lowercase_name = setting_name.toLowerCase();
    const param = lowercase_name.replaceAll(" ", "-");
    const id = lowercase_name.replaceAll(" ", "_");
    const form_input = document.getElementById('form_input');
    const close_setting = document.getElementById('closing_setting');
    const opening_setting_input = document.getElementById('opening_setting_input');
    const closing_setting_input = document.getElementById('closing_setting_input');

    if (number_of_settings > 1 && document.getElementById('sync_settings').checked) {
        form_input.classList.remove('one-setting');
        form_input.classList.add('two-settings');
        document.getElementById('opening_setting').classList.remove('hide-hlp');
        document.getElementById('opening_setting_separator').classList.remove('hide-hlp');
        close_setting.classList.add('one-half-pos');
        close_setting.classList.add('one-half-height');
        close_setting.classList.remove('whole-height');
        document.getElementById('dialog_hint').classList.add('dialog-hint-shift');

        document.getElementById('closing_setting_text').innerText = "Closing";
        opening_setting_input.step = input_step;
        opening_setting_input.name = `opening-${param}`;
        closing_setting_input.name = `closing-${param}`;
        opening_setting_input.placeholder = `${document.getElementById(id + '_setting_opening').innerText}`;
        opening_setting_input.select();
    } else {
        form_input.classList.add('one-setting');
        form_input.classList.remove('two-settings');
        document.getElementById('opening_setting').classList.add('hide-hlp');
        document.getElementById('opening_setting_separator').classList.add('hide-hlp');
        close_setting.classList.remove('one-half-pos');
        close_setting.classList.remove('one-half-height');
        close_setting.classList.add('whole-height');
        document.getElementById('dialog_hint').classList.remove('dialog-hint-shift');

        document.getElementById('closing_setting_text').innerText = closing_setting_text;
        opening_setting_input.name = "";
        closing_setting_input.name = param;
        closing_setting_input.select();
    }

    closing_setting_input.placeholder = `${document.getElementById(id + '_setting_closing').innerText}`;
    closing_setting_input.step = input_step;

    document.getElementById('dialog_setting_prompt').innerText = `Enter new value for "${prompt}"`;
    document.getElementById('dialog_setting_name').innerText = `Enter ${setting_name}`;

    const setting_dialog = document.getElementById('setting_dialog');
    setting_dialog.action = '/motor?';
    setting_dialog.classList.add('setting-dialog-show');
}

function cancelForm() {
    document.getElementById('setting_dialog').classList.remove('setting-dialog-show');
}

function submitForm() {
    const open_setting = document.getElementById('opening_setting_input');
    const close_setting = document.getElementById('closing_setting_input');
    const action = document.getElementById('setting_dialog').action;
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
    document.getElementById('current_setting_opening').classList.add('setting-opening-txt-show');
    document.getElementById('velocity_setting_opening').classList.add('setting-opening-txt-show');
    document.getElementById('acceleration_setting_opening').classList.add('setting-opening-txt-show');
}

function hideOpeningSettings() {
    document.getElementById('current_setting_opening').classList.remove('setting-opening-txt-show');
    document.getElementById('velocity_setting_opening').classList.remove('setting-opening-txt-show');
    document.getElementById('acceleration_setting_opening').classList.remove('setting-opening-txt-show');
}

function syncSettings() {
    if (document.getElementById('sync_settings').checked) {
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