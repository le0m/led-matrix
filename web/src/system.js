import { humanFileSize } from "./utils";

/**
 * @typedef SystemStatus
 * @property {{size: number, free: number, minFree: number, maxFreeBlock: number}} heap
 * @property {number} flash
 * @property {{size: number, used: number, version: string}} firmware
 * @property {{size: number, used: number}} filesystem
 * @property {{model: string, revision: string, cores: number, clock: number}} chip
 * @property {string} sdk
 */

let pullTimer = undefined;
const hwMsg = document.getElementById('hardware-message');
const swMsg = document.getElementById('software-message');
const heap = document.getElementById('heap-perc');
const fs = document.getElementById('fs-perc');
const fw = document.getElementById('firmware-perc');
const heapInfo = document.getElementById('heap-info');
const fsInfo = document.getElementById('fs-info');
const fwInfo = document.getElementById('fw-info');

/**
 * Get current status.
 *
 * @param {string} baseUrl API URL
 * @returns {Promise<SystemStatus>}
 */
const getStatus = async (baseUrl) => {
    const url = new URL('status', baseUrl);
    const res = await fetch(url)
    if (!res.ok || res.status != 200) {
        throw new Error(`Error getting current status, status code ${res.status}: ${await res.text()}`);
    }

    return await res.json();
};

const setProgress = (elem, progress) => {
    elem.value = progress;
    ['success', 'warning', 'error', 'pattern'].forEach((c) => elem.classList.remove(`is-${c}`));
    if (progress <= 50) {
        elem.classList.add('is-success');
    } else if (progress <= 75) {
        elem.classList.add('is-warning');
    } else {
        elem.classList.add('is-error');
    }
};

const loop = async (baseUrl) => {
    const status = await getStatus(baseUrl);

    // Update hardware/software info
    hwMsg.innerText = `Running on a ${status.chip.model} (rev. ${status.chip.revision}) with ${status.chip.cores} CPU clocking at ${status.chip.clock} MHz`;
    swMsg.innerText = `Firmware version ${status.firmware.version}, SDK version ${status.sdk}`;

    // Update resources usage
    const heapPerc = 100 * (status.heap.size - status.heap.free) / status.heap.size;
    const fsPerc = 100 * status.filesystem.used / status.filesystem.size;
    const fwPerc = 100 * status.firmware.used / status.firmware.size;
    setProgress(heap, heapPerc);
    setProgress(fs, fsPerc);
    setProgress(fw, fwPerc);
    heapInfo.innerText = `(${humanFileSize(status.heap.size - status.heap.free)} / ${humanFileSize(status.heap.size)})`;
    fsInfo.innerText = `(${humanFileSize(status.filesystem.used)} / ${humanFileSize(status.filesystem.size)})`;
    fwInfo.innerText = `(${humanFileSize(status.firmware.used)} / ${humanFileSize(status.firmware.size)})`;
};

export const system = (baseUrl) =>
    loop(baseUrl).then(() => pullTimer = setTimeout(() => loop(baseUrl), 60_000));
