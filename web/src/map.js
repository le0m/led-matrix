import {
	handleFetch,
	humanFileSize,
	resetMessage,
	setDisabled,
	setProgress,
	showMessage,
	readImage,
	canvasToImage,
	hexToRgb,
	rgbToHex,
} from './utils.js';

const submitFile = document.getElementById('submit-map-file');
setDisabled(submitFile, true);
const feedbackMessage = document.getElementById('submit-map-file-message');
resetMessage(feedbackMessage);
const uploadProgress = document.getElementById('submit-map-progress');
const deleteMap = document.getElementById('delete-map');
const deleteMapConfirm = document.getElementById('delete-map-confirm');
const fileInput = document.getElementById('map-file');
fileInput.value = null;
const fileLabel = fileInput.parentElement.querySelector('span');
const initialFileLabelText = fileLabel.innerText;
const fileRemove = document.getElementById('remove-map-file');
fileRemove.classList.add('is-hidden');
const showGrid = document.getElementById('show-grid');
showGrid.checked = false;
const zoomInput = document.getElementById('map-zoom');
zoomInput.value = 1;
const pointSizeInput = document.getElementById('location-size');
pointSizeInput.value = 2;
const locationColorInput = document.getElementById('location-color');
locationColorInput.value = '#ff0000';
const routeColorInput = document.getElementById('location-route-color');
routeColorInput.value = '#0000ff';
const locationLatitude = document.getElementById('location-latitude');
const locationLongitude = document.getElementById('location-longitude');
const locationUrl = document.getElementById('location-url');
const locationMethod = document.getElementById('location-method');
const locationRegex = document.getElementById('location-regex');
const locationBody = document.getElementById('location-body');
const locationHeaders = document.getElementById('location-headers');
const locationTest = document.getElementById('location-test');
const locationTestMessage = document.getElementById('location-test-message');
resetMessage(locationTestMessage);

/** @type {HTMLCanvasElement} */
const canvas = document.getElementById('map-preview');
canvas.width = 64 * 36;
canvas.height = 64 * 18;
/** @type {CanvasRenderingContext2D} */
const context = canvas.getContext('2d');
context.fillStyle = '#000000ff';
/** @type {HTMLCanvasElement} */
const canvasPanel = document.getElementById('panel-preview');
canvasPanel.width = 64;
canvasPanel.height = 64;
/** @type {CanvasRenderingContext2D} */
const contextPanel = canvasPanel.getContext('2d');
contextPanel.fillStyle = '#000000ff';
let maxSize = 1024 * 1024; // 1 MiB, overridden if disk free size is lower
/** @type {File|undefined} */
let currentFile = undefined;
/** @type {Uint8Array|undefined} */
let imageData;
let positions = [];

const drawGrid = (cellW, cellH) => {
	// Draw latitude and longitude lines on preview
	context.strokeStyle = '#ffffff';
	context.lineWidth = 1;
	// Longitude (vertical)
	for (let x = cellW; x < canvas.width; x += cellW) {
		context.beginPath();
		context.moveTo(x, 0);
		context.lineTo(x, canvas.height);
		context.stroke();
	}
	// Latitude (horizontal)
	for (let y = cellH; y < canvas.height; y += cellH) {
		context.beginPath();
		context.moveTo(0, y);
		context.lineTo(canvas.width, y);
		context.stroke();
	}
};

const handleFile = async (file) => {
	const img = await readImage(file);
	canvas.width = canvasPanel.width * 36;
	canvas.height = canvasPanel.height * 18;
	// Scale to canvas size and show preview
	context.drawImage(img, 0, 0, img.width, img.height, 0, 0, canvas.width, canvas.height);
	imageData = await canvasToImage(canvas).then((blob) => blob.bytes());
	if (imageData.byteLength > maxSize) {
		console.error(
			`Not enough space for storing map (${humanFileSize(imageData.byteLength)}), max size allowed is ${humanFileSize(maxSize)}`,
		);
		showMessage(feedbackMessage, `Resized map exceeds maximum size of ${humanFileSize(maxSize)}`, 'is-error');

		return;
	}

	const cellW = canvasPanel.width * zoomInput.value;
	const cellH = canvasPanel.height * zoomInput.value;
	if (showGrid.checked) {
		drawGrid(cellW, cellH);
	}

	setDisabled(submitFile, false);
	console.log(`Image is ${humanFileSize(imageData.byteLength)}`);
	const [lat, lon] = (await getLatLon()) ?? [];
	if (!lat || !lon) {
		console.log('Skipping position draw');

		return;
	}

	const x = ((lon + 180) / 360) * canvas.width;
	const y = ((90 - lat) / 180) * canvas.height;
	const col = (x / cellW) | 0;
	const row = (y / cellH) | 0;
	const cellX = cellW * col;
	const cellY = cellH * row;
	// Draw panel preview
	contextPanel.drawImage(canvas, cellX, cellY, cellW, cellH, 0, 0, canvasPanel.width, canvasPanel.height);
	// Draw position
	context.fillStyle = locationColorInput.value;
	context.fillRect(x - 4, y - 4, 8, 8);
	context.fillStyle = '#000000'; // restore
	// Visualize a border around the cell on the map preview
	context.strokeStyle = locationColorInput.value;
	context.lineWidth = 4;
	context.strokeRect(cellX, cellY, cellW, cellH);
	context.strokeStyle = '#ffffff'; // restore
	// Scale and draw position on panel preview
	const pX = (x - cellX) * (canvasPanel.width / cellW);
	const pY = (y - cellY) * (canvasPanel.height / cellH);
	contextPanel.fillStyle = locationColorInput.value;
	contextPanel.fillRect(pX - 2, pY - 2, 4, 4);
	contextPanel.fillStyle = '#000000'; // restore
	// TODO: use `routeColorInput` to draw `positions`
};

const getLatLon = async () => {
	let pos = undefined;
	if (locationUrl.value && locationMethod.value && locationRegex.value) {
		const api = await getFromAPI();
		if (api?.length === 2) {
			console.log('Got position from API', api);
			pos = api;
		}
	}

	if (!pos?.every(Boolean) && locationLatitude.value.trim() && locationLongitude.value.trim()) {
		pos = [parseFloat(locationLatitude.value.trim()), parseFloat(locationLongitude.value.trim())];
		console.log('Got position from config', pos);
	}

	return pos;
};

const getFromAPI = async () => {
	if (!locationUrl.value || !locationMethod.value || !locationRegex.value) {
		return undefined;
	}

	const url = new URL(locationUrl.value);
	const method = (locationMethod.value ?? 'GET').toUpperCase();
	let body = undefined;
	if (locationBody.value) {
		if (method === 'GET') {
			url.search = locationBody.value;
		} else {
			body = locationBody.value;
		}
	}

	let headers = new Headers();
	if (locationHeaders.value) {
		headers = new Headers(JSON.parse(locationHeaders.value));
	}
	if (url.username && url.password) {
		const auth = `${url.username}:${url.password}`;
		const b64 = btoa(String.fromCharCode(...new TextEncoder().encode(auth)));
		headers.set('Authorization', `Basic ${b64}`);
		url.username = '';
		url.password = '';
	}

	showMessage(locationTestMessage, 'Sending request...', 'is-primary');
	const res = await handleFetch(url, { method, headers, body });
	if (!res) {
		showMessage(locationTestMessage, 'Error in API request', 'is-error');

		return undefined;
	}

	const text = await res.text();
	const re = new RegExp(locationRegex.value);
	const matches = re.exec(text);
	if (!matches || matches.length === 0) {
		showMessage(locationTestMessage, 'Regex did not match', 'is-error');

		return undefined;
	}

	matches.shift();
	showMessage(locationTestMessage, `Regex matched: ${matches}`, 'is-success');

	return matches.map((m) => parseFloat(m.trim()));
};

/**
 * Initialize map module.
 *
 * @param {import('./state.js').State} state
 */
export const map = (state) => {
	// Fill canvas with black, because the LED panel is black
	context.fillRect(0, 0, canvas.width, canvas.height);

	// Get current image
	state.map().then((map) => {
		if (!map) {
			return;
		}

		handleFile(map.file);
	});

	// Get current config
	state.config().then((config) => {
		if (!config) {
			return;
		}

		locationLatitude.value = config.map.latitude;
		locationLongitude.value = config.map.longitude;
		locationUrl.value = config.map.url;
		locationMethod.value = (config.map.method || 'GET').toUpperCase();
		locationRegex.value = config.map.regex;
		locationBody.value = config.map.body;
		locationHeaders.value = config.map.headers;
		pointSizeInput.value = config.map.pointSize || 2;
		locationColorInput.value = rgbToHex(config.map.pointColor || 63488); // red
		routeColorInput.value = rgbToHex(config.map.trackColor || 31); // blue
		positions = config.map.positions || [];
	});

	// Get current free disk
	state.status().then((status) => {
		if (!status) {
			return;
		}

		const {
			filesystem: { size, used },
		} = status;
		const free = size - used;
		if (free < maxSize) {
			maxSize = (free * 0.9) | 0; // leave some space
		}
	});

	// Clear selected file and canvas
	fileRemove.addEventListener('click', () => {
		fileInput.value = null;
		fileLabel.innerText = initialFileLabelText;
		fileRemove.classList.add('is-hidden');
		currentFile = undefined;
		imageData = undefined;
		// Fill canvas with black, because the LED panel is black
		context.fillRect(0, 0, canvas.width, canvas.height);
		contextPanel.fillRect(0, 0, canvasPanel.width, canvasPanel.height);
		setDisabled(submitFile, true);
		resetMessage(feedbackMessage);
	});

	// Enable submit and redraw map
	showGrid.addEventListener('change', () => {
		if (!currentFile) {
			return;
		}

		handleFile(currentFile);
	});
	[
		zoomInput,
		pointSizeInput,
		locationColorInput,
		routeColorInput,
		locationLatitude,
		locationLongitude,
		locationUrl,
		locationMethod,
		locationRegex,
		locationBody,
	].forEach((elem) =>
		elem.addEventListener('change', () => {
			setDisabled(
				submitFile,
				!(locationLatitude.value && locationLongitude.value) &&
					!(locationUrl.value && locationMethod.value && locationRegex.value),
			);
			if (!currentFile) {
				return;
			}

			handleFile(currentFile);
		}),
	);

	// Test location API
	locationTest.addEventListener('click', () => {
		if (currentFile) {
			handleFile(currentFile);

			return;
		}

		getLatLon();
	});

	// File selected
	fileInput.addEventListener('change', (e) => {
		setDisabled(submitFile, true);
		if (e.target.files.length === 0) {
			fileLabel.innerText = initialFileLabelText;

			return;
		}
		if (e.target.files[0].type !== 'image/jpeg') {
			console.error(`Selected image is not JPEG: ${e.target.files[0].type}`);

			return;
		}

		// Fill canvas with black, because the LED panel is black
		context.fillRect(0, 0, canvas.width, canvas.height);
		resetMessage(feedbackMessage);
		currentFile = e.target.files[0];
		fileLabel.innerText = currentFile.name;
		fileRemove.classList.remove('is-hidden');
		handleFile(currentFile);
	});

	// Send map image and config to server
	submitFile.addEventListener('click', async () => {
		setDisabled(submitFile, true);
		if (currentFile) {
			showMessage(feedbackMessage, 'Uploading map...', 'is-primary');

			uploadProgress.classList.remove('is-hidden');
			const success = await state.uploadMap(imageData, currentFile.type, (progress) =>
				setProgress(uploadProgress, (100 * progress.current) / progress.total, 'is-primary'),
			);
			uploadProgress.classList.add('is-hidden');
			setProgress(uploadProgress, undefined);
			if (!success) {
				showMessage(feedbackMessage, 'Error uploading map', 'is-error');
				setDisabled(submitFile, false);

				return;
			}

			showMessage(feedbackMessage, 'Map uploaded', 'is-success');
		}

		showMessage(feedbackMessage, 'Saving...', 'is-primary');
		const res = await state.saveConfig({
			map: {
				latitude: locationLatitude.value,
				longitude: locationLongitude.value,
				url: locationUrl.value,
				method: (locationMethod.value || 'GET').toUpperCase(),
				regex: locationRegex.value,
				body: locationBody.value,
				headers: locationHeaders.value,
				pointSize: pointSizeInput.value,
				pointColor: hexToRgb(locationColorInput.value || '#ff0000'),
				trackColor: hexToRgb(routeColorInput.value || '#0000ff'),
			},
		});
		if (!res) {
			showMessage(feedbackMessage, 'Error saving configuration', 'is-error');
			setDisabled(submitFile, false);

			return;
		}

		showMessage(feedbackMessage, 'Saved', 'is-success');
	});

	// Delete map
	deleteMap.addEventListener('click', () => deleteMapConfirm.showModal());
	deleteMapConfirm.addEventListener('close', async () => {
		const confirmed = parseInt(deleteMapConfirm.returnValue, 10);
		if (!confirmed) {
			return;
		}

		setDisabled(submitFile, true);
		setDisabled(deleteMap, true);
		showMessage(feedbackMessage, 'Deleting map...', 'is-primary');
		setDisabled(deleteMap, false);
		if (!(await state.deleteMap())) {
			showMessage(feedbackMessage, 'Error deleting map', 'is-error');

			return;
		}

		showMessage(feedbackMessage, 'Map deleted', 'is-success');
		if (imageData?.byteLength) {
			setDisabled(submitFile, false);
		}
	});
};
