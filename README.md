This is a personal project to control a LED matrix from an ESP32 microcontroller.

# Components

My LED panel is 19,2x19,2cm, so I'm using a 25x25x3cm frame with a 3D-printed passepartout. I've screwed 4 screws through the frame's backpanel into the LED panel's screw holes, with spacers to keep the panel away from the backpanel and pressed against the glass frame.

The gyroscope is glued inside with the chip facing upwards (on the inner border of the frame), while the esp32 and the wires are free-moving (but not too much since it's packed in there) behind the LED panel :D

For powering it I'm using a random USB charger (5V 3A output): I've cut the mini-USB head and wired the esp32 board and LED panel (plus the gyroscope's ground because the esp32's were full). The panel actually uses up to 4-5A (depending on "brand", size and brightness) and the esp32 uses 0.5-1A, so I'm looking for a small power supply that can accomodate that, but it's currently working so ¯\\_(ツ)_/¯

See [gallery](#gallery) for photos.

## Hardware

- an ESP32 board/kit (ex. AZ-Delivery's [ESP32 Dev KitC v2](https://www.az-delivery.de/en/products/esp32-developmentboard))
- a HUB-75/HUB-75E RGB LED matrix (ex. [this](https://www.waveshare.com/wiki/RGB-Matrix-P3-64x64) one, I'm using off-brand ones from AliExpress)
- an MPU6050 3-axis gyroscope + accelerator (ex. AZ-Delivery's [GY-521](https://www.az-delivery.de/en/products/gy-521-6-achsen-gyroskop-und-beschleunigungssensor))
- a display frame to put everything in (the passepartout model is in `led-matrix-passepartout.stl`)

## Software

- [AdafruitGFX](https://github.com/adafruit/Adafruit-GFX-Library), as a requirement of other libraries for rendering
- [ESP32-HUB75-MatrixPanel-DMA](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA), for controlling the LED matrix
- [FastLED](https://github.com/FastLED/FastLED), for color conversion functions and other utilities
- the ESP32Async project and their [AsyncTCP](https://github.com/ESP32Async/AsyncTCP)/[ESPAsyncWebServer](https://github.com/ESP32Async/ESPAsyncWebServer) libraries
- [ArduinoJSON](https://github.com/bblanchon/ArduinoJson), for JSON de/serialization
- [AnimatedGIF](https://github.com/bitbank2/AnimatedGIF), for decoding and drawing GIFs
- project Nayuki's [qr-code-generator](https://github.com/nayuki/QR-Code-generator), for generating QR codes (included in `lib/` directory)
- [i2cdevlib](https://github.com/jrowberg/i2cdevlib), for reading the MPU6050 sensor (included in `lib/` directory)
- [NES.css](https://github.com/nostalgic-css/NES.css), for the NES-styled theme

Thanks to all of them :)

## Note on PlatformIO

PlatformioIO does not update espressif in their releases, and currently (`platformio/platform-espressif32 @ 6.10.0`) uses version `2.0.17` (see github discussion [here](https://github.com/platformio/platform-espressif32/issues/1225#issuecomment-2245116163)).

If a newer version is really needed, use the [pioarduino](https://github.com/pioarduino/platform-espressif32) fork for espressif 3+. I tried it to have [this](https://github.com/espressif/arduino-esp32/pull/10217) fix, but switching caused the firmware bin to go from ~1 MB to ~1.2 MB.
This is why the `pathExists()` method exists.

# Usage

On first boot the esp32 creates a WiFi network with name `esp32-led-matrix` and password `daftpunk` (hardcoded in `src/wifi.h`). Connect to it, turn the frame upside down and scan the QR code to visit the web UI, where the WiFi name and password can be configured (stored in plain JSON in the flash memory). The esp32 will then connect to the provided WiFi and show a different QR code to visit the web UI on that network.

## Display modes

The LED panel has 4 display modes, depending on the frame orientation:
- ↑ shows image or GIF uploaded through the web UI
- → shows a [Conway's Game of Life](https://en.wikipedia.org/wiki/Conway's_Game_of_Life) game
- ↓ shows the QR code
- ← shows nothing, TBD what to do

## Web UI

- WiFi settings
- brightness (0-255)
- upload image or GIF, with preview of the scaled-down version; this file will be stored in the flash memory and loaded on boot, so the size is limited depending on the model of esp32 used (mine has 1.4MB and I'm reserving 0.4MB for the project's static data)
- upload firmware updates (OTA)
- application logs (by default the logs are sent only if the web UI is loaded by at least 1 user, use the compile variable `LOG_QUEUE` to enable a queue of up to 50 messages that will be delivered once a user connects)

# Gallery

Wiring/PIN layout

<img src="https://github.com/user-attachments/assets/639bfff5-33df-4ccd-8ace-b3b7bd0f3105" alt="Wiring/PIN layout" width="800">

Some LEDs have been harmed in the development process. Our thoughts go to all the dead pixels out there~

<table>
    <tr>
        <td>
            <p>Font</p>
            <img src="https://github.com/user-attachments/assets/127f204d-a744-4bc3-8daa-86bc05b2e7ae" alt="Front" width="300">
        </td>
        <td>
            <p>Back</p>
            <img src="https://github.com/user-attachments/assets/ad2dc76f-5795-4f9d-8d62-73eaa76a2642" alt="Back" width="300">
        </td>
    </tr>
    <tr>
        <td>
            <p>Inside overview</p>
            <img src="https://github.com/user-attachments/assets/89290c72-be9d-48ae-81e2-061d67721fd4" alt="Inside overview" width="300">
        </td>
        <td>
            <p>GY-521 glued to the inner border of the panel frame</p>
            <img src="https://github.com/user-attachments/assets/09560be5-7bf0-49b4-9062-4507ca82acf4" alt="GY-521 glued to the inner border of the panel frame" width="300">
        </td>
    </tr>
    <tr>
        <td>
            <p>ESP32 wires</p>
            <img src="https://github.com/user-attachments/assets/b1536666-e6f3-48b7-b0cc-3fc9d728e89b" alt="ESP32 wires" width="300">
        </td>
        <td>
            <p>Under the LED panel</p>
            <img src="https://github.com/user-attachments/assets/fc978aba-f130-479f-9e3d-3d351aaf3a8e" alt="Under the LED panel" width="300">
        </td>
    </tr>
    <tr>
        <td>
            <p>Image rendering</p>
            <img src="https://github.com/user-attachments/assets/308a4614-1d05-4956-9a51-0c810895d79b" alt="Image rendering" width="300">
        </td>
        <td>
            <p>GIF rendering</p>
            <img src="https://github.com/user-attachments/assets/726eb4c8-b945-4988-a89a-f609bb3e5281" alt="GIF rendering" width="300">
        </td>
    </tr>
    <tr>
        <td>
            <p>Rotation</p>
            <img src="https://github.com/user-attachments/assets/76f52250-7bd9-4076-abf3-0fe4acd02f26" alt="Rotation" width="300">
        </td>
    </tr>
</table>

# TODOs

- add FPS and other Life config
- send current GIF
- modeselector: prevent random wrong reads, use multiple reads in the span of some seconds
