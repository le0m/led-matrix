import './style.css';
import { image } from './image.js';
import { config } from './config.js';
import { firmware } from './firmware.js';
import { websocket } from './websocket.js';

(() => {
    const baseUrl = new URL(window.location.origin);
    image(baseUrl);
    config(baseUrl);
    firmware(baseUrl);
    websocket(baseUrl);
})();
