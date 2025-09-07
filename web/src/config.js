import { resetMessage, setDisabled, showMessage } from './utils.js';

let hidePwd = true;
const submitWifi = document.getElementById('submit-wifi');
setDisabled(submitWifi, true);
const feedbackWifiMessage = document.getElementById('submit-wifi-message');
resetMessage(feedbackWifiMessage);
const submitBrightness = document.getElementById('submit-brightness');
setDisabled(submitBrightness, true);
const feedbackBrightnessMessage = document.getElementById('submit-brightness-message');
resetMessage(feedbackBrightnessMessage);
const submitLog = document.getElementById('submit-log');
setDisabled(submitLog, true);
const feedbackLogMessage = document.getElementById('submit-log-message');
resetMessage(feedbackLogMessage);
const ssidInput = document.getElementById('ssid');
const pwdInput = document.getElementById('password');
const pwdToggle = document.getElementById('toggle-password');
const brightnessInput = document.getElementById('brightness');
const logSelect = document.getElementById('log-level');

export const config = (baseUrl) => {
    // Get current config
    const url = new URL('config', baseUrl);
    fetch(url, {
        method: 'GET',
        headers: { 'Accept': 'application/json' },
    }).then(async (res) => {
        const data = await res.text();
        if (!res.ok || res.status != 200) {
            console.error(`Error receiving configuration, status code ${res.status}: ${data}`);

            return;
        }

        const json = JSON.parse(data);
        ssidInput.value = json?.wifi?.ssid;
        pwdInput.value = json?.wifi?.password;
        brightnessInput.value = json?.panel?.brightness || 127;
        logSelect.value = json?.log?.level || 2; // info
    });

    // Handle password toggle
    pwdToggle.addEventListener('click', () => {
        hidePwd = !hidePwd;
        pwdToggle.innerText = hidePwd ? 'show' : 'hide';
        pwdInput.type = hidePwd ? 'password' : 'text';
    });

    // Enable submit on value changes
    const handleInput = (inputElem, submitElem, feedbackElem) => {
        inputElem.addEventListener('input', () => {
            resetMessage(feedbackElem);
            setDisabled(submitElem, false);
        });
    };
    handleInput(ssidInput, submitWifi, feedbackWifiMessage);
    handleInput(pwdInput, submitWifi, feedbackWifiMessage);
    handleInput(brightnessInput, submitBrightness, feedbackBrightnessMessage);
    handleInput(logSelect, submitLog, feedbackLogMessage);

    // Handle submit to server
    const handleSubmit = (submitElem, feedbackElem) => {
        submitElem.addEventListener('click', async () => {
            setDisabled(submitElem, true);
            showMessage(feedbackElem, 'Saving...', 'is-primary');
            const config = {
                wifi: {
                    ssid: ssidInput.value,
                    password: pwdInput.value,
                },
                panel: {
                    brightness: Math.min(255, Math.max(0, brightnessInput.value)),
                },
                log: {
                    level: parseInt(logSelect.value, 10)
                }
            };

            try {
                const url = new URL('config', baseUrl);
                const response = await fetch(url, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(config),
                });
                if (!response.ok || response.status != 204) {
                    throw new Error(`status code ${response.status}: ${await response.text()}`);
                }

                showMessage(feedbackElem, 'Configuration saved', 'is-success');
            } catch (e) {
                console.error('Error sending configuration:', e);
                showMessage(feedbackElem, 'Error saving configuration', 'is-error');
                setDisabled(submitElem, false);
            }
        });
    };
    handleSubmit(submitWifi, feedbackWifiMessage);
    handleSubmit(submitBrightness, feedbackBrightnessMessage);
    handleSubmit(submitLog, feedbackLogMessage);
};
