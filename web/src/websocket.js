const logMessages = document.getElementById('log-messages');
logMessages.value = '';

export const websocket = (baseUrl) => {
    let url = new URL('ws', baseUrl);
    url.protocol = 'ws';
    const ws = new WebSocket(url);

    ws.addEventListener('message', (event) => {
        logMessages.value = `[${new Date().toLocaleString()}] ${event.data}\n` + logMessages.value;
    });
};
