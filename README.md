# Tiny Robot

Tiny Robot is a two-wheel line-following robot controlled by an STM32. It supports automatic line following, manual web control with a live camera view, speed adjustment, and QR-triggered stopping with an animal picture on an OLED display.

## Main files

| Device | File | Function |
|---|---|---|
| STM32 | `stm32_robot_controller/stm32_robot_controller.ino` | Main robot firmware; upload this to STM32 |
| ESP32 | `esp32_remote_controller/esp32_remote_controller.ino` | Wi-Fi access point, browser remote, OLED display |
| ESP32-CAM | `esp32_camera_server/esp32_camera_server.ino` | Camera server and QR signal bridge |
| PC | `pc_qr_scanner/qr_scanner.py` | QR detection and UDP sender; run on the PC |
| Art | `assets/oled/*.png` | Source artwork for every OLED screen, 128x64 each |
| Tool | `tools/png_to_bitmaps.py` | Converts that artwork into `esp32_remote_controller/animal_bitmaps.h` |

## Capabilities

- Five-sensor black-line following with a PD controller.
- Manual forward, backward, left, right, and stop control from a phone browser.
- Live camera image embedded in the remote web page.
- Remote speed control and Auto/Manual mode switching.
- Two-motor control through a DRV8833 H-bridge.
- Recovery when the line is temporarily lost.
- Finish-line detection and active braking.
- Animal QR detection for DOG, CAT, BIRD, LION, and TIGER.
- Five-second stop after a valid QR detection, with the matching animal drawn on the OLED and shown on the web page.

## OLED screens

The display switches between five screens on its own. Every screen except Status is a full-screen 128x64 bitmap with its lettering already drawn into the artwork, so the firmware only blits the image.

| Screen | When | Source |
|---|---|---|
| Welcome | First 2.5 s after boot (`START_SCREEN_MS`) | `Start.png` |
| Status | Robot stopped | Drawn as text: `MODE`, `SPEED`, camera IP (or `waiting`) |
| Scanning | Whole time the robot is moving | `scan1.png`–`scan4.png` looping at `SCAN_FRAME_MS` |
| Found | First 1 s after a QR hit (`DETECT_SHOW_MS`) | `Detected.png` |
| Animal | Rest of the 5 s stop | `Dog.png`, `cat.png`, `Bird.png`, `Lion.png`, `Tiger.png` |

"Moving" is inferred on the remote from the last command it forwarded: any direction button or `MODE:AUTO` sets it, `stop` and `MODE:MANUAL` clear it. The whole sequence is driven by `updateScreen()`, which redraws only when a frame is actually due — the still screens are drawn once and left alone.

## Installation

Install Arduino IDE. Add the STM32duino Board Manager URL in `File > Preferences`:

```text
https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json
```

Then install `STM32 MCU based boards` from `Tools > Board > Boards Manager`, select the exact STM32 board, and upload the main sketch. Install the ESP32 board package before uploading the ESP32 sketches.

Install these Arduino libraries from `Tools > Manage Libraries` for the ESP32 remote:

- `WebSockets` by Markus Sattler (Links2004)
- `Adafruit GFX Library`
- `Adafruit SH110X` — the display on this robot is an SH1106
- `Adafruit SSD1306` — only if you switch `OLED_DRIVER_SH1106` back to 0

For the PC QR scanner, install Python 3 and run:

```bash
python -m pip install opencv-python numpy pyzbar requests
```

## Running the system

1. Upload `stm32_robot_controller/stm32_robot_controller.ino` to the STM32.
2. Upload `esp32_remote_controller/esp32_remote_controller.ino` to the ESP32.
3. Upload the complete `esp32_camera_server/` sketch to the AI Thinker ESP32-CAM. It joins the robot's own Wi-Fi automatically.
4. Connect the phone to Wi-Fi `My_Robot` with password `password1234`, then open `http://192.168.4.1`. The camera image appears once the ESP32-CAM has joined.
5. Connect the PC to the same `My_Robot` Wi-Fi. Read the camera IP from the `CAM:` line on the web page or from the ESP32-CAM serial monitor, then run:

```bash
python pc_qr_scanner/qr_scanner.py 192.168.4.2
```

The IP argument is optional and defaults to `192.168.4.2`.

The STM32 can also work by itself for motor control and line following. The ESP32 is required for the web remote and the OLED, and the ESP32-CAM plus PC are required for QR stopping.

## Communication protocol

- ESP32 remote → STM32: `Serial2`, 115200 baud, newline-terminated: `forward`, `backward`, `left`, `right`, `stop`, `MODE:AUTO`, `MODE:MANUAL`, `SPEED:<value>`.
- PC → ESP32-CAM: UDP port `1234`; `DOG→D`, `CAT→C`, `BIRD→B`, `LION→L`, `TIGER→T`.
- ESP32-CAM → STM32: `Serial1` to `Serial3`, 9600 baud; a valid animal code becomes `S`, which triggers a five-second stop.
- ESP32-CAM → ESP32 remote: UDP port `1235`; the animal code itself, so the OLED knows which picture to draw. The camera also sends `P` every two seconds so the remote learns the camera IP for the live image.
- ESP32 remote → browser: WebSocket port `81`; `CAMIP:<ip>` and `ANIMAL:<name>`.

## Wiring

### STM32 pin mapping

| Function | Pin |
|---|---|
| Left motor AIN1/AIN2 | PA0 / PA1 |
| Right motor BIN1/BIN2 | PB0 / PB1 |
| Line sensors L2/L1/C/R1/R2 | PB9 / PB8 / PB7 / PB6 / PB5 |
| Serial2 (from ESP32 remote) | PA2 TX / PA3 RX |
| Serial3 (from ESP32-CAM) | PB10 TX / PB11 RX |

The physical pins for `Serial2` and `Serial3` depend on the STM32 board/core configuration. Verify them before wiring. Cross UART signals (TX→RX, RX→TX) and share GND.

### ESP32 remote

| Function | Pin |
|---|---|
| OLED SDA | GPIO21 |
| OLED SCL | GPIO22 |
| To STM32 Serial2 RX (PA3) | GPIO17 (TX) |
| From STM32 Serial2 TX (PA2) | GPIO16 (RX) |

The OLED is an I2C 128x64 module at address `0x3C`. Change `OLED_ADDR` in the sketch if your module uses `0x3D`. If the display is missing, the sketch prints a warning and everything else still runs.

### Display controller

Identical-looking I2C OLED modules carry one of two controllers, and they are not interchangeable. `OLED_DRIVER_SH1106` at the top of `esp32_remote_controller.ino` selects which one, and everything downstream — library, display object, `begin()` call, colour constants — follows from it.

| Value | Controller | Typical module |
|---|---|---|
| 0 | SSD1306 | most 0.96" panels |
| 1 | SH1106 | most 1.3" panels — **this robot's display** |

Getting it wrong is not a subtle failure but it is a confusing one: an SH1106 driven by the SSD1306 library shows the image squeezed into the top strip of the panel with the rest left as power-on noise. The SSD1306 library uses horizontal addressing mode, which SH1106 does not implement, so the whole 1024-byte frame lands in page 0 instead of walking down all eight pages.

Three sketches under `tests/` diagnose this: `test_oled_display` sweeps the I2C clock to rule out wiring, `test_oled_sh1106` tries the SH110X driver, and `test_oled_raw` talks to the panel with bare `Wire` commands and page addressing to report its true height and column offset.

### ESP32-CAM

| Function | Pin |
|---|---|
| To STM32 Serial3 RX (PB11) | GPIO14 (TX) |

## Screen artwork

`esp32_remote_controller/animal_bitmaps.h` is generated, not hand-written. The real source is the PNG files in `assets/oled/`, one per screen, each 128x64 — the same size as the display. To change what the robot shows, edit the PNG and run:

```bash
py tools/png_to_bitmaps.py
```

The filename becomes the variable name, so `Dog.png` becomes `bmp_dog`. The script prints an ASCII preview of every converted image and rewrites the header. Eleven images at 1024 bytes each use about 11 KB of PROGMEM.

Conversion to 1-bit uses **Otsu's method**, which picks the light/dark cut from each image's own histogram rather than using a fixed value. This matters because the scan frames are drawn dim — their brightest pixel is only 84–142 out of 255 — so a fixed cut at 128 would render them as solid black.

Adding an animal means adding a PNG here, a row in the `ANIMALS` table in `esp32_remote_controller.ino`, an entry in `ANIMAL_MAP` in `pc_qr_scanner/qr_scanner.py`, and one in `isAnimalCode()` in `esp32_camera_server.ino`.

## Repository layout

- `tests/` contains hardware tests.
- `experiments/` contains old controllers, tuning versions, and prototypes.
- `examples/esp32cam_examples/` contains unrelated ESP32-CAM examples.
- `tools/` contains the artwork converter.
- `assets/oled/` contains the source PNGs for the display.

The current controller uses `Kp=80`, `Kd=30`, and a default `baseSpeed=180` that the web slider overrides (clamped to 120–255). It is a PD controller, not a full PID controller.

## Motor torque tuning

The motor drive itself is left exactly as in the version that is confirmed to run in the correct direction on this hardware: one pin held low, the other PWMed. Do not swap the pin pairs in `setSpeed()` — that reverses both motors, which flips forward/backward, flips left/right, and makes line following drive away from the line.

Weak-feeling wheels are handled by the constants at the top of `stm32_robot_controller.ino` instead:

| Constant | Default | What it does |
|---|---|---|
| `MIN_PWM` | 135 | Floor applied to any non-zero command, so the wheel always clears the breakaway point instead of just buzzing. This is the value from the previously working controller. Raise in steps of 10 if the robot still will not start moving, lower it if the slowest setting is too fast to steer. |
| `LEFT_MOTOR_OFFSET` | 25 | Subtracted from the left motor when the two motors are not equally strong and the robot drifts on a straight run. Also from the previously working controller. Set to 0 for no compensation. |

`MIN_SPEED` follows `MIN_PWM`, and the web slider's `min` attribute is set to the same number. Change all three together.

If the wheels are still weak after tuning `MIN_PWM`, the cause is electrical rather than firmware: check the battery under load (motors sag a pack that looks fine at rest), confirm the motor supply does not come from the STM32 regulator, confirm the DRV8833 `nSLEEP` pin is pulled high, and check that the driver is not going into thermal shutdown.

## Confirming which firmware is running

`setup()` prints the compile timestamp:

```text
STM32 robot ready | build Sep 11 2026 16:42:03
```

Open the serial monitor at 115200 and reset the board. If the timestamp does not match the upload that was just made, the upload did not take — on a Blue Pill the usual cause is BOOT0 left at 1, which reboots into the bootloader instead of running the program.
