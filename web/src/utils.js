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
