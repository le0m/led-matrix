import './style.css';
import { State } from './state.js';
import { media } from './media.js';
import { map } from './map.js';
import { config } from './config.js';
import { firmware } from './firmware.js';
import { websocket } from './websocket.js';
import { system } from './system.js';

(async () => {
	const baseUrl = new URL(window.location.origin);
	const state = new State(baseUrl);
	await state.preload(false);
	media(state);
	map(state);
	config(state);
	firmware(state);
	websocket(baseUrl);
	system(state);
})();
