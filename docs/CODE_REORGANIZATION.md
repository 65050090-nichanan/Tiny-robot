# Tiny Robot Code Reorganization

## Production

- `stm32_robot_controller/stm32_robot_controller.ino`: main STM32 firmware with five line sensors, PWM motor control, PD line following, Auto/Manual modes, remote commands, QR stop, braking, finish-line detection, and line recovery.
- `esp32_remote_controller/esp32_remote_controller.ino`: ESP32 Wi-Fi AP, web remote with live camera view, Serial2 bridge to STM32, and SSD1306 OLED that draws the detected animal.
- `esp32_camera_server/esp32_camera_server.ino`: AI Thinker camera server, `/capture`, `:81/stream`, UDP receiver, Serial1 QR-stop bridge, and UDP forward of the animal code to the remote.
- `pc_qr_scanner/qr_scanner.py`: PC QR scanner using the clean protocol `DOG→D`, `CAT→C`, `BIRD→B`, `LION→L`, `TIGER→T`.
- `tools/make_animal_bitmaps.py`: generates `esp32_remote_controller/animal_bitmaps.h`, the five 64x64 monochrome animal images.

The camera support files are kept with the camera sketch: `camera_web_server.cpp`, `camera_web_ui.h`, `camera_pins.h`, `partitions.csv`, and `ci.json`.

## Tests

`tests/` contains `test_stm32_drv8833_motors`, `test_motor_hbridge`, `test_esp32_led`, `test_esp32_to_stm32_serial`, and `test_esp32cam_uart`. Each Arduino sketch has a matching folder and filename.

## Experiments and examples

`experiments/` contains the previous STM32 controllers, PWM/PD and speed tuning sketches, the old WebSocket prototype, and the newer QR scanner that sends `S` directly. `examples/esp32cam_examples/` contains copied ESP32-CAM projects unrelated to the production robot. No source code was intentionally deleted.

## Protocol

All three boards now share one network: the ESP32 remote is the access point `My_Robot` and the ESP32-CAM joins it as a client, so the remote web page can embed the camera stream.

The PC sends `D/C/B/L/T` to ESP32-CAM UDP port `1234`. ESP32-CAM converts a valid code to `S` and sends it to STM32 through Serial1 at 9600 baud, and forwards the original animal code to the remote on UDP port `1235`. STM32 receives `S` on Serial3 and stops for five seconds. The remote draws the matching bitmap on the OLED and pushes `ANIMAL:<name>` to the browser. The camera also pings UDP `1235` every two seconds so the remote learns its IP and can publish `CAMIP:<ip>`. The ESP32 remote sends newline-terminated motion, mode, and speed commands through Serial2 at 115200 baud.

## Build risks

- Select the exact STM32 board and verify the physical Serial2/Serial3 pins. `Serial3` must be enabled by the selected variant.
- Verify motor-driver wiring, PWM capability, common GND, voltage levels, and crossed TX/RX.
- Do not compile the separate UART test together with the camera production sketch.
- The ESP32 remote needs the `WebSockets`, `Adafruit SSD1306`, and `Adafruit GFX` libraries.
- The camera stream serves one client at a time; running the PC scanner against `/capture` while the phone watches `:81/stream` uses the two separate servers, so both work.
- The control algorithm is PD (`Kp=80`, `Kd=30`), despite older comments calling it PID.
