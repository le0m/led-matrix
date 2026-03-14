/**
 * Disable or enable an input.
 *
 * @param {HTMLInputElement} elem Element
 * @param {boolean} disabled Whether to disable or enable
 */
export const setDisabled = (elem, disabled = false) => {
	elem.disabled = disabled;
	if (disabled) {
		elem.classList.add('is-disabled');
	} else {
		elem.classList.remove('is-disabled');
	}
};

/**
 * Show a feedback message. If a delay is provided, the element is reset after that time.
 *
 * @param {HTMLSpanElement} elem Span message element
 * @param {string} message Message to show
 * @param {string} className Optional class to add to the span
 * @param {number|null} delay Optional time to wait before reset; null to not reset
 * @returns {void}
 */
export const showMessage = (elem, message, className = null, delay = null) => {
	resetMessage(elem, false);
	elem.innerText = message;
	if (className !== null) {
		elem.classList.add(className);
	}
	if (delay !== null) {
		setTimeout(() => resetMessage(elem), delay);
	}
};

/**
 * Reset a feedback message element.
 *
 * @param {HTMLSpanElement} elem Span message element
 * @returns {void}
 */
export const resetMessage = (elem, hidden = true) => {
	elem.className = 'nes-text';
	elem.innerText = '';
	if (hidden) {
		elem.classList.add('is-hidden');
	}
};

/**
 * Convert bytes to human-readable file sizes.
 *
 * @param {string|number} size Size in bytes
 * @returns {string}
 */
export const humanFileSize = (size) => {
	size = parseInt(size, 10);
	const i = size === 0 ? 0 : Math.floor(Math.log(size) / Math.log(1024));

	return `${Number((size / Math.pow(1024, i)).toFixed(2))} ${['B', 'kB', 'MB', 'GB', 'TB'][i]}`;
};

/**
 * Wait some time.
 *
 * @param {number} ms Milliseconds
 * @returns {Promise<void>}
 */
export const wait = (ms) => new Promise((res) => setTimeout(res, ms));

/**
 * Wrapper for fetch() that handles errors by logging and returning undefined.
 *
 * @param {URL} url
 * @param {RequestInit} options
 * @returns {Promise<Response|undefined>}
 */
export const handleFetch = async (url, options = {}) => {
	try {
		const resp = await fetch(url, options);
		if (!resp.ok) {
			throw new Error(`Status code ${resp.status}`, { cause: { response: resp } });
		}

		return resp;
	} catch (e) {
		console.error(`Error fetching ${url.toString()}: ${e.message}`, e);

		return undefined;
	}
};

/**
 * Read a blob into an Image.
 *
 * @param {Blob} data
 * @returns {Promise<HTMLImageElement>}
 */
export const readImage = (data) =>
	new Promise((res, rej) => {
		const objUrl = window.URL.createObjectURL(data);
		const img = new Image();
		img.onerror = (e) => rej(e);
		img.onload = () => res(img);
		img.src = objUrl;
	});

/**
 * Convert the canvas to an image blob.
 *
 * @param {HTMLCanvasElement} canvas
 * @param {string} type
 * @returns {Promise<Blob>}
 */
export const canvasToImage = (canvas, type = 'image/jpeg') =>
	new Promise((res, rej) =>
		canvas.toBlob((blob) => {
			if (!blob) {
				rej('Error generating scaled image');

				return;
			}

			res(blob);
		}, type),
	);

/**
 * Convert hex string to rgb565.
 *
 * @param {string} hex Hexadecimal color
 * @returns {number}
 */
export const hexToRgb = (hex) => {
	const rgb = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/.exec(hex);
	if (rgb === null) {
		console.error(`Hex to rgb regex failed: ${hex}`);

		return null;
	}

	const r = parseInt(rgb[1], 16);
	const g = parseInt(rgb[2], 16);
	const b = parseInt(rgb[3], 16);

	return ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3);
};

/**
 * Convert rgb565 to hex string.
 *
 * @param {number} rgb RGB565 color
 * @returns {string}
 */
export const rgbToHex = (rgb) => {
	const rgb565 = [(rgb & 0xf800) >> 11, (rgb & 0x07e0) >> 5, rgb & 0x001f];
	const rgb888 = [
		Math.floor((rgb565[0] * 255) / 31 + 0.5),
		Math.floor((rgb565[1] * 255) / 63 + 0.5),
		Math.floor((rgb565[2] * 255) / 31 + 0.5),
	];

	return `#${rgb888.map((b) => b.toString(16).padStart(2, '0')).join('')}`;
};

/**
 * Set progress value and corresponding CSS class to the progress bar.
 *
 * @param {HTMLProgressElement} elem
 * @param {number|string} progress
 * @param {string} force Force a "is-*" class
 */
export const setProgress = (elem, progress = undefined, force = undefined) => {
	if (progress === undefined) {
		elem.removeAttribute('value');
	} else {
		elem.value = progress;
	}

	['success', 'warning', 'error', 'pattern'].forEach((c) => elem.classList.remove(`is-${c}`));

	if (force !== undefined) {
		elem.classList.add(force);

		return;
	}

	if (progress === undefined) {
		elem.classList.add('is-pattern');
	} else if (progress <= 50) {
		elem.classList.add('is-success');
	} else if (progress <= 75) {
		elem.classList.add('is-warning');
	} else {
		elem.classList.add('is-error');
	}
};
