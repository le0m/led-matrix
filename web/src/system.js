import { humanFileSize, setProgress } from './utils';

/**
 * @typedef SystemStatus
 * @property {{size: number, free: number, minFree: number, maxFreeBlock: number}} heap
 * @property {number} flash
 * @property {{found: boolean, size: number, free: number, minFree: number, maxFreeBlock: number}} psram
 * @property {{size: number, used: number, version: string}} firmware
 * @property {{size: number, used: number}} filesystem
 * @property {{model: string, revision: string, cores: number, clock: number}} chip
 * @property {string} sdk
 */

const hwMsg = document.getElementById('hardware-message');
const swMsg = document.getElementById('software-message');
const heap = document.getElementById('heap-perc');
const fs = document.getElementById('fs-perc');
const fw = document.getElementById('firmware-perc');
const psramLabel = document.getElementById('psram-label');
const psram = document.getElementById('psram-perc');
const heapInfo = document.getElementById('heap-info');
const fsInfo = document.getElementById('fs-info');
const fwInfo = document.getElementById('fw-info');
const psramInfo = document.getElementById('psram-info');
const refreshButton = document.getElementById('refresh-resources');

/**
 * Update current status.
 *
 * @param {import('./state.js').State} state
 */
const updateStatus = async (state) => {
	const status = await state.status(true);
	if (!status) {
		return;
	}

	// Update hardware/software info
	hwMsg.innerText = `Running on a ${status.chip.model} (rev. ${status.chip.revision}) with ${status.chip.cores} CPU clocking at ${status.chip.clock} MHz`;
	swMsg.innerText = `Firmware version ${status.firmware.version}, SDK version ${status.sdk}`;

	// Update resources usage
	const heapPerc = (100 * (status.heap.size - status.heap.free)) / status.heap.size;
	const fsPerc = (100 * status.filesystem.used) / status.filesystem.size;
	const fwPerc = (100 * status.firmware.used) / status.firmware.size;
	setProgress(heap, heapPerc);
	setProgress(fs, fsPerc);
	setProgress(fw, fwPerc);
	heapInfo.innerText = `(${humanFileSize(status.heap.size - status.heap.free)} / ${humanFileSize(status.heap.size)})`;
	fsInfo.innerText = `(${humanFileSize(status.filesystem.used)} / ${humanFileSize(status.filesystem.size)})`;
	fwInfo.innerText = `(${humanFileSize(status.firmware.used)} / ${humanFileSize(status.firmware.size)})`;

	if (status.psram.found) {
		const psramPerc = (100 * (status.psram.size - status.psram.free)) / status.psram.size;
		setProgress(psram, psramPerc);
		psramInfo.innerText = `(${humanFileSize(status.psram.size - status.psram.free)} / ${humanFileSize(status.psram.size)})`;
		psramLabel.classList.remove('is-hidden');
	} else {
		psramLabel.classList.add('is-hidden');
	}
};

/**
 * Initialize system module.
 *
 * @param {import('./state.js').State} state
 */
export const system = (state) => {
	updateStatus(state);

	refreshButton.addEventListener('click', () => updateStatus(state));
};
