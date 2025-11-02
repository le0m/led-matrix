import { handleFetch } from './utils.js';

/**
 * @typedef SystemStatus
 * @property {{ size: number, free: number, minFree: number, maxFreeBlock: number }} heap
 * @property {number} flash
 * @property {{ size: number, used: number, version: string }} firmware
 * @property {{ size: number, used: number }} filesystem
 * @property {{ model: string, revision: string, cores: number, clock: number }} chip
 * @property {string} sdk
 */

/**
 * @typedef MapConfiguration
 * @property {string} latitude
 * @property {string} longitude
 * @property {string} url
 * @property {string} method
 * @property {string} regex
 * @property {string} body
 * @property {string} headers
 * @property {string} pointSize
 * @property {number} pointColor
 * @property {number} trackColor
 * @property {{ latitude: string, longitude: string }[]} positions
 */

/**
 * @typedef Configuration
 * @property {{ ssid: string, password: string }} wifi
 * @property {{ brightness: number }} panel
 * @property {{ level: number }} log
 * @property {MapConfiguration} map
 */

export class State {
	#baseUrl;
	#statusPromise = null;
	#configPromise = null;
	#mediaPromise = null;
	#mapPromise = null;

	constructor(baseUrl) {
		this.#baseUrl = baseUrl;
	}

	/**
	 * Get a File from a response.
	 *
	 * @param {Response} response
	 * @returns {Promise<File | undefined>}
	 */
	static async #fileFromResponse(response) {
		const contentType = response.headers.get('Content-Type');
		let file;
		switch (contentType) {
			case 'image/jpeg':
				file = new File([await response.blob()], 'media.jpg', { type: 'image/jpeg' });
				break;

			case 'image/gif':
				file = new File([await response.blob()], 'media.gif', { type: 'image/gif' });
				break;

			default:
				console.error(`Unhandled media type ${contentType}`);

				return undefined;
		}

		return file;
	}

	/**
	 * Clear stored state.
	 */
	clear() {
		this.#statusPromise = null;
		this.#configPromise = null;
		this.#mediaPromise = null;
		this.#mapPromise = null;
	}

	/**
	 * Preload state. Pass `false` to reduce load on the server.
	 *
	 * @param {boolean} parallel
	 * @returns {Promise<[SystemStatus | undefined, Configuration | undefined, { file: File, contentType: string } | undefined, { file: File, contentType: string } | undefined]>}
	 */
	preload(parallel = true) {
		if (parallel) {
			return Promise.all([this.status(), this.config(), this.media(), this.map()]);
		}

		return [this.status, this.config, this.media, this.map].reduce(
			(p, f) =>
				p.then((r) =>
					f
						.bind(this)()
						.then((s) => [...r, s])
						.catch((e) => {
							console.error(`Error preloading data: ${e.toString()}`);

							return r;
						}),
				),
			Promise.resolve([]),
		);
	}

	/**
	 * Upload a file with progress callback.
	 *
	 * Uses XMLHttpRequest because fetch with streamable responses requires HTTP 2 (for half mutex), but the server does not support it.
	 *
	 * @param {URL} url
	 * @param {Uint8Array} data
	 * @param {string} type
	 * @param {({ total: number, current: number }) => void} progress
	 * @returns {Promise<boolean>}
	 */
	#uploadFile(url, data, type, progress = undefined) {
		return new Promise((resolve, reject) => {
			const req = new XMLHttpRequest();
			req.upload.addEventListener('progress', (event) => {
				if (!event.lengthComputable) {
					return;
				}

				progress?.({ total: event.total, current: event.loaded });
			});
			req.addEventListener('load', () => {
				resolve(req.readyState === 4 && req.status >= 200 && req.status < 300);
			});
			req.addEventListener('error', () => {
				reject('Error uploading media');
			});

			req.open('POST', url);
			req.setRequestHeader('Content-Type', type);
			req.send(data);
		});
	}

	/**
	 * Get system status.
	 *
	 * @property {boolean} refresh
	 * @returns {Promise<SystemStatus | undefined>}
	 */
	status(refresh = false) {
		if (this.#statusPromise && !refresh) {
			return this.#statusPromise;
		}

		console.log('Fetching status');

		return (this.#statusPromise = handleFetch(new URL('status', this.#baseUrl)).then(async (res) => {
			try {
				const text = await res.text();
				return JSON.parse(text);
			} catch (_) {
				return undefined;
			}
		}));
	}

	/**
	 * Get configuration.
	 *
	 * @property {boolean} refresh
	 * @returns {Promise<Configuration | undefined>}
	 */
	config(refresh = false) {
		if (this.#configPromise && !refresh) {
			return this.#configPromise;
		}

		console.log('Fetching config');

		return (this.#configPromise = handleFetch(new URL('config', this.#baseUrl)).then(async (res) => {
			try {
				const text = await res.text();
				return JSON.parse(text);
			} catch (_) {
				return undefined;
			}
		}));
	}

	/**
	 * Save configuration.
	 *
	 * @param {Partial<Configuration>} config
	 * @returns {Promise<Configuration | undefined>}
	 */
	async saveConfig(config) {
		const res = await handleFetch(new URL('config', this.#baseUrl), {
			method: 'POST',
			headers: { 'Content-Type': 'application/json' },
			body: JSON.stringify(config),
		});

		if (!res) {
			return undefined;
		}

		return this.config(true);
	}

	/**
	 * Get media.
	 *
	 * @property {boolean} refresh
	 * @returns {Promise<{ file: File, contentType: string } | undefined>}
	 */
	media(refresh = false) {
		if (this.#mediaPromise && !refresh) {
			return this.#mediaPromise;
		}

		console.log('Fetching media');

		return (this.#mediaPromise = handleFetch(new URL('drawer', this.#baseUrl), {
			headers: { Accept: 'image/jpeg' },
		}).then(async (res) => {
			if (!res || res.status === 204 || res.headers.get('Content-Length') === '0') {
				// no media stored
				return undefined;
			}

			const file = await State.#fileFromResponse(res);
			if (!file) {
				return undefined;
			}

			return {
				file,
				contentType: res.headers.get('Content-Type'),
			};
		}));
	}

	/**
	 * Upload media file.
	 *
	 * @param {Uint8Array} data
	 * @param {string} type
	 * @param {({ total: number, current: number }) => void} progress
	 * @returns {Promise<boolean>}
	 */
	uploadMedia(data, type, progress = undefined) {
		return this.#uploadFile(new URL('drawer', this.#baseUrl), data, type, progress);
	}

	/**
	 * Delete media file.
	 *
	 * @returns {Promise<boolean>}
	 */
	async deleteMedia() {
		return (await handleFetch(new URL('drawer', this.#baseUrl), { method: 'DELETE' })) !== undefined;
	}

	/**
	 * Get map.
	 *
	 * @property {boolean} refresh
	 * @returns {Promise<{ file: File, contentType: string } | undefined>}
	 */
	map(refresh = false) {
		if (this.#mapPromise && !refresh) {
			return this.#mapPromise;
		}

		console.log('Fetching map');

		return (this.#mapPromise = handleFetch(new URL('map', this.#baseUrl), { headers: { Accept: 'image/jpeg' } }).then(
			async (res) => {
				if (!res || res.status === 204 || res.headers.get('Content-Length') === '0') {
					// no media stored
					return undefined;
				}

				const file = await State.#fileFromResponse(res);
				if (!file) {
					return undefined;
				}

				return {
					file,
					contentType: res.headers.get('Content-Type'),
				};
			},
		));
	}

	/**
	 * Upload map file.
	 *
	 * @param {Uint8Array} data
	 * @param {string} type
	 * @param {({ total: number, current: number }) => void} progress
	 * @returns {Promise<boolean>}
	 */
	async uploadMap(data, type, progress = undefined) {
		return this.#uploadFile(new URL('map', this.#baseUrl), data, type, progress);
	}

	/**
	 * Delete map file.
	 *
	 * @returns {Promise<boolean>}
	 */
	async deleteMap() {
		return (await handleFetch(new URL('map', this.#baseUrl), { method: 'DELETE' })) !== undefined;
	}

	/**
	 * Upload OTA file.
	 *
	 * @param {File} file
	 * @param {string} mode
	 * @param {({ total: number, current: number }) => void} progress
	 * @returns {Promise<boolean>}
	 */
	async uploadOTA(file, mode, progress = undefined) {
		const url = new URL('ota', this.#baseUrl);
		url.searchParams.set('mode', mode);
		url.searchParams.set('size', file.size);
		if (!(await handleFetch(url))) {
			return false;
		}

		if (!(await this.#uploadFile(new URL('ota', this.#baseUrl), file, 'application/octet-stream', progress))) {
			return false;
		}

		return true;
	}
}
