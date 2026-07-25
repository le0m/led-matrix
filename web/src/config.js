import { resetMessage, setDisabled, showMessage, handleFetch, humanFileSize, humanTimeInterval } from './utils.js';

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
const submitMonitor = document.getElementById('submit-monitor');
setDisabled(submitMonitor, true);
const feedbackMonitorMessage = document.getElementById('submit-monitor-message');
resetMessage(feedbackMonitorMessage);
const submitStatusLed = document.getElementById('submit-statusled');
setDisabled(submitStatusLed, true);
const feedbackStatusLedMessage = document.getElementById('submit-statusled-message');
resetMessage(feedbackStatusLedMessage);
const ssidInput = document.getElementById('ssid');
const pwdInput = document.getElementById('password');
const pwdToggle = document.getElementById('toggle-password');
const brightnessInput = document.getElementById('brightness');
const logSelect = document.getElementById('log-level');
const monitorCheckbox = document.getElementById('monitor-enable');
const monitorMessages = document.getElementById('monitor-messages');
const refreshMonitor = document.getElementById('refresh-monitor');
const statusLedCheckbox = document.getElementById('statusled-enable');

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
		monitorCheckbox.checked = (config.monitor?.enable ?? 0) === 1;
		statusLedCheckbox.checked = (config.statusled?.enable ?? 1) === 1;
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
	monitorCheckbox.addEventListener('change', () => {
		resetMessage(feedbackMonitorMessage);
		setDisabled(submitMonitor, false);
	});
	statusLedCheckbox.addEventListener('change', () => {
		resetMessage(feedbackStatusLedMessage);
		setDisabled(submitStatusLed, false);
	});

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

	submitMonitor.addEventListener('click', async () => {
		setDisabled(submitMonitor, true);
		showMessage(feedbackMonitorMessage, 'Saving...', 'is-primary');
		const res = await state.saveConfig({
			monitor: { enable: monitorCheckbox.checked ? 1 : 0 },
		});
		if (!res) {
			showMessage(feedbackMonitorMessage, 'Error saving configuration', 'is-error');
			setDisabled(submitMonitor, false);

			return;
		}

		showMessage(feedbackMonitorMessage, 'Configuration saved', 'is-success');
	});

	submitStatusLed.addEventListener('click', async () => {
		setDisabled(submitStatusLed, true);
		showMessage(feedbackStatusLedMessage, 'Saving...', 'is-primary');
		const res = await state.saveConfig({
			statusled: { enable: statusLedCheckbox.checked ? 1 : 0 },
		});
		if (!res) {
			showMessage(feedbackStatusLedMessage, 'Error saving configuration', 'is-error');
			setDisabled(submitStatusLed, false);

			return;
		}

		showMessage(feedbackStatusLedMessage, 'Configuration saved', 'is-success');
	});

	refreshMonitor.addEventListener('click', async () => {
		try {
			const res = await fetch('/monitor');
			if (!res.ok) {
				monitorMessages.value = 'Monitor not available';

				return;
			}

			const text = await res.text();
			const records = text.split('\n').filter((l) => l.trim().length > 0);
			records.sort((a, b) => parseInt(a, 10) - parseInt(b, 10));
			monitorMessages.value = records
				.map((line) => {
					const match = /^(\d+)\s+f=(\d+)\s+m=(\d+)\s+b=(\d+)\s+w=(\w)$/.exec(line.trim());
					if (!match) {
						return line;
					}

					const [, ms, free, min, max, wifi] = match;

					return `${humanTimeInterval(parseInt(ms, 10))} f=${humanFileSize(free)} m=${humanFileSize(min)} b=${humanFileSize(max)} w=${wifi}`;
				})
				.join('\n');
		} catch (e) {
			monitorMessages.value = 'Monitor not available';
		}
	});
};
