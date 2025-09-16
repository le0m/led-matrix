import {
    handleFetch,
    humanFileSize,
    resetMessage,
    setDisabled,
    showMessage,
    readImage,
    canvasToJpeg,
    hexToRgb,
    rgbToHex
} from './utils.js';

const submitFile = document.getElementById('submit-map-file');
setDisabled(submitFile, true);
const feedbackMessage = document.getElementById('submit-map-file-message');
resetMessage(feedbackMessage);
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
    imageData = await canvasToJpeg(canvas).then((blob) => blob.bytes());
    if (imageData.byteLength > maxSize) {
        console.error(`Not enough space for storing map (${humanFileSize(imageData.byteLength)}), max size allowed is ${humanFileSize(maxSize)}`);
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
    const col = (x / cellW) |0;
    const row = (y / cellH) |0;
    const cellX = (cellW * col);
    const cellY = (cellH * row);
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
    if (locationUrl.value && locationMethod.value && locationRegex.value) {
        return getFromAPI();
    }

    if (locationLatitude.value.trim() && locationLongitude.value.trim()) {
        return [parseFloat(locationLatitude.value.trim()), parseFloat(locationLongitude.value.trim())];
    }

    return undefined;
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

    let headers = undefined;
    if (locationHeaders.value) {
        headers = JSON.parse(locationHeaders.value);
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

    showMessage(locationTestMessage, 'Regex matched', 'is-success');
    matches.shift();

    return matches.map((m) => parseFloat(m.trim()));
};

export const map = (baseUrl) => {
    // Fill canvas with black, because the LED panel is black
    context.fillRect(0, 0, canvas.width, canvas.height);

    // Get current image
    // TODO: consolidate API calls, this one causes heap allocation issues if run concurrently
    // handleFetch(new URL('map', baseUrl), { headers: { 'Accept': 'image/jpeg' } }).then((resp) => {
    //     if (!resp) {
    //         return;
    //     }

    //     resp.blob().then(handleFile);
    // });

    // Get current config
    handleFetch(new URL('config', baseUrl), {
        method: 'GET',
        headers: { 'Accept': 'application/json' },
    }).then(async (resp) => {
        if (!resp) {
            return;
        }

        const json = await resp.json();
        locationLatitude.value = json?.map?.latitude;
        locationLongitude.value = json?.map?.longitude;
        locationUrl.value = json?.map?.url;
        locationMethod.value = (json?.map?.method ?? 'GET').toUpperCase();
        locationRegex.value = json?.map?.regex;
        locationBody.value = json?.map?.body;
        locationHeaders.value = json?.map?.headers;
        pointSizeInput.value = json?.map?.pointSize;
        locationColorInput.value = rgbToHex(json?.map?.pointColor ?? 63488); // red
        routeColorInput.value = rgbToHex(json?.map?.trackColor ?? 31); // blue
        positions = json?.map?.positions ?? [];
    });

    // Get current free disk
    handleFetch(new URL('status', baseUrl)).then(async (resp) => {
        if (!resp) {
            return;
        }

        const { filesystem: { size, used } } = await resp.json();
        const free = size - used;
        if (free < maxSize) {
            maxSize = (free * 0.9)|0; // leave some space
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
        contextPanel.fillRect(0, 0, canvasPanel.width, canvasPanel.height)
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
        locationBody
    ].forEach((elem) => elem.addEventListener('change', () => {
        setDisabled(
            submitFile,
            !(locationLatitude.value && locationLongitude.value)
            && !(locationUrl.value && locationMethod.value && locationRegex.value)
        );
        if (!currentFile) {
            return;
        }

        handleFile(currentFile);
    }));

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
            showMessage(feedbackMessage, 'Uploading image...', 'is-primary');
            const res = await handleFetch(new URL('map', baseUrl), {
                method: 'POST',
                headers: {
                    'Content-Type': currentFile.type,
                    'Content-Length': imageData.byteLength
                },
                body: imageData
            });
            if (!res) {
                showMessage(feedbackMessage, 'Error uploading image', 'is-error');
                setDisabled(submitFile, false);

                return;
            }

            showMessage(feedbackMessage, 'Image uploaded', 'is-success');
        }

        showMessage(feedbackMessage, 'Saving...', 'is-primary');
        const res = await handleFetch(new URL('config', baseUrl), {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                map: {
                    latitude: locationLatitude.value,
                    longitude: locationLongitude.value,
                    url: locationUrl.value,
                    method: locationMethod.value,
                    regex: locationRegex.value,
                    body: locationBody.value,
                    headers: locationHeaders.value,
                    pointSize: pointSizeInput.value,
                    pointColor: hexToRgb(locationColorInput.value || '#ff0000'),
                    trackColor: hexToRgb(routeColorInput.value || '#0000ff'),
                }
            }),
        });
        if (!res) {
            showMessage(feedbackMessage, 'Error saving configuration', 'is-error');
            setDisabled(submitFile, false);

            return;
        }

        showMessage(feedbackMessage, 'Saved', 'is-success');
    });
};
