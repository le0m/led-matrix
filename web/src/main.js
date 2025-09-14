import './style.css';
import { image } from './image.js';
import { map } from './map.js';
import { config } from './config.js';
import { firmware } from './firmware.js';
import { websocket } from './websocket.js';
import { system } from './system.js';

(() => {
    const baseUrl = new URL(window.location.origin);
    image(baseUrl);
    map(baseUrl);
    config(baseUrl);
    firmware(baseUrl);
    websocket(baseUrl);
    system(baseUrl);
})();
