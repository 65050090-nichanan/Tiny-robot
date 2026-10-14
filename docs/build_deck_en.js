// Builds the English edition of the Tiny Robot setup deck.
//
// Every figure here is read out of the firmware in this repository, not invented.
// All the drawing lives in deck_kit.js, shared with the Thai edition, so this file
// holds nothing but content: change spacing or colour there and both editions follow.

const { createDeck, THEME, M, W } = require('./deck_kit');

const deck = createDeck({
  title: 'Tiny Robot - Setup and First Run',
  author: 'Tiny Robot',
  subject: 'Setup and operating guide for the line-following robot',
  footer: 'Tiny Robot · Setup and first run',
});
const { pres, C, card, numberDot, step, node, arrow, table, callout, codeBox } = deck;

// ================= SLIDES =================

// --- 1. Cover ---
pres.addSection({ title: 'Overview' });
let s = pres.addSlide({ masterName: 'TITLE_DARK', sectionTitle: 'Overview' });
s.addText('A guide for whoever picks this up next', { placeholder: 'eyebrow' });
s.addText('Tiny Robot', { placeholder: 'title' });
s.addText('A two-wheel robot that follows a black line, stops at a QR code and takes orders from a phone.   Everything from an empty laptop to a run on the course.', { placeholder: 'body' });
s.addText('STM32 Blue Pill  ·  ESP32  ·  ESP32-CAM  ·  Python', {
  x: M, y: 6.3, w: W, h: 0.4, fontSize: 13, color: '6B7E93', align: 'left', isTextBox: true, margin: 0,
});
s.addNotes('This deck is for handing the robot over. It runs from installing the toolchain, through wiring and flashing, to a first drive, and finishes with the faults we actually hit and what each turned out to be.\n\nKeep the robot on the bench while presenting, so each part can be pointed at.');

// --- 2. What it does ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'Overview' });
s.addText('What the robot can do', { placeholder: 'title' });
{
  const cw = (W - 0.4 * 2) / 3, ch = 1.62;
  const items = [
    ['Follows a line', 'Five sensors under a PD controller, bend-tolerant, and it brakes itself at the finish', THEME.colors.accent1, false],
    ['Takes orders', 'Forward, back, turn and stop from any phone browser, with a speed slider', THEME.colors.accent1, false],
    ['Sees', 'The ESP32-CAM streams into the web page and feeds the QR reader on the PC', THEME.colors.accent1, false],
    ['Reads five codes', 'DOG  CAT  BIRD  LION  TIGER, decoded on the PC and sent back', THEME.colors.accent2, false],
    ['Reacts', 'A valid code stops it for five seconds and paints that animal on the OLED', THEME.colors.accent2, false],
    ['Logs itself', 'Speed and steering written to a CSV every second, ready to open in Excel', THEME.colors.accent1, true],
  ];
  items.forEach((it, i) => {
    const col = i % 3, row = Math.floor(i / 3);
    card(s, {
      x: M + col * (cw + 0.4), y: 1.75 + row * (ch + 0.32), w: cw, h: ch,
      head: it[0], body: it[1], accent: it[2], dark: it[3],
    });
  });
}
callout(s, { x: M, y: 5.55, w: W, h: 1.1, tone: 'info',
  title: 'It needs nobody else’s Wi-Fi',
  body: 'The ESP32 on the robot runs its own access point, My_Robot2, and the phone and PC join that directly.\nIt demonstrates anywhere, without asking for a site password and without a slow network to blame.' });
s.addNotes('Make the point that the system is built in layers and does not need all of itself to work.\n\nThe STM32 alone follows the line. The ESP32 adds the remote and the screen. The camera and the PC add QR stopping. Lose any one of them and the rest carries on.');

// --- 3. The four parts ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'Overview' });
s.addText('The four parts', { placeholder: 'title' });
{
  const cw = (W - 0.3 * 3) / 4;
  const items = [
    ['STM32 Blue Pill', 'The brain. Drives the motors, reads the sensors, follows the line', 'stm32_robot_controller'],
    ['ESP32', 'Runs the Wi-Fi access point, the web remote and the OLED', 'esp32_remote_controller'],
    ['ESP32-CAM', 'The eye. Streams to the PC and calls the stop when a code is found', 'esp32_camera_server'],
    ['The PC', 'Decodes the QR codes and writes the run to a CSV on the desktop', 'qr_scanner.py'],
  ];
  items.forEach((it, i) => {
    const x = M + i * (cw + 0.3);
    s.addShape(pres.shapes.ROUNDED_RECTANGLE, {
      x, y: 1.6, w: cw, h: 3.5, rectRadius: 0.1,
      fill: { color: C.background2 }, line: { color: 'D9E0E8', width: 1 },
      objectName: 'part-' + i,
    });
    numberDot(s, { x: x + 0.28, y: 1.88, n: i + 1 });
    s.addText(it[0], {
      x: x + 0.28, y: 2.58, w: cw - 0.56, h: 0.7,
      fontSize: 17, bold: true, color: C.text1, align: 'left', valign: 'top', isTextBox: true, margin: 0,
    });
    s.addText(it[1], {
      x: x + 0.28, y: 3.3, w: cw - 0.56, h: 1.1,
      fontSize: 13, color: '4A5568', align: 'left', valign: 'top', isTextBox: true, margin: 0, lineSpacingMultiple: 1.25,
    });
    s.addText(it[2], {
      x: x + 0.28, y: 4.52, w: cw - 0.56, h: 0.45,
      fontSize: 10.5, color: THEME.colors.accent2, fontFace: 'Courier New',
      align: 'left', valign: 'top', isTextBox: true, margin: 0,
    });
  });
}
callout(s, { x: M, y: 5.35, w: W, h: 1.05, tone: 'info',
  title: 'You can start with less than all of it',
  body: 'The STM32 on its own already follows the line.  Add the ESP32 for the remote and the screen, and bring in the camera and the PC when you get to QR stopping.' });
s.addNotes('The folder under each card is the one to open for that board. Learn those four pairings: uploading the wrong sketch to the wrong board costs an afternoon of confusion.');

// --- 4. Who talks to whom ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'Overview' });
s.addText('Who talks to whom', { placeholder: 'title' });
{
  const nw = 3.2, nh = 1.25, gap = (W - nw * 3) / 2;
  const xs = [M, M + nw + gap, M + (nw + gap) * 2];

  s.addText('Path 1  ·  Driving it by hand', {
    x: M, y: 1.4, w: W, h: 0.3, fontSize: 13, bold: true, color: THEME.colors.accent1,
    align: 'left', isTextBox: true, margin: 0,
  });
  node(s, { x: xs[0], y: 1.78, w: nw, h: nh, title: 'Phone', sub: 'opens 192.168.4.1', fill: C.background2, fg: THEME.colors.dk1 });
  node(s, { x: xs[1], y: 1.78, w: nw, h: nh, title: 'ESP32 remote', sub: 'Wi-Fi AP + OLED', fill: THEME.colors.dk1, fg: 'FFFFFF' });
  node(s, { x: xs[2], y: 1.78, w: nw, h: nh, title: 'STM32', sub: 'motors + sensors', fill: C.background2, fg: THEME.colors.dk1 });
  arrow(s, { x: xs[0] + nw + 0.12, y: 2.3, w: gap - 0.24, label: 'WebSocket 81' });
  arrow(s, { x: xs[1] + nw + 0.12, y: 2.3, w: gap - 0.24, label: 'Serial2 115200' });

  s.addText('Path 2  ·  Reading a code and stopping', {
    x: M, y: 3.5, w: W, h: 0.3, fontSize: 13, bold: true, color: THEME.colors.accent2,
    align: 'left', isTextBox: true, margin: 0,
  });
  node(s, { x: xs[0], y: 3.88, w: nw, h: nh, title: 'The PC', sub: 'qr_scanner.py', fill: C.background2, fg: THEME.colors.dk1 });
  node(s, { x: xs[1], y: 3.88, w: nw, h: nh, title: 'ESP32-CAM', sub: 'camera + bridge', fill: THEME.colors.dk1, fg: 'FFFFFF' });
  node(s, { x: xs[2], y: 3.88, w: nw, h: nh, title: 'STM32', sub: 'stops for 5 seconds', fill: C.background2, fg: THEME.colors.dk1 });
  arrow(s, { x: xs[0] + nw + 0.12, y: 4.4, w: gap - 0.24, label: 'UDP 1234' });
  arrow(s, { x: xs[1] + nw + 0.12, y: 4.4, w: gap - 0.24, label: "Serial 9600  'S'" });
}
callout(s, { x: M, y: 5.5, w: W, h: 1.0, tone: 'info',
  title: 'Two more lines this picture leaves out',
  body: 'The camera pings the remote every 2 seconds so the remote learns its IP and can link to the stream,  and the STM32 sends its state back every 200 ms for the PC to collect into the spreadsheet.' });
s.addNotes('Everything runs on My_Robot2, the network the ESP32 remote creates. No home router is involved.\n\nThe remote is always the gateway, so it is always 192.168.4.1. The camera gets whatever address is free from .2 upward, which is why the Python script hunts for it on every start.');

// --- 5. Divider: Install ---
pres.addSection({ title: 'Install' });
s = pres.addSlide({ masterName: 'SECTION', sectionTitle: 'Install' });
s.addText('01', { placeholder: 'eyebrow' });
s.addText('Install and wire', { placeholder: 'title' });
s.addText('Software on the laptop, wires on the robot, firmware on all three boards', { placeholder: 'body' });

// --- 6. Install the software ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'Install' });
s.addText('Software on the laptop first', { placeholder: 'title' });
{
  const cw = (W - 0.4 * 2) / 3;
  s.addShape(pres.shapes.ROUNDED_RECTANGLE, { x: M, y: 1.5, w: cw, h: 3.9, rectRadius: 0.1,
    fill: { color: C.background2 }, line: { color: 'D9E0E8', width: 1 }, objectName: 'sw-1' });
  numberDot(s, { x: M + 0.28, y: 1.78, n: 1 });
  s.addText('Arduino IDE and cores', { x: M + 0.28, y: 2.48, w: cw - 0.56, h: 0.4,
    fontSize: 16, bold: true, color: C.text1, align: 'left', valign: 'top', isTextBox: true, margin: 0 });
  s.addText('Add the STM32 Board Manager URL under File > Preferences, then install STM32 MCU based boards and the ESP32 package.', {
    x: M + 0.28, y: 2.95, w: cw - 0.56, h: 1.3, fontSize: 13, color: '4A5568',
    align: 'left', valign: 'top', isTextBox: true, margin: 0, lineSpacingMultiple: 1.25 });
  s.addText('github.com/stm32duino/\nBoardManagerFiles', { x: M + 0.28, y: 4.4, w: cw - 0.56, h: 0.7,
    fontSize: 10, color: THEME.colors.accent2, fontFace: 'Courier New', align: 'left', valign: 'top', isTextBox: true, margin: 0 });

  const x2 = M + cw + 0.4;
  s.addShape(pres.shapes.ROUNDED_RECTANGLE, { x: x2, y: 1.5, w: cw, h: 3.9, rectRadius: 0.1,
    fill: { color: C.background2 }, line: { color: 'D9E0E8', width: 1 }, objectName: 'sw-2' });
  numberDot(s, { x: x2 + 0.28, y: 1.78, n: 2 });
  s.addText('Libraries for the ESP32', { x: x2 + 0.28, y: 2.48, w: cw - 0.56, h: 0.4,
    fontSize: 16, bold: true, color: C.text1, align: 'left', valign: 'top', isTextBox: true, margin: 0 });
  s.addText([
    { text: 'WebSockets by Markus Sattler', options: { bullet: true, breakLine: true } },
    { text: 'Adafruit GFX Library', options: { bullet: true, breakLine: true } },
    { text: 'Adafruit SH110X  ←  this robot’s screen', options: { bullet: true, breakLine: true } },
    { text: 'Adafruit SSD1306 only for a 0.96 inch panel', options: { bullet: true } },
  ], { x: x2 + 0.28, y: 2.95, w: cw - 0.56, h: 2.0, fontSize: 13, color: '4A5568',
    align: 'left', valign: 'top', isTextBox: true, margin: 0, paraSpaceAfter: 7 });

  const x3 = M + (cw + 0.4) * 2;
  s.addShape(pres.shapes.ROUNDED_RECTANGLE, { x: x3, y: 1.5, w: cw, h: 3.9, rectRadius: 0.1,
    fill: { color: C.text1 }, line: { color: C.text1, width: 1 }, objectName: 'sw-3' });
  numberDot(s, { x: x3 + 0.28, y: 1.78, n: 3, color: THEME.colors.accent1 });
  s.addText('On the PC side', { x: x3 + 0.28, y: 2.48, w: cw - 0.56, h: 0.4,
    fontSize: 16, bold: true, color: 'FFFFFF', align: 'left', valign: 'top', isTextBox: true, margin: 0 });
  s.addText('Install Python, then these four libraries from a command prompt.', {
    x: x3 + 0.28, y: 2.95, w: cw - 0.56, h: 0.9, fontSize: 13, color: 'B9C6D4',
    align: 'left', valign: 'top', isTextBox: true, margin: 0, lineSpacingMultiple: 1.25 });
  s.addText('py -m pip install\n  opencv-python numpy\n  pyzbar requests', {
    x: x3 + 0.28, y: 3.95, w: cw - 0.56, h: 1.1, fontSize: 11, color: THEME.colors.accent1,
    fontFace: 'Courier New', align: 'left', valign: 'top', isTextBox: true, margin: 0, lineSpacingMultiple: 1.2 });
}
callout(s, { x: M, y: 5.6, w: W, h: 0.9, tone: 'warn',
  title: 'Two different controllers hide behind identical-looking OLED modules',
  body: 'This robot has the SH1106. Drive it with the SSD1306 library and the whole frame lands in the top strip of the panel, with power-on noise below it.' });
s.addNotes('The full Board Manager URL is in README.md under Getting started. Copy it from there rather than typing it.\n\nIf the python command is not recognised on Windows, use py instead. That is normal, not a broken install.');

// --- 7. STM32 wiring ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'Install' });
s.addText('Wiring the STM32', { placeholder: 'title' });
table(s, {
  x: M, y: 1.5, w: 7.2, colW: [4.3, 2.9],
  head: ['Function', 'Pin'],
  rows: [
    ['Left motor  AIN1 / AIN2', 'PA0 / PA1'],
    ['Right motor  BIN1 / BIN2', 'PB0 / PB1'],
    ['Line sensors L2 L1 C R1 R2', 'PB9 PB8 PB7 PB6 PB5'],
    ['Serial2  from the ESP32 remote', 'PA2 TX / PA3 RX'],
    ['Serial3  from the ESP32-CAM', 'PB10 TX / PB11 RX'],
  ],
});
s.addText('The physical pins behind Serial2 and Serial3 depend on the board and the core, so check them before cutting wire.', {
  x: M, y: 4.15, w: 7.2, h: 0.6, fontSize: 12, color: THEME.colors.accent6,
  align: 'left', valign: 'top', isTextBox: true, margin: 0, lineSpacingMultiple: 1.2,
});
card(s, { x: 8.1, y: 1.5, w: W - 7.5, h: 1.7,
  head: 'Cross the data lines', accent: THEME.colors.accent1,
  body: 'One side’s TX always goes to the other side’s RX, and the grounds must be joined or the signal has no reference.' });
card(s, { x: 8.1, y: 3.45, w: W - 7.5, h: 1.55,
  head: 'Sensors run left to right', accent: THEME.colors.accent2,
  body: 'L2 is leftmost, R2 rightmost. Reverse the order and the robot steers away from the line instead of onto it.' });
callout(s, { x: M, y: 5.6, w: W, h: 0.9, tone: 'warn',
  title: 'Do not swap the motor pin pairs in setSpeed()',
  body: 'It reverses both motors: forward becomes backward, left becomes right, and line following drives the robot off the course.' });
s.addNotes('The pin names for Serial2 and Serial3 come from the variant, not from the chip, so a different core or board part number can move them. Verify before wiring, every time.');

// --- 8. ESP32 wiring ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'Install' });
s.addText('Wiring the ESP32 and the camera', { placeholder: 'title' });
table(s, {
  x: M, y: 1.5, w: 7.2, colW: [4.6, 2.6],
  head: ['Function', 'Pin'],
  rows: [
    ['OLED  SDA', 'GPIO21'],
    ['OLED  SCL', 'GPIO22'],
    ['To STM32 Serial2 RX (PA3)', 'GPIO17 TX'],
    ['From STM32 Serial2 TX (PA2)', 'GPIO16 RX'],
    ['CAM to STM32 Serial3 RX (PB11)', 'GPIO14 TX'],
  ],
});
s.addText('The camera reaches the STM32 on Serial3, a different port from the remote, so both can command the robot at once.', {
  x: M, y: 4.15, w: 7.2, h: 0.6, fontSize: 12, color: THEME.colors.accent6,
  align: 'left', valign: 'top', isTextBox: true, margin: 0, lineSpacingMultiple: 1.2,
});
card(s, { x: 8.1, y: 1.5, w: W - 7.5, h: 1.7,
  head: 'The OLED sits at 0x3C', accent: THEME.colors.accent1,
  body: 'A 128x64 I2C module. Change OLED_ADDR in the sketch if yours answers at 0x3D instead.' });
card(s, { x: 8.1, y: 3.45, w: W - 7.5, h: 1.55,
  head: 'A missing screen is survivable', accent: THEME.colors.accent2,
  body: 'If the display is not found the sketch skips every draw and everything else keeps running.' });
callout(s, { x: M, y: 5.6, w: W, h: 0.9, tone: 'warn',
  title: 'Solder the four screen wires rather than pushing them on',
  body: 'A loose contact cuts a frame mid-transfer and the panel stays dark while the code runs perfectly. It is the hardest fault here to track down.' });
s.addNotes('The loose-wire version of this cost us a full day of looking in the wrong place. Tell whoever takes over to solder those four from the start.');

// --- 9. ST-Link setup ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'Install' });
s.addText('Setting up ST-Link for the STM32', { placeholder: 'title' });
step(s, { x: M, y: 1.6, w: 5.7, n: 1, head: 'Install STM32CubeProgrammer',
  body: 'A free download from ST, account required. Keep the default path, and it brings the ST-Link driver with it.' });
step(s, { x: M, y: 2.95, w: 5.7, n: 2, head: 'Connect four wires',
  body: 'As in the table. Match them by the names printed on the dongle, never by pin number: the clones do not agree on a layout.' });
step(s, { x: M, y: 4.3, w: 5.7, n: 3, head: 'Set it in the IDE',
  body: 'Upload method becomes STM32CubeProgrammer (SWD), and Port stays empty. Every other setting is on the next slide.' });
table(s, {
  x: 6.9, y: 1.55, w: W - 6.3, colW: [2.1, 3.7],
  head: ['ST-Link', 'Blue Pill'],
  rows: [
    ['SWDIO', 'DIO   (PA13)'],
    ['SWCLK', 'DCLK  (PA14)'],
    ['GND', 'GND'],
    ['3.3V', '3V3  only if unpowered'],
  ],
  fontSize: 12.5,
});
callout(s, { x: 6.9, y: 3.7, w: W - 6.3, h: 1.4, tone: 'warn',
  title: 'PA13 and PA14 are not on the side headers',
  body: 'Follow the numbering and it jumps from A12 to A15. Both are on the 4-pin header at the far end from the USB socket, labelled DIO and DCLK.' });
callout(s, { x: M, y: 5.8, w: W, h: 0.85, tone: 'info',
  title: 'What switching to ST-Link buys you',
  body: 'BOOT0 never moves again, and no COM port is tied up, because the upload does not travel over one.' });
s.addNotes('Those two are exactly what made the old route fail: a BOOT0 left in the wrong position, and a Serial Monitor holding the port.\n\nThe dongle can stay plugged in permanently, since nothing in this firmware uses PA13 or PA14.');

// --- 10. IDE settings ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'Install' });
s.addText('Every Tools setting for the STM32', { placeholder: 'title' });
table(s, {
  x: M, y: 1.5, w: W, colW: [3.2, 4.1, 4.8],
  head: ['Tools menu', 'Set to', 'What a wrong value looks like'],
  rows: [
    ['Board', 'Generic STM32F1 series', 'No Upload method entry in the menu at all'],
    ['Board part number', 'BluePill F103C8', ''],
    ['Upload method', 'STM32CubeProgrammer (SWD)', 'Upload fails, it is calling another route'],
    ['U(S)ART support', "Enabled (generic 'Serial')", 'No Serial at all, and the log goes with it'],
    ['USB support', 'None', 'Serial moves to the USB socket, so the PA9 log falls silent with no error'],
    ['Optimize, Debug, C Runtime', 'Leave every default alone', ''],
    ['Port', 'Leave it empty', ''],
  ],
  fontSize: 12.5,
});
callout(s, { x: M, y: 5.45, w: W, h: 1.05, tone: 'info',
  title: 'The three rows with a note in the last column are the three worth checking twice',
  body: 'The rest still compile and run if you get them wrong.\nAn empty Port is correct, not a fault: an ST-Link never appears as a COM port in the first place.' });
s.addNotes('Board is the one people miss. Open Tools and if there is no Upload method entry anywhere, a board from a different core is selected.\n\nUSB support is the cruellest. Set to CDC it moves Serial onto the board USB socket, and the PA9 log goes quiet with nothing printed to explain it, while the code runs perfectly.\n\nBluePill F103C8 is the 64 KB entry and the firmware uses about 25 KB. If it ever outgrows that, BluePill F103CB unlocks the 128 KB the chip really holds.');

// --- 11. Upload each board ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'Install' });
s.addText('Flashing each board', { placeholder: 'title' });
step(s, { x: M, y: 1.6, w: 5.7, n: 1, head: 'STM32  over ST-Link',
  body: 'Open stm32_robot_controller, pick Generic STM32F1 series and press Upload. No BOOT0, no port to choose.' });
step(s, { x: M, y: 3.1, w: 5.7, n: 2, head: 'ESP32 remote  over its own USB',
  body: 'Open esp32_remote_controller and pick ESP32 Dev Module. Adafruit GFX and Adafruit SH110X must be installed or it will not compile.' });
step(s, { x: M, y: 4.6, w: 5.7, n: 3, head: 'ESP32-CAM  over USB-TTL',
  body: 'This board has no USB socket. Tie IO0 to GND before uploading and release it afterwards. Pick AI Thinker ESP32-CAM.' });
callout(s, { x: 6.9, y: 1.6, w: W - 6.3, h: 1.5, tone: 'warn',
  title: "undefined reference to 'Serial2'",
  body: 'Not a mistake in the sketch. The official STM32 core does not create Serial2 and Serial3, so the sketch creates them itself in two lines at the top. Do not delete them.' });
callout(s, { x: 6.9, y: 3.3, w: W - 6.3, h: 2.0, tone: 'info',
  title: 'Knowing the upload took',
  body: 'ST-Link prints Download verified successfully, which it only does after reading the flash back and comparing it to the file.\n\nOtherwise open the Serial Monitor at 115200, reset, and check the compile timestamp against the upload you just made.' });
s.addNotes('The order does not matter. Changing Tools > Board to match the board in front of you does, every single time.\n\nForgetting that is the most common mistake here: the sketch gets compiled for the wrong board, and sometimes it even uploads and only goes wrong at run time.\n\nA change to the camera code needs only esp32_camera_server reflashed, not all three.');

// --- 12. Flashing without the IDE ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'Install' });
s.addText('Flashing without the IDE', { placeholder: 'title' });
step(s, { x: M, y: 1.55, w: 5.7, n: 1, head: 'Export the binary first',
  body: 'Sketch > Export Compiled Binary. STM32CubeProgrammer cannot compile; it only writes a finished file to the chip.' });
s.addText('stm32_robot_controller\\build\\\nSTMicroelectronics.stm32.GenF1\\', {
  x: M + 0.72, y: 2.85, w: 5.0, h: 0.6, fontSize: 10.5, color: THEME.colors.accent2,
  fontFace: 'Courier New', align: 'left', valign: 'top', isTextBox: true, margin: 0, lineSpacingMultiple: 1.2,
});
step(s, { x: M, y: 3.6, w: 5.7, n: 2, head: 'Then write it to the chip',
  body: 'Leave the ST-Link connected as usual and run the command below.' });
codeBox(s, { x: M, y: 4.45, w: 5.7, h: 1.15,
  text: 'STM32_Programmer_CLI.exe -c port=SWD\n   mode=UR -w firmware.bin 0x08000000\n   -v -rst' });
table(s, {
  x: 6.9, y: 1.5, w: W - 6.3, colW: [2.6, 3.2],
  head: ['Part of the command', 'What it does'],
  rows: [
    ['-c port=SWD mode=UR', 'Connect through ST-Link'],
    ['-w file 0x08000000', 'Write to the start of flash'],
    ['-v', 'Read it back and compare'],
    ['-rst', 'Reset so it runs straight away'],
  ],
  fontSize: 12,
});
callout(s, { x: 6.9, y: 3.6, w: W - 6.3, h: 1.45, tone: 'warn',
  title: 'The address belongs to .bin only',
  body: 'A .hex carries its own address, so pass it as -w file.hex with no number after it, or the file lands in the wrong place.' });
callout(s, { x: M, y: 5.8, w: W, h: 0.85, tone: 'info',
  title: 'When this is worth it',
  body: 'Putting one build on several boards, or handing a build to someone who should not need the whole toolchain.  For one edit and one upload, the IDE button is quicker.' });
s.addNotes('This is what the IDE Upload button does behind the scenes; here we just call the steps ourselves.\n\nFor buttons instead of typing, open the STM32CubeProgrammer window, choose ST-LINK top right, press Connect, then Erasing and Programming with Verify and Run after programming ticked.');

// --- 13. Divider: Running ---
pres.addSection({ title: 'Running it' });
s = pres.addSlide({ masterName: 'SECTION', sectionTitle: 'Running it' });
s.addText('02', { placeholder: 'eyebrow' });
s.addText('Running it', { placeholder: 'title' });
s.addText('The order to switch things on, what the screen says, and what gets logged', { placeholder: 'body' });

// --- 14. Six steps ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'Running it' });
s.addText('Six steps to a first run', { placeholder: 'title' });
{
  const colW = 5.7, gap = W - colW * 2;
  const items = [
    ['Power the robot', 'The welcome screen holds for 2.5 seconds, then the status screen takes over'],
    ['Join the robot’s Wi-Fi', 'My_Robot2, password1234. Both the phone and the PC need to be on it'],
    ['Open the web page', 'Browse to 192.168.4.1. The camera image appears once the ESP32-CAM has joined'],
    ['Raise the speed slider', 'It starts at 0 every time. Leave it there and the robot will not move, auto or not'],
    ['Start the PC script', 'py pc_qr_scanner/qr_scanner.py. It finds the camera on the network by itself'],
    ['Place it on the line', 'Centre sensor over the black, then switch to Auto'],
  ];
  items.forEach((it, i) => {
    const col = i % 2, row = Math.floor(i / 2);
    step(s, {
      x: M + col * (colW + gap), y: 1.6 + row * 1.42, w: colW,
      n: i + 1, head: it[0], body: it[1],
    });
  });
}
callout(s, { x: M, y: 5.75, w: W, h: 0.9, tone: 'info',
  title: 'The script runs fine without a camera',
  body: 'If it cannot find one it carries on without QR reading, and still logs the whole run to the CSV exactly as before.' });
s.addNotes('Step 4 is where newcomers get stuck. The robot sits there and they assume it is broken, when the slider simply has not been raised from zero.\n\nIt starts at zero on purpose, so nothing lurches the moment the page loads.');

// --- 15. OLED screens ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'Running it' });
s.addText('What the screen is telling you', { placeholder: 'title' });
table(s, {
  x: M, y: 1.5, w: W, colW: [2.6, 4.5, 5.0],
  head: ['Screen', 'When', 'What it means'],
  rows: [
    ['Welcome', 'First 2.5 seconds after power', 'The board booted, and the screen and its wires are fine'],
    ['Status', 'Whenever the robot is stopped', 'Shows the mode, the speed and the camera IP'],
    ['Eyes glancing', 'The whole time it is moving', 'Hunting for a QR code: 600 ms ahead, then 450 ms each side'],
    ['Found', 'First second after a code is read', 'A code got through and the stop is coming'],
    ['Animal', 'The rest of the five-second stop', 'Which animal the code turned out to be'],
  ],
  fontSize: 12.5,
});
callout(s, { x: M, y: 5.5, w: W, h: 1.0, tone: 'info',
  title: 'The screen is the best diagnostic tool on the robot',
  body: 'It says how far the board got without a laptop attached.  A welcome screen that never changes means the program hung during boot, which is not a Wi-Fi problem.' });
s.addNotes('Every screen except Status is a full 128x64 bitmap with its lettering drawn into the artwork, so the firmware only blits an image.\n\nTo change one, edit the PNG in assets/oled and run py tools/png_to_bitmaps.py.');

// --- 16. The web page ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'Running it' });
s.addText('The web remote', { placeholder: 'title' });
{
  const cw = (W - 0.4 * 2) / 3, ch = 1.72;
  const items = [
    ['Direction buttons', 'Forward, back, left, right and stop. Manual mode only', THEME.colors.accent1],
    ['Speed slider', 'Starts at 0. What you set is what the motors get, with nothing rounded up', THEME.colors.accent1],
    ['Trim slider', 'Corrects a robot that pulls to one side, live, while it drives. No reflash', THEME.colors.accent1],
    ['Auto / Manual switch', 'Auto follows the line, Manual takes the buttons. Switch any time', THEME.colors.accent2],
    ['Camera image', 'Appears once the camera joins, and disappears if it goes quiet', THEME.colors.accent2],
    ['No numbers here', 'Deliberate. This page is for driving; every figure goes to the spreadsheet', THEME.colors.accent1],
  ];
  items.forEach((it, i) => {
    const col = i % 3, row = Math.floor(i / 3);
    card(s, {
      x: M + col * (cw + 0.4), y: 1.55 + row * (ch + 0.32), w: cw, h: ch,
      head: it[0], body: it[1], accent: it[2], dark: i === 5,
    });
  });
}
callout(s, { x: M, y: 5.75, w: W, h: 0.9, tone: 'warn',
  title: 'The page hangs even though the Wi-Fi is connected',
  body: 'Some phones notice the network has no internet and quietly switch back to mobile data. Turn that automatic switching off first.' });
s.addNotes('If the page will not open at all, check the phone is really on My_Robot2 rather than assuming it.');

// --- 17. What gets logged ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'Running it' });
s.addText('What lands in the spreadsheet', { placeholder: 'title' });
table(s, {
  x: M, y: 1.5, w: 7.5, colW: [2.6, 4.9],
  head: ['Column', 'Holds'],
  rows: [
    ['Timestamp  Event', 'The time, and the kind of row: START, RUN each second, or QR'],
    ['Mode  Action', 'The mode, and what it was doing: STRAIGHT, BEND, SEARCH, FINISH'],
    ['Base_Speed', 'What the slider is asking for'],
    ['Left_PWM  Right_PWM', 'What the motors were actually handed, after trim'],
    ['Turn  Error', 'The steering output, and how far off the line it was'],
    ['Kp  Kd  Trim', 'The gains in force, so an old file explains its own tuning'],
    ['S_L2 to S_R2', 'The five sensors one per column; 1 means that one is over black'],
  ],
  fontSize: 12,
});
s.addText('One row a second, plus one per QR hit. Plot Turn against time in Excel and you can see exactly where on the course the robot fights hardest.', {
  x: M, y: 5.1, w: 7.5, h: 0.8, fontSize: 12, color: THEME.colors.accent6,
  align: 'left', valign: 'top', isTextBox: true, margin: 0, lineSpacingMultiple: 1.25,
});
card(s, { x: 8.4, y: 1.5, w: W - 7.8, h: 1.9,
  head: 'Where the file goes', accent: THEME.colors.accent1,
  body: 'robot_mission_log.csv on the desktop. The script asks Windows where that really is, so OneDrive does not hide it.' });
card(s, { x: 8.4, y: 3.6, w: W - 7.8, h: 1.9, dark: true,
  head: 'Close Excel before a run', accent: THEME.colors.accent1,
  body: 'Windows locks an open file outright. If it cannot write, the script falls back to a new name with the time appended.' });
s.addNotes('The sensors get a column each because a single 00100 field is read as a number by Excel, which drops the leading zeros and leaves 100.\n\nKp and Kd are constants and logged anyway, so last week file says what tuning produced it instead of relying on anyone memory.');

// --- 18. Divider: Tuning ---
pres.addSection({ title: 'Tuning and faults' });
s = pres.addSlide({ masterName: 'SECTION', sectionTitle: 'Tuning and faults' });
s.addText('03', { placeholder: 'eyebrow' });
s.addText('Tuning and faults', { placeholder: 'title' });
s.addText('The constants worth touching, how it hunts for a lost line, and what keeps going wrong', { placeholder: 'body' });

// --- 19. Constants ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'Tuning and faults' });
s.addText('The constants worth touching', { placeholder: 'title' });
table(s, {
  x: M, y: 1.5, w: W, colW: [3.1, 1.3, 7.7],
  head: ['Name', 'Now', 'When to change it'],
  rows: [
    ['Kp', '45', 'Raise it if the robot cuts corners wide, lower it if it weaves on the straights'],
    ['Kd', '35', 'Damping. Raise it when the robot oscillates coming out of a bend'],
    ['leftMotorOffset', '25', 'Corrects a pull to one side. The Trim slider sets this live from the web page'],
    ['MIN_PWM', '0', 'What you ask for is what the motors get. Set 135 to have low values pushed up instead'],
    ['LOST_CONFIRM_MS', '150', 'How long the line may be missing before it reacts. Raise it if it lurches on tight bends'],
    ['FINISH_CONFIRM_MS', '40', 'How long all five black must hold to count as the finish, and not a crossing'],
  ],
  fontSize: 12.5,
});
callout(s, { x: M, y: 4.75, w: W, h: 1.05, tone: 'warn',
  title: 'Too much Kp makes the robot spin instead of turn',
  body: 'Error peaks at 4, so Kp of 80 asks for 320 against the 255 the motors have. The outer wheel saturates, the inner one reverses, and every bend becomes a pivot.' });
s.addNotes('Change one at a time and drive it after each. Change two and you cannot tell which helped.\n\nKp and Kd are written into every CSV row, so an old file tells you the tuning it was recorded with.');

// --- 20. Finding the line ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'Tuning and faults' });
s.addText('How it hunts for a lost line', { placeholder: 'title' });
s.addText('No sensor on the line does not mean lost. A tight bend swings the line past the end of the sensor row for a moment, and that is normal.', {
  x: M, y: 1.35, w: W, h: 0.45, fontSize: 14, color: '4A5568', align: 'left', isTextBox: true, margin: 0,
});
{
  const cw = (W - 0.4 * 2) / 3;
  const items = [
    ['Wait  150 ms', 'Decide nothing. Keep turning the way it already was, which is the direction that chases the line. Most bends are over before this timer is'],
    ['Reverse  400 ms', 'Back up, angled toward the side the line was last seen. Covers taking a bend too fast'],
    ['Sweep  up to 6 s', 'Pivot on the spot at 55% speed, alternating sides, each leg wider than the last. Only then does it give up'],
  ];
  items.forEach((it, i) => {
    const x = M + i * (cw + 0.4);
    s.addShape(pres.shapes.ROUNDED_RECTANGLE, {
      x, y: 2.0, w: cw, h: 2.5, rectRadius: 0.1,
      fill: { color: C.background2 }, line: { color: 'D9E0E8', width: 1 }, objectName: 'stage-' + i,
    });
    numberDot(s, { x: x + 0.28, y: 2.25, n: i + 1 });
    s.addText(it[0], { x: x + 0.28, y: 2.92, w: cw - 0.56, h: 0.4,
      fontSize: 16, bold: true, color: C.text1, align: 'left', valign: 'top', isTextBox: true, margin: 0 });
    s.addText(it[1], { x: x + 0.28, y: 3.38, w: cw - 0.56, h: 1.0,
      fontSize: 12.5, color: '4A5568', align: 'left', valign: 'top', isTextBox: true, margin: 0, lineSpacingMultiple: 1.25 });
  });
}
callout(s, { x: M, y: 4.75, w: W, h: 1.35, tone: 'info',
  title: 'All white and all black are opposite answers, not similar ones',
  body: 'Nothing on the line means the line is off the row and the robot must move itself until a sensor finds it again.  The reading that stops it is the exact opposite: all five over black, which is the finish bar.' });
s.addNotes('The sweep is deliberately slower than ordinary steering. Spin fast and the line crosses under the sensor row quicker than the row can register it, so the robot overshoots a line it has already passed.\n\nThe direction of the first leg is chosen once when the search begins and never recomputed, or it would keep sweeping the same way and walk further from the line each time.');

// --- 21. Faults ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'Tuning and faults' });
s.addText('Faults we actually hit', { placeholder: 'title' });
table(s, {
  x: M, y: 1.4, w: W, colW: [3.4, 3.9, 4.8],
  head: ['Symptom', 'What it turned out to be', 'Fix'],
  rows: [
    ['Screen dark on a restart', 'The panel still holding the I2C line', 'Firmware frees it at boot. If not, unplug power for 10 s'],
    ['STM32 will not take an upload', 'Wrong core selected, or no CubeProgrammer', 'Generic STM32F1 series, Upload method SWD'],
    ["undefined reference to 'Serial2'", 'The core does not create Serial2 or Serial3', 'The sketch creates them at the top. Keep those lines'],
    ['Web page will not open', 'Not actually joined to My_Robot2', 'Join it, then stop the phone switching to mobile data'],
    ['Robot always pulls one way', 'One motor stronger than the other', 'Move the Trim slider while it drives. No reflash'],
    ['Robot stops mid-course', 'Sensor polarity inverted: read as a finish bar', 'The black row in Serial must read 0 0 1 0 0 on the line'],
    ['CSV never updates', 'The file is open in Excel', 'Close it and run the script again'],
    ['Stripes across the camera image', 'Pixel clock too fast for the board', 'Already halved in the firmware. Reflash the camera'],
    ['Slider raised but nothing moves', 'Still at 0, or below what the wheels need', 'Go above 135. Below that the motors buzz but do not turn'],
    ['QR never decodes', 'Blurred, or the code is too far away', 'Move closer, and light the code instead of shading it'],
  ],
  fontSize: 11.5,
  headSize: 12.5,
});
s.addNotes('This is the page people will come back to. Tell them to photograph it.\n\nEvery row happened to us. None of them are hypothetical, and most were not what anyone suspected first.');

// --- 22. Checklist ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'Tuning and faults' });
s.addText('Before a real run', { placeholder: 'title' });
{
  const colW = 5.7, gap = W - colW * 2;
  const items = [
    'Battery charged, and motor power separate from the boards',
    'All four screen wires soldered, not just pushed on',
    'Data lines crossed correctly, grounds joined',
    'Compile timestamp in the Serial Monitor matches the upload',
    'On the line, the sensors read 0 0 1 0 0',
    'Phone and PC both joined to My_Robot2',
    'The CSV closed in Excel before starting the script',
    'Manual mode drives forward and back before switching to Auto',
  ];
  items.forEach((t, i) => {
    const col = i % 2, row = Math.floor(i / 2);
    const x = M + col * (colW + gap), y = 1.75 + row * 1.05;
    s.addShape(pres.shapes.ROUNDED_RECTANGLE, {
      x, y, w: 0.34, h: 0.34, rectRadius: 0.06,
      fill: { color: 'FFFFFF' }, line: { color: THEME.colors.accent4, width: 1.5 },
      objectName: 'check-' + i,
    });
    s.addText(t, {
      x: x + 0.55, y: y - 0.04, w: colW - 0.55, h: 0.75,
      fontSize: 13.5, color: C.text1, align: 'left', valign: 'top', isTextBox: true, margin: 0,
      lineSpacingMultiple: 1.2,
    });
  });
}
callout(s, { x: M, y: 5.85, w: W, h: 0.85, tone: 'info',
  title: 'Top to bottom, every time',
  body: 'Eight checks take under two minutes and rule out almost everything that otherwise costs a whole day.' });
s.addNotes('Worth printing and taping up beside the test course.');

// --- 23. Closing ---
s = pres.addSlide({ masterName: 'TITLE_DARK', sectionTitle: 'Tuning and faults' });
s.addText('Where to read further', { placeholder: 'eyebrow' });
s.addText('Go and build it', { placeholder: 'title' });
s.addText('Everything in this deck is written out in full in the project README, together with the reasoning behind every constant.', { placeholder: 'body' });
{
  const cw = (W - 0.4 * 2) / 3;
  const items = [
    ['README.md', 'The full guide: every constant and why it is what it is'],
    ['tests/', 'Sketches that isolate the screen and the I2C bus from the wiring'],
    ['assets/oled/', 'Source artwork for every screen; edit and regenerate'],
  ];
  items.forEach((it, i) => {
    const x = M + i * (cw + 0.4);
    s.addText(it[0], {
      x, y: 5.55, w: cw, h: 0.35, fontSize: 15, bold: true, color: THEME.colors.accent1,
      fontFace: 'Courier New', align: 'left', valign: 'top', isTextBox: true, margin: 0,
    });
    s.addText(it[1], {
      x, y: 5.95, w: cw, h: 0.8, fontSize: 12, color: 'B9C6D4',
      align: 'left', valign: 'top', isTextBox: true, margin: 0, lineSpacingMultiple: 1.2,
    });
  });
}
s.addNotes('Finish by opening README.md on screen once, so they know where to go when something breaks.');

deck.finish(process.argv[2] || 'Tiny-Robot-Setup-Guide-EN.pptx');
