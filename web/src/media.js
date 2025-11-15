import {
	humanFileSize,
	resetMessage,
	setDisabled,
	showMessage,
	readImage,
	canvasToImage,
	setProgress,
} from './utils.js';
import { parseGIF, decompressFrames } from 'gifuct-js';
import { quantize, applyPalette, GIFEncoder } from 'gifenc';

/**
 * Original code taken from https://github.com/javl/image2cpp
 */

const submitFile = document.getElementById('submit-file');
setDisabled(submitFile, true);
const feedbackMessage = document.getElementById('submit-file-message');
resetMessage(feedbackMessage);
const uploadProgress = document.getElementById('submit-file-progress');
const deleteMedia = document.getElementById('delete-media');
const deleteMediaConfirm = document.getElementById('delete-media-confirm');
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
/** @type {Uint8Array|undefined} */
let imageData;
let maxSize = 1024 * 1024; // 1 MiB, overridden if disk free size is lower

/**
 * @param {File} file
 */
const loadImage = async (file) => {
	const img = await readImage(file);
	// Center and scale image to canvas size keeping ratio
	const imgW = img.width;
	const imgH = img.height;
	const ratio = Math.min(canvas.width / imgW, canvas.height / imgH);
	const dstW = (imgW * ratio) | 0;
	const dstH = (imgH * ratio) | 0;
	const dstX = dstW < canvas.width ? ((canvas.width - dstW) / 2) | 0 : 0;
	const dstY = dstH < canvas.height ? ((canvas.height - dstH) / 2) | 0 : 0;

	// Draw preview
	context.drawImage(img, 0, 0, imgW, imgH, dstX, dstY, dstW, dstH);
	imageData = await canvasToImage(canvas).then((blob) => blob.bytes());
	if (imageData.byteLength > maxSize) {
		console.error(
			`Not enough space for storing image (${humanFileSize(imageData.byteLength)}), max size allowed is ${humanFileSize(maxSize)}`,
		);
		showMessage(feedbackMessage, `Resized image exceeds maximum size of ${humanFileSize(maxSize)}`, 'is-error');

		return;
	}

	setDisabled(submitFile, false);
	console.log(`Image is ${humanFileSize(imageData.byteLength)}`);
};

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
	offsetX = scaledGifW < canvas.width ? (canvas.width - scaledGifW) / 2 : 0;
	offsetY = scaledGifH < canvas.height ? (canvas.height - scaledGifH) / 2 : 0;
	gifEncoder = new GIFEncoder({ auto: true });
	console.log(
		`Rendering GIF: ${frames.length} frames, ${(frames.reduce((tot, frm) => tot + frm.delay, 0) / 1000).toPrecision(2)}s, ${humanFileSize(file.size)}`,
	);
	for (let i = 0; i < frames.length; i++) {
		const frameData = renderFrame(frames[i], i === 0);
		encodeFrame(frameData, frames[i].delay);
	}

	gifData = finishScaledGif();
	console.log(`Rendering took ${(Date.now() - start) / 1000}s, result file is ${humanFileSize(gifData.byteLength)}`);
	if (gifData.byteLength > maxSize) {
		console.error(`GIF size is ${humanFileSize(gifData.byteLength)}, max size allowed is ${humanFileSize(maxSize)}`);
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
					previousFrameData.dims.left * ratio + offsetX,
					previousFrameData.dims.top * ratio + offsetY,
					previousFrameData.dims.width * ratio,
					previousFrameData.dims.height * ratio,
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
	const dstX = frame.dims.left * ratio + offsetX;
	const dstY = frame.dims.top * ratio + offsetY;

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

/**
 * Initialize media module.
 *
 * @param {import('./state.js').State} state
 */
export const media = (state) => {
	// Fill canvas with black, because the LED panel is black
	context.fillRect(0, 0, canvas.width, canvas.height);

	// Get current image
	state.media().then(async (media) => {
		if (!media) {
			return;
		}

		if (media.contentType === 'image/gif') {
			await loadGif(media.file);
		} else if (media.contentType.startsWith('image/')) {
			await loadImage(media.file);
		} else {
			console.error(`Unhandled content type for current image: ${media.contentType}`);
		}

		setDisabled(submitFile, true);
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
		resetGif();
		imageData = undefined;
		// Fill canvas with black, because the LED panel is black
		context.fillRect(0, 0, canvas.width, canvas.height);
		setDisabled(submitFile, true);
		resetMessage(feedbackMessage);
		canvas.classList.remove('is-hidden');
		gifPreview.classList.add('is-hidden');
	});

	// File selected
	fileInput.addEventListener('change', async (e) => {
		setDisabled(submitFile, true);
		if (e.target.files.length === 0) {
			fileLabel.innerText = initialFileLabelText;

			return;
		}

		const file = e.target.files[0];

		if (!file.type.startsWith('image/')) {
			console.error(`Selected media is not an image: ${e.target.files[0].type}`);
			showMessage(feedbackMessage, 'Only image files are supported', 'is-error');

			return;
		}

		// Fill canvas with black, because the LED panel is black
		context.fillRect(0, 0, canvas.width, canvas.height);
		resetMessage(feedbackMessage);
		resetGif();
		imageData = undefined;
		canvas.classList.remove('is-hidden');
		gifPreview.classList.add('is-hidden');
		fileLabel.innerText = file.name;
		fileRemove.classList.remove('is-hidden');
		if (file.type === 'image/gif') {
			loadGif(file);

			return;
		}

		loadImage(file);
	});

	// Send file bytes to server
	submitFile.addEventListener('click', async () => {
		setDisabled(submitFile, true);
		showMessage(feedbackMessage, 'Uploading...', 'is-primary');
		let data, contentType;
		if (imageData) {
			data = imageData;
			contentType = 'image/jpeg';
		} else if (gifData) {
			data = gifData;
			contentType = 'image/gif';
		} else {
			console.error('Neither image or gif selected');

			return;
		}

		uploadProgress.classList.remove('is-hidden');
		const success = await state.uploadMedia(data, contentType, (progress) =>
			setProgress(uploadProgress, (100 * progress.current) / progress.total, 'is-primary'),
		);
		uploadProgress.classList.add('is-hidden');
		setProgress(uploadProgress, undefined);
		if (!success) {
			showMessage(feedbackMessage, 'Error uploading file', 'is-error');
			setDisabled(submitFile, false);

			return;
		}

		showMessage(feedbackMessage, 'File uploaded', 'is-success');
	});

	// Delete image/gif
	deleteMedia.addEventListener('click', () => deleteMediaConfirm.showModal());
	deleteMediaConfirm.addEventListener('close', async () => {
		const confirmed = parseInt(deleteMediaConfirm.returnValue, 10);
		if (!confirmed) {
			return;
		}

		setDisabled(submitFile, true);
		setDisabled(deleteMedia, true);
		showMessage(feedbackMessage, 'Deleting media...', 'is-primary');
		setDisabled(deleteMedia, false);
		if (!(await state.deleteMedia())) {
			showMessage(feedbackMessage, 'Error deleting media', 'is-error');

			return;
		}

		showMessage(feedbackMessage, 'Media deleted', 'is-success');
		if (imageData?.byteLength || gifData?.byteLength) {
			setDisabled(submitFile, false);
		}
	});
};
