import { humanFileSize, resetMessage, setDisabled, showMessage } from './utils.js';
import { parseGIF, decompressFrames } from 'gifuct-js';
import { quantize, applyPalette, prequantize, GIFEncoder, nearestColorIndexWithDistance } from 'gifenc';

/**
 * Original code taken from https://github.com/javl/image2cpp
 */

const submitFile = document.getElementById('submit-file');
setDisabled(submitFile, true);
const feedbackMessage = document.getElementById('submit-file-message');
resetMessage(feedbackMessage);
const fileInput = document.getElementById('file');
fileInput.value = null;
const fileLabel = fileInput.parentElement.querySelector('span');
const initialFileLabelText = fileLabel.innerText;
const fileRemove = document.getElementById('remove-file');
fileRemove.classList.add('is-hidden');
const gifPreview = document.getElementById('gif-preview');
gifPreview.classList.add('is-hidden');
/** @type {HTMLCanvasElement} */
const canvas = document.getElementById('drawer-preview');
canvas.width = 64;
canvas.height = 64;
/** @type {CanvasRenderingContext2D} */
const context = canvas.getContext('2d');
context.fillStyle = '#000000ff';
/** @type {Uint8Array|null} */
let result = null;

// GIF stuff
const tmpCanvas = new OffscreenCanvas(canvas.width, canvas.height);
const tmpContext = tmpCanvas.getContext('2d');
/** @type {ImageData|undefined} */
let frameImageData;
/** @type {import('gifuct-js').ParsedFrame|undefined} */
let previousFrameData;
/** @type {ImageData|undefined} */
let previousImageData;
/** @type {number} pixel ratio (canvas / GIF) */
let ratio = 1;
let offsetX, offsetY;
let gifEncoder;
/** @type {Uint8Array|undefined} */
let gifData;
let maxSize = 1024 * 1024; // 1 MiB, overridden if disk free size is lower

/**
 * @param {File} file
 */
const loadGif = async (file) => {
    const start = Date.now();
    showMessage(feedbackMessage, 'Generating GIF...', 'is-primary');
    const gif = parseGIF(await file.arrayBuffer());
    const frames = decompressFrames(gif, true);
    // Get pixel ratio of first frame
    ratio = Math.min(canvas.width / frames[0].dims.width, canvas.height / frames[0].dims.height);
    const scaledGifW = gif.lsd.width * ratio;
    const scaledGifH = gif.lsd.height * ratio;
    offsetX = scaledGifW < canvas.width
        ? ((canvas.width - scaledGifW) / 2)
        : 0;
    offsetY = scaledGifH < canvas.height
        ? ((canvas.height - scaledGifH) / 2)
        : 0;
    gifEncoder = new GIFEncoder({ auto: true });
    console.log(`Rendering GIF: ${frames.length} frames, ${(frames.reduce((tot, frm) => tot + frm.delay, 0) / 1000).toPrecision(2)}s, ${(file.size / 1024)|0} KiB`);
    for (let i = 0; i < frames.length; i++) {
        const frameData = renderFrame(frames[i], i === 0);
        encodeFrame(frameData, frames[i].delay);
    }

    gifData = finishScaledGif();
    console.log(`Rendering took ${(Date.now() - start) / 1000}s, result file is ${humanFileSize(gifData.byteLength)}`);
    if (gifData.byteLength > maxSize) {
        console.error(`GIF size is ${(gifData.byteLength / 1024)|0} KiB, max size allowed is ${humanFileSize(maxSize)}`);
        showMessage(feedbackMessage, `Scaled down GIF exceeds maximum size of ${humanFileSize(maxSize)}`, 'is-error');

        return;
    }

    resetMessage(feedbackMessage);
    setDisabled(submitFile, false);
};

const resetGif = () => {
    frameImageData = undefined;
    previousFrameData = undefined;
    previousImageData = undefined;
    gifData = undefined;
    gifEncoder = undefined;
};

/**
 * Adapted from:
 *  - https://github.com/deanm/omggif/pull/31/files
 *  - https://github.com/matt-way/gifuct-js/blob/55cecc4d1ca5ea073f962a73ec31582e392fcdde/demo/demo.js
 */
const renderFrame = (frame, first = false) => {
    if (previousFrameData) {
        switch (previousFrameData.disposalType) {
            case 0: // "No disposal specified"
            case 1: // "Do not dispose"
                // do nothing, we draw over the existing canvas
                break;

            case 2: // "Restore to background"
                // fill with black, because LED panel is black
                // NOTE: only the frame's area, not the whole canvas
                context.fillRect(
                    (previousFrameData.dims.left * ratio) + offsetX,
                    (previousFrameData.dims.top * ratio) + offsetY,
                    previousFrameData.dims.width * ratio,
                    previousFrameData.dims.height * ratio
                );
                break;

            case 3: // "Restore to previous"
                // revert back to most recent frame that was not set to "Restore to previous", or frame 0
                if (previousImageData) {
                    context.putImageData(previousImageData, 0, 0);
                }
                break;

            default:
                console.error(`Unknown disposal method: ${previousFrameData.disposalType}`);
        }
    }
    if (first || frame.disposalType < 2) {
        previousImageData = context.getImageData(0, 0, canvas.width, canvas.height);
    }

    if (!frameImageData || frame.dims.width != frameImageData.width || frame.dims.height != frameImageData.height) {
        tmpCanvas.width = frame.dims.width;
        tmpCanvas.height = frame.dims.height;
        frameImageData = tmpContext.createImageData(tmpCanvas.width, tmpCanvas.height);
    }

    frameImageData.data.set(frame.patch);
    tmpContext.putImageData(frameImageData, 0, 0);

    // Center and scale frame
    const dstW = frame.dims.width * ratio;
    const dstH = frame.dims.height * ratio;
    const dstX = (frame.dims.left * ratio) + offsetX;
    const dstY = (frame.dims.top * ratio) + offsetY;

    context.drawImage(tmpCanvas, dstX, dstY, dstW, dstH);
    previousFrameData = frame;

    return context.getImageData(0, 0, canvas.width, canvas.height);
};

/**
 * @param {ImageData} imageData
 * @param {number} delay
 */
const encodeFrame = (imageData, delay) => {
    const { data, width, height } = imageData;
    const palette = quantize(data, 256, { format: 'rgb565' });
    const index = applyPalette(data, palette, 'rgb565');
    gifEncoder.writeFrame(index, width, height, { palette, delay });
};

/**
 *
 * @return {Uint8Array}
 */
const finishScaledGif = () => {
    gifEncoder.finish();
    const blob = new Blob([gifEncoder.bytes()], { type: 'image/gif' });
    const reader = new FileReader();
    reader.addEventListener('load', () => {
        gifPreview.src = reader.result;
        canvas.classList.add('is-hidden');
        gifPreview.classList.remove('is-hidden');
    });
    reader.readAsDataURL(blob);

    return gifEncoder.bytes();
};

export const image = (baseUrl) => {
    // Fill canvas with black, because the LED panel is black
    context.fillRect(0, 0, canvas.width, canvas.height);

    // Get current image
    let url = new URL('drawer', baseUrl);
    fetch(url).then(async (res) => {
        if (!res.ok) {
            console.error(`Error receiving current image, status code ${res.status}: ${await res.text()}`);

            return;
        }

        const imageData = context.getImageData(0, 0, canvas.width, canvas.height);
        const data = await res.bytes();
        let readIndex = 0;
        // Add alpha values for ImageData
        for (let i = 0; i < imageData.data.length; i++) {
            if ((i + 1) % 4 === 0) {
                imageData.data[i] = 255
                continue;
            }

            imageData.data[i] = data[readIndex];
            readIndex++;
        }

        context.putImageData(imageData, 0, 0);
    });

    // Get current free disk
    url = new URL('status', baseUrl);
    fetch(url).then(async (res) => {
        if (!res.ok || res.status != 200) {
            console.error(`Error getting current status, status code ${res.status}: ${await res.text()}`);

            return;
        }

        try {
            const { filesystem: { size, used } } = await res.json();
            const free = size - used;
            if (free < maxSize) {
                maxSize = (free * 0.9)|0; // leave some space
            }
        } catch (e) {
            console.error(`Error fetching status`, e);
        }
    });

    // Clear selected file and canvas
    fileRemove.addEventListener('click', () => {
        fileInput.value = null;
        fileLabel.innerText = initialFileLabelText;
        fileRemove.classList.add('is-hidden');
        result = null;
        resetGif();
        // Fill canvas with black, because the LED panel is black
        context.fillRect(0, 0, canvas.width, canvas.height);
        setDisabled(submitFile, true);
        resetMessage(feedbackMessage);
        canvas.classList.remove('is-hidden');
        gifPreview.classList.add('is-hidden');
    });

    // File selected
    fileInput.addEventListener('change', (e) => {
        if (e.target.files.length === 0) {
            fileLabel.innerText = initialFileLabelText;

            return;
        }

        // Fill canvas with black, because the LED panel is black
        context.fillRect(0, 0, canvas.width, canvas.height);
        setDisabled(submitFile, true);
        resetMessage(feedbackMessage);
        resetGif();
        result = null;
        canvas.classList.remove('is-hidden');
        gifPreview.classList.add('is-hidden');
        const file = e.target.files[0];
        fileLabel.innerText = file.name;
        fileRemove.classList.remove('is-hidden');
        if (file.name.endsWith('.gif')) {
            loadGif(file);

            return;
        }

        // Convert image to array of 8bit values
        const reader = new FileReader();
        reader.onload = (file) => {
            const img = new Image();
            img.onload = () => {
                // Center and scale image to canvas size keeping ratio
                const imgW = img.width;
                const imgH = img.height;
                const ratio = Math.min(canvas.width / imgW, canvas.height / imgH);
                const dstW = (imgW * ratio) |0;
                const dstH = (imgH * ratio) |0;
                const dstX = dstW < canvas.width
                    ?  ((canvas.width - dstW) / 2) |0
                    : 0
                const dstY = dstH < canvas.height
                    ? ((canvas.height - dstH) / 2) |0
                    : 0;

                // Draw preview
                context.drawImage(img, 0, 0, imgW, imgH, dstX, dstY, dstW, dstH);
                // Prepare data
                const imageData = context.getImageData(0, 0, canvas.width, canvas.height);
                // Remove alpha values from ImageData
                result = new Uint8Array(canvas.width * canvas.height * 3);
                let resultIndex = 0;
                for (let i = 0; i < imageData.data.length; i++) {
                    if ((i + 1) % 4 === 0) {
                        continue;
                    }

                    result[resultIndex] = imageData.data[i];
                    resultIndex++;
                }

                if (result.byteLength > maxSize) {
                    console.error(`Not enough space for storing image (${humanFileSize(result.byteLength)}), max size allowed is ${humanFileSize(maxSize)}`);
                    showMessage(feedbackMessage, `Converted image exceeds maximum size of ${humanFileSize(maxSize)}`, 'is-error');

                    return;
                }

                resetMessage(feedbackMessage);
                setDisabled(submitFile, false);
                console.log(`Image data is ${result.byteLength} bytes`/* , [...result].map(b => b.toString(16).padStart(2, '0')).join(', ') */);
            };
            img.src = file.target.result;
        };
        reader.readAsDataURL(file);
    });

    // Send file bytes to server
    submitFile.addEventListener('click', async () => {
        setDisabled(submitFile, true);
        showMessage(feedbackMessage, 'Uploading...', 'is-primary');
        try {
            let url = new URL('drawer', baseUrl);
            let data, contentType, length;
            if (result) {
                console.log('uploading image');
                data = result;
                contentType = 'application/octet-stream';
                length = result.byteLength;
            } else if (gifData) {
                console.log('uploading GIF');
                data = gifData;
                contentType = 'image/gif';
                length = gifData.byteLength;
            } else {
                console.error('Neither image or gif selected');

                return;
            }

            const response = await fetch(url, {
                method: 'POST',
                headers: { 'Content-Type': contentType, 'Content-Length': length },
                body: data,
            });
            if (!response.ok || response.status != 204) {
                throw new Error(`status code ${response.status}: ${await response.text()}`);
            }

            showMessage(feedbackMessage, 'File uploaded', 'is-success');
        } catch (e) {
            console.error('Error uploading file:', e);
            showMessage(feedbackMessage, 'Error uploading file', 'is-error');
            setDisabled(submitFile, false);
        }
    });
};
