import { resetMessage, setDisabled, showMessage, handleFetch } from './utils.js';

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

/**
 * Initialize config module.
 *
 * @param {import('./state.js').State} state
 */
export const config = (state) => {
	// Get current config
	state.config().then((config) => {
		if (!config) {
			return;
		}

		ssidInput.value = config.wifi.ssid;
		pwdInput.value = config.wifi.password;
		brightnessInput.value = config.panel.brightness || 127;
		logSelect.value = config.log.level || 2; // info
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
			const res = await state.saveConfig({
				wifi: {
					ssid: ssidInput.value,
					password: pwdInput.value,
				},
				panel: {
					brightness: Math.min(255, Math.max(0, brightnessInput.value)),
				},
				log: {
					level: parseInt(logSelect.value, 10),
				},
			});
			if (!res) {
				showMessage(feedbackElem, 'Error saving configuration', 'is-error');
				setDisabled(submitElem, false);

				return;
			}

			showMessage(feedbackElem, 'Configuration saved', 'is-success');
		});
	};
	handleSubmit(submitWifi, feedbackWifiMessage);
	handleSubmit(submitBrightness, feedbackBrightnessMessage);
	handleSubmit(submitLog, feedbackLogMessage);
};
