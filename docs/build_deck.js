// สร้างไฟล์ PowerPoint คู่มือติดตั้ง Tiny Robot
// ข้อมูลทั้งหมดดึงจากโค้ดจริงใน repo ไม่ใช่ค่าสมมติ

const pptxgen = require('pptxgenjs');

const THEME = {
  name: 'Tiny Robot',
  headFontFace: 'Tahoma',
  bodyFontFace: 'Tahoma',
  colors: {
    dk1: '16202E',   // กรมท่าเข้ม ใช้เป็นสีตัวอักษรหลักและพื้นสไลด์เข้ม
    lt1: 'FFFFFF',
    dk2: '27405C',
    lt2: 'EEF2F6',   // พื้นการ์ดอ่อน
    accent1: 'E8803A',  // ส้ม สีเน้นหลัก
    accent2: '1C7293',  // ฟ้าอมเขียว
    accent3: '065A82',
    accent4: '2F7A4F',  // เขียว = ผ่าน
    accent5: 'B3261E',  // แดง = ปัญหา
    accent6: '6B7E93',
    hlink: '1C7293',
    folFlink: '6B7E93',
    folHlink: '6B7E93',
  },
};

const pres = new pptxgen();
pres.layout = 'LAYOUT_WIDE';   // 13.3 x 7.5 นิ้ว -- ต้องตั้งก่อนเพิ่มสไลด์
pres.theme = { headFontFace: THEME.headFontFace, bodyFontFace: THEME.bodyFontFace };
pres.title = 'Tiny Robot - คู่มือติดตั้งและเริ่มใช้งาน';
pres.author = 'Tiny Robot';
pres.subject = 'คู่มือติดตั้งและใช้งานหุ่นเดินตามเส้น';

const C = pres.SchemeColor;
const M = 0.6;            // ระยะขอบสไลด์
const W = 13.3 - M * 2;   // ความกว้างที่ใช้ได้ = 12.1

// ---------- เลย์เอาต์ ----------

pres.defineSlideMaster({
  title: 'TITLE_DARK',
  background: { color: C.text1 },
  objects: [
    // ไม่ใส่ charSpacing เพราะภาษาไทยมีสระกับวรรณยุกต์เกาะอยู่บนตัวอักษร
    // พอถ่างระยะห่าง ตัวที่เกาะจะหลุดออกจากตัวที่มันเกาะอยู่ อ่านแล้วสะดุด
    { placeholder: { options: { name: 'eyebrow', type: 'body', x: M, y: 2.0, w: W, h: 0.4,
        fontSize: 15, bold: true, color: THEME.colors.accent1, align: 'left' }, text: '' } },
    { placeholder: { options: { name: 'title', type: 'title', x: M, y: 2.5, w: W, h: 1.5,
        fontSize: 54, bold: true, color: C.background1, align: 'left' }, text: '' } },
    { placeholder: { options: { name: 'body', type: 'body', x: M, y: 4.1, w: 10.4, h: 1.5,
        fontSize: 18, color: 'B9C6D4', align: 'left' }, text: '' } },
  ],
});

pres.defineSlideMaster({
  title: 'SECTION',
  background: { color: C.text1 },
  objects: [
    { placeholder: { options: { name: 'eyebrow', type: 'body', x: M, y: 2.6, w: W, h: 0.5,
        fontSize: 60, bold: true, color: THEME.colors.accent1, align: 'left' }, text: '' } },
    { placeholder: { options: { name: 'title', type: 'title', x: M, y: 3.5, w: W, h: 1.0,
        fontSize: 40, bold: true, color: C.background1, align: 'left' }, text: '' } },
    { placeholder: { options: { name: 'body', type: 'body', x: M, y: 4.6, w: 9.5, h: 1.0,
        fontSize: 16, color: 'B9C6D4', align: 'left' }, text: '' } },
  ],
});

pres.defineSlideMaster({
  title: 'CONTENT',
  background: { color: C.background1 },
  objects: [
    { placeholder: { options: { name: 'title', type: 'title', x: M, y: 0.42, w: W, h: 0.75,
        fontSize: 32, bold: true, color: C.text1, align: 'left' }, text: '' } },
    { text: { text: 'Tiny Robot · คู่มือติดตั้งและเริ่มใช้งาน', options: { x: M, y: 6.85, w: 8, h: 0.3,
        fontSize: 10, color: THEME.colors.accent6, align: 'left', isTextBox: true, margin: 0 } } },
  ],
  slideNumber: { x: 12.4, y: 6.85, w: 0.5, h: 0.3, fontSize: 10, color: THEME.colors.accent6, align: 'right' },
});

// ---------- ตัวช่วยวาด ----------

// การ์ดพื้นอ่อนหนึ่งใบ พร้อมหัวข้อและคำอธิบาย
function card(slide, { x, y, w, h, head, body, accent, dark, headSize = 16, bodySize = 13 }) {
  slide.addShape(pres.shapes.ROUNDED_RECTANGLE, {
    x, y, w, h, rectRadius: 0.1,
    fill: { color: dark ? C.text1 : C.background2 },
    line: { color: dark ? C.text1 : 'D9E0E8', width: 1 },
    objectName: 'card-' + head.slice(0, 14),
  });
  slide.addText(head, {
    x: x + 0.28, y: y + 0.2, w: w - 0.56, h: 0.4,
    fontSize: headSize, bold: true, color: accent || (dark ? C.background1 : C.text1),
    align: 'left', valign: 'top', isTextBox: true, margin: 0,
  });
  slide.addText(body, {
    x: x + 0.28, y: y + 0.72, w: w - 0.56, h: h - 0.95,
    fontSize: bodySize, color: dark ? 'B9C6D4' : '4A5568',
    align: 'left', valign: 'top', isTextBox: true, margin: 0, lineSpacingMultiple: 1.25,
  });
}

// วงกลมใส่ตัวเลข -- ลวดลายประจำเล่มที่ใช้ซ้ำทุกหน้าที่เป็นลำดับขั้น
function numberDot(slide, { x, y, n, size = 0.52, color }) {
  slide.addShape(pres.shapes.OVAL, {
    x, y, w: size, h: size,
    fill: { color: color || THEME.colors.accent1 },
    line: { color: color || THEME.colors.accent1, width: 1 },
    objectName: 'dot-' + n,
  });
  slide.addText(String(n), {
    x, y, w: size, h: size,
    fontSize: 15, bold: true, color: 'FFFFFF',
    align: 'center', valign: 'middle', isTextBox: true, margin: 0,
  });
}

// ขั้นตอนหนึ่งข้อ = วงกลมเลข + หัวข้อ + คำอธิบาย
function step(slide, { x, y, w, n, head, body, color }) {
  numberDot(slide, { x, y, n, color });
  slide.addText(head, {
    x: x + 0.72, y: y - 0.02, w: w - 0.72, h: 0.33,
    fontSize: 16, bold: true, color: C.text1, align: 'left', valign: 'top', isTextBox: true, margin: 0,
  });
  slide.addText(body, {
    x: x + 0.72, y: y + 0.34, w: w - 0.72, h: 0.9,
    fontSize: 13, color: '4A5568', align: 'left', valign: 'top', isTextBox: true, margin: 0,
    lineSpacingMultiple: 1.25,
  });
}

// กล่องในผังการสื่อสาร
function node(slide, { x, y, w, h, title, sub, fill, fg }) {
  slide.addShape(pres.shapes.ROUNDED_RECTANGLE, {
    x, y, w, h, rectRadius: 0.1,
    fill: { color: fill }, line: { color: fill, width: 1 },
    objectName: 'node-' + title.slice(0, 12),
  });
  slide.addText(title, {
    x: x + 0.15, y: y + 0.18, w: w - 0.3, h: 0.35,
    fontSize: 16, bold: true, color: fg, align: 'center', valign: 'middle', isTextBox: true, margin: 0,
  });
  slide.addText(sub, {
    x: x + 0.15, y: y + 0.56, w: w - 0.3, h: 0.3,
    fontSize: 11, color: fg === 'FFFFFF' ? 'B9C6D4' : '4A5568',
    align: 'center', valign: 'middle', isTextBox: true, margin: 0,
  });
}

// ลูกศรพร้อมป้ายกำกับช่องทางสื่อสาร
function arrow(slide, { x, y, w, label }) {
  slide.addText(label, {
    x: x - 0.1, y: y - 0.42, w: w + 0.2, h: 0.32,
    fontSize: 10, color: THEME.colors.accent6, align: 'center', valign: 'bottom', isTextBox: true, margin: 0,
  });
  slide.addShape(pres.shapes.RIGHT_ARROW, {
    x, y, w, h: 0.22,
    fill: { color: THEME.colors.accent1 }, line: { color: THEME.colors.accent1, width: 0 },
    objectName: 'arrow-' + label.slice(0, 10),
  });
}

// ตารางมาตรฐานของเล่มนี้ หัวตารางพื้นเข้ม ตัวอักษรขาว
function table(slide, { x, y, w, colW, head, rows, fontSize = 13, headSize = 13 }) {
  const body = rows.map((r) =>
    r.map((cell, i) => ({
      text: cell,
      options: {
        fontSize,
        color: i === 0 ? THEME.colors.dk1 : '4A5568',
        bold: i === 0,
        valign: 'middle',
      },
    }))
  );
  slide.addTable(
    [head.map((h) => ({ text: h, options: { fontSize: headSize, bold: true, color: 'FFFFFF', fill: { color: THEME.colors.dk1 }, valign: 'middle' } })), ...body],
    {
      x, y, w, colW,
      border: { type: 'solid', color: 'D9E0E8', pt: 1 },
      fill: { color: 'FFFFFF' },
      margin: 0.09,
      autoPage: false,
    }
  );
}

// แถบหมายเหตุ ใช้เน้นข้อที่พลาดแล้วเสียเวลาทั้งวัน
function callout(slide, { x, y, w, h, title, body, tone = 'warn' }) {
  const accent = tone === 'warn' ? THEME.colors.accent5 : THEME.colors.accent2;
  slide.addShape(pres.shapes.ROUNDED_RECTANGLE, {
    x, y, w, h, rectRadius: 0.08,
    fill: { color: tone === 'warn' ? 'FBEEEC' : 'E8F1F5' },
    line: { color: tone === 'warn' ? 'F0D3CE' : 'CBE0E9', width: 1 },
    objectName: 'callout',
  });
  slide.addText(title, {
    x: x + 0.25, y: y + 0.14, w: w - 0.5, h: 0.3,
    fontSize: 14, bold: true, color: accent, align: 'left', valign: 'top', isTextBox: true, margin: 0,
  });
  slide.addText(body, {
    x: x + 0.25, y: y + 0.48, w: w - 0.5, h: h - 0.62,
    fontSize: 12.5, color: '4A5568', align: 'left', valign: 'top', isTextBox: true, margin: 0,
    lineSpacingMultiple: 1.2,
  });
}

// ================= สไลด์ =================

// --- 1. ปก ---
pres.addSection({ title: 'ภาพรวม' });
let s = pres.addSlide({ masterName: 'TITLE_DARK', sectionTitle: 'ภาพรวม' });
s.addText('คู่มือสำหรับผู้ใช้งานใหม่', { placeholder: 'eyebrow' });
s.addText('Tiny Robot', { placeholder: 'title' });
s.addText('หุ่นสองล้อเดินตามเส้นดำ อ่าน QR แล้วหยุด และสั่งงานผ่านมือถือได้   ติดตั้งตั้งแต่ศูนย์จนวิ่งได้จริง', { placeholder: 'body' });
s.addText('STM32 Blue Pill  ·  ESP32  ·  ESP32-CAM  ·  Python', {
  x: M, y: 6.3, w: W, h: 0.4, fontSize: 13, color: '6B7E93', align: 'left', isTextBox: true, margin: 0,
});
s.addNotes('สไลด์ชุดนี้ใช้สอนคนที่เพิ่งมารับช่วงต่อ ไล่ตั้งแต่ลงโปรแกรมในเครื่อง ต่อสาย อัปเฟิร์มแวร์ จนเปิดใช้งานได้จริง แล้วปิดท้ายด้วยอาการเสียที่เจอบ่อยพร้อมวิธีแก้\n\nเวลาสอนให้เปิดหุ่นจริงวางไว้ข้างๆ ด้วย จะได้ชี้ของจริงประกอบไปทีละหน้า');

// --- 2. หุ่นทำอะไรได้ ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'ภาพรวม' });
s.addText('หุ่นตัวนี้ทำอะไรได้', { placeholder: 'title' });
{
  const cw = (W - 0.4 * 2) / 3, ch = 1.62;
  const items = [
    ['เดินตามเส้นดำ', 'เซนเซอร์ 5 ดวง คุมด้วย PD เข้าโค้งได้ และเบรกเองเมื่อถึงเส้นชัย', THEME.colors.accent1, false],
    ['รับคำสั่งจากมือถือ', 'เดินหน้า ถอยหลัง เลี้ยว หยุด พร้อมแถบเลื่อนความเร็วและโหมด Auto', THEME.colors.accent1, false],
    ['มองเห็น', 'ESP32-CAM ส่งภาพสดเข้าหน้าเว็บ และส่งให้เครื่อง PC อ่าน QR', THEME.colors.accent1, false],
    ['อ่าน QR ได้ 5 คำ', 'DOG  CAT  BIRD  LION  TIGER ถอดรหัสที่เครื่อง PC แล้วส่งกลับมา', THEME.colors.accent2, false],
    ['ตอบสนอง', 'เจอรหัสแล้วหยุด 5 วินาที พร้อมวาดรูปสัตว์ตัวนั้นขึ้นจอ OLED', THEME.colors.accent2, false],
    ['เก็บข้อมูลให้ด้วย', 'บันทึกความเร็วและการเลี้ยวทุกวินาที ลงไฟล์ CSV เปิดใน Excel ได้ทันที', THEME.colors.accent1, true],
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
  title: 'ไม่ต้องพึ่งอินเทอร์เน็ตของที่ไหนเลย',
  body: 'ESP32 บนตัวหุ่นปล่อย Wi-Fi ของมันเองชื่อ My_Robot2  มือถือกับคอมต่อเข้าวงนี้ตรงๆ\nเอาไปสาธิตที่ไหนก็ได้ ไม่ต้องขอรหัส Wi-Fi ของสถานที่ และไม่มีปัญหาเน็ตช้า' });
s.addNotes('ย้ำว่าระบบทำงานเป็นชั้นๆ ไม่ต้องมีครบทุกอย่างถึงจะวิ่งได้\n\nSTM32 ตัวเดียวก็เดินตามเส้นได้แล้ว ESP32 เพิ่มรีโมทกับจอ ส่วนกล้องกับ PC เพิ่มเรื่องอ่าน QR เข้ามา ถ้าชิ้นไหนเสีย ที่เหลือยังทำงานต่อได้');

// --- 3. อุปกรณ์ 4 ชิ้น ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'ภาพรวม' });
s.addText('อุปกรณ์ 4 ชิ้นที่ต้องมี', { placeholder: 'title' });
{
  const cw = (W - 0.3 * 3) / 4;
  const items = [
    ['STM32 Blue Pill', 'สมองของหุ่น คุมมอเตอร์ อ่านเซนเซอร์ และเดินตามเส้น', 'stm32_robot_controller'],
    ['ESP32', 'ปล่อย Wi-Fi ชื่อ My_Robot2 เปิดหน้าเว็บรีโมท และคุมจอ OLED', 'esp32_remote_controller'],
    ['ESP32-CAM', 'กล้องของหุ่น ส่งภาพให้ PC และสั่งหยุดเมื่อเจอรหัสสัตว์', 'esp32_camera_server'],
    ['เครื่อง PC', 'อ่าน QR จากภาพ แล้วเก็บข้อมูลการวิ่งลง CSV บนเดสก์ท็อป', 'qr_scanner.py'],
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
  title: 'ไม่ต้องมีครบก็เริ่มได้',
  body: 'STM32 ตัวเดียวเดินตามเส้นได้แล้ว  เพิ่ม ESP32 เพื่อให้มีรีโมทกับจอ  แล้วค่อยเพิ่มกล้องกับ PC ตอนจะทำเรื่องอ่าน QR' });
s.addNotes('ชื่อโฟลเดอร์ใต้การ์ดแต่ละใบคือโฟลเดอร์ในโปรเจกต์ที่ต้องเปิดอัปลงบอร์ดตัวนั้น จำคู่นี้ให้แม่น เวลาอัปผิดบอร์ดจะงงกันทั้งวัน');

// --- 4. ใครคุยกับใคร ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'ภาพรวม' });
s.addText('ใครคุยกับใคร', { placeholder: 'title' });
{
  const nw = 3.2, nh = 1.25, gap = (W - nw * 3) / 2;
  const xs = [M, M + nw + gap, M + (nw + gap) * 2];

  s.addText('เส้นทางที่ 1  ·  สั่งงานด้วยมือ', {
    x: M, y: 1.4, w: W, h: 0.3, fontSize: 13, bold: true, color: THEME.colors.accent1,
    align: 'left', isTextBox: true, margin: 0,
  });
  node(s, { x: xs[0], y: 1.78, w: nw, h: nh, title: 'มือถือ', sub: 'เปิด 192.168.4.1', fill: C.background2, fg: THEME.colors.dk1 });
  node(s, { x: xs[1], y: 1.78, w: nw, h: nh, title: 'ESP32 รีโมท', sub: 'ปล่อย Wi-Fi + จอ OLED', fill: THEME.colors.dk1, fg: 'FFFFFF' });
  node(s, { x: xs[2], y: 1.78, w: nw, h: nh, title: 'STM32', sub: 'มอเตอร์ + เซนเซอร์', fill: C.background2, fg: THEME.colors.dk1 });
  arrow(s, { x: xs[0] + nw + 0.12, y: 2.3, w: gap - 0.24, label: 'WebSocket 81' });
  arrow(s, { x: xs[1] + nw + 0.12, y: 2.3, w: gap - 0.24, label: 'Serial2 115200' });

  s.addText('เส้นทางที่ 2  ·  อ่าน QR แล้วสั่งหยุด', {
    x: M, y: 3.5, w: W, h: 0.3, fontSize: 13, bold: true, color: THEME.colors.accent2,
    align: 'left', isTextBox: true, margin: 0,
  });
  node(s, { x: xs[0], y: 3.88, w: nw, h: nh, title: 'เครื่อง PC', sub: 'qr_scanner.py', fill: C.background2, fg: THEME.colors.dk1 });
  node(s, { x: xs[1], y: 3.88, w: nw, h: nh, title: 'ESP32-CAM', sub: 'กล้อง + สะพานสัญญาณ', fill: THEME.colors.dk1, fg: 'FFFFFF' });
  node(s, { x: xs[2], y: 3.88, w: nw, h: nh, title: 'STM32', sub: 'หยุด 5 วินาที', fill: C.background2, fg: THEME.colors.dk1 });
  arrow(s, { x: xs[0] + nw + 0.12, y: 4.4, w: gap - 0.24, label: 'UDP 1234' });
  arrow(s, { x: xs[1] + nw + 0.12, y: 4.4, w: gap - 0.24, label: "Serial 9600  'S'" });
}
callout(s, { x: M, y: 5.5, w: W, h: 1.0, tone: 'info',
  title: 'ยังมีอีกสองเส้นที่มองไม่เห็นจากหน้านี้',
  body: 'กล้องส่งสัญญาณบอกตัวตนให้รีโมททุก 2 วินาที รีโมทจะได้รู้ IP ไว้ทำลิงก์ภาพสด   และ STM32 ส่งค่าสถานะกลับทุก 200 มิลลิวินาที ให้ PC ดึงไปเก็บลง Excel' });
s.addNotes('ทุกอย่างอยู่บน Wi-Fi ชื่อ My_Robot2 ที่ ESP32 รีโมทเป็นคนปล่อย ไม่ใช่ Wi-Fi บ้าน\n\nรีโมทเป็นเกตเวย์ของวงเสมอ IP จึงเป็น 192.168.4.1 ตายตัว ส่วนกล้องได้ IP อะไรก็ได้ตั้งแต่ .2 ขึ้นไป สคริปต์ Python จึงไล่หากล้องเองทุกครั้งที่เปิด');

// --- 5. คั่น: ติดตั้ง ---
pres.addSection({ title: 'ติดตั้ง' });
s = pres.addSlide({ masterName: 'SECTION', sectionTitle: 'ติดตั้ง' });
s.addText('01', { placeholder: 'eyebrow' });
s.addText('ติดตั้งและต่อสาย', { placeholder: 'title' });
s.addText('ลงโปรแกรมในเครื่อง ต่อสายให้ครบ แล้วอัปเฟิร์มแวร์ลงทั้งสามบอร์ด', { placeholder: 'body' });

// --- 6. ลงโปรแกรม ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'ติดตั้ง' });
s.addText('ลงโปรแกรมในเครื่องก่อน', { placeholder: 'title' });
{
  const cw = (W - 0.4 * 2) / 3;
  s.addShape(pres.shapes.ROUNDED_RECTANGLE, { x: M, y: 1.5, w: cw, h: 3.9, rectRadius: 0.1,
    fill: { color: C.background2 }, line: { color: 'D9E0E8', width: 1 }, objectName: 'sw-1' });
  numberDot(s, { x: M + 0.28, y: 1.78, n: 1 });
  s.addText('Arduino IDE + บอร์ด', { x: M + 0.28, y: 2.48, w: cw - 0.56, h: 0.4,
    fontSize: 16, bold: true, color: C.text1, align: 'left', valign: 'top', isTextBox: true, margin: 0 });
  s.addText('เปิด File > Preferences แล้วใส่ลิงก์ Board Manager ของ STM32 ลงไป จากนั้นลง STM32 MCU based boards และชุดบอร์ด ESP32', {
    x: M + 0.28, y: 2.95, w: cw - 0.56, h: 1.3, fontSize: 13, color: '4A5568',
    align: 'left', valign: 'top', isTextBox: true, margin: 0, lineSpacingMultiple: 1.25 });
  s.addText('github.com/stm32duino/\nBoardManagerFiles', { x: M + 0.28, y: 4.4, w: cw - 0.56, h: 0.7,
    fontSize: 10, color: THEME.colors.accent2, fontFace: 'Courier New', align: 'left', valign: 'top', isTextBox: true, margin: 0 });

  const x2 = M + cw + 0.4;
  s.addShape(pres.shapes.ROUNDED_RECTANGLE, { x: x2, y: 1.5, w: cw, h: 3.9, rectRadius: 0.1,
    fill: { color: C.background2 }, line: { color: 'D9E0E8', width: 1 }, objectName: 'sw-2' });
  numberDot(s, { x: x2 + 0.28, y: 1.78, n: 2 });
  s.addText('ไลบรารีของ ESP32', { x: x2 + 0.28, y: 2.48, w: cw - 0.56, h: 0.4,
    fontSize: 16, bold: true, color: C.text1, align: 'left', valign: 'top', isTextBox: true, margin: 0 });
  s.addText([
    { text: 'WebSockets โดย Markus Sattler', options: { bullet: true, breakLine: true } },
    { text: 'Adafruit GFX Library', options: { bullet: true, breakLine: true } },
    { text: 'Adafruit SH110X  ←  จอของหุ่นตัวนี้', options: { bullet: true, breakLine: true } },
    { text: 'Adafruit SSD1306  ถ้าเปลี่ยนไปใช้จอ 0.96 นิ้ว', options: { bullet: true } },
  ], { x: x2 + 0.28, y: 2.95, w: cw - 0.56, h: 2.0, fontSize: 13, color: '4A5568',
    align: 'left', valign: 'top', isTextBox: true, margin: 0, paraSpaceAfter: 7 });

  const x3 = M + (cw + 0.4) * 2;
  s.addShape(pres.shapes.ROUNDED_RECTANGLE, { x: x3, y: 1.5, w: cw, h: 3.9, rectRadius: 0.1,
    fill: { color: C.text1 }, line: { color: C.text1, width: 1 }, objectName: 'sw-3' });
  numberDot(s, { x: x3 + 0.28, y: 1.78, n: 3, color: THEME.colors.accent1 });
  s.addText('ของฝั่งเครื่อง PC', { x: x3 + 0.28, y: 2.48, w: cw - 0.56, h: 0.4,
    fontSize: 16, bold: true, color: 'FFFFFF', align: 'left', valign: 'top', isTextBox: true, margin: 0 });
  s.addText('ลง Python ก่อน แล้วสั่งติดตั้งไลบรารีสี่ตัวนี้ในหน้าต่าง Command Prompt', {
    x: x3 + 0.28, y: 2.95, w: cw - 0.56, h: 0.9, fontSize: 13, color: 'B9C6D4',
    align: 'left', valign: 'top', isTextBox: true, margin: 0, lineSpacingMultiple: 1.25 });
  s.addText('py -m pip install\n  opencv-python numpy\n  pyzbar requests', {
    x: x3 + 0.28, y: 3.95, w: cw - 0.56, h: 1.1, fontSize: 11, color: THEME.colors.accent1,
    fontFace: 'Courier New', align: 'left', valign: 'top', isTextBox: true, margin: 0, lineSpacingMultiple: 1.2 });
}
callout(s, { x: M, y: 5.6, w: W, h: 0.9, tone: 'warn',
  title: 'จอ OLED มีสองชิปที่หน้าตาเหมือนกันเป๊ะ',
  body: 'จอของหุ่นตัวนี้เป็น SH1106 ถ้าลงไลบรารีผิดตัว ภาพจะอัดอยู่แถบบนสุดของจอ ส่วนที่เหลือเป็นจุดรบกวน' });
s.addNotes('ลิงก์ Board Manager เต็มๆ อยู่ใน README.md หัวข้อ Getting started ให้ก๊อปจากตรงนั้น อย่าพิมพ์เอง\n\nถ้าสั่ง python แล้วเครื่องไม่รู้จัก ให้ใช้ py แทน เป็นเรื่องปกติของ Python บน Windows');

// --- 7. ต่อสาย STM32 ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'ติดตั้ง' });
s.addText('ต่อสาย STM32', { placeholder: 'title' });
table(s, {
  x: M, y: 1.5, w: 7.2, colW: [4.3, 2.9],
  head: ['หน้าที่', 'ขา'],
  rows: [
    ['มอเตอร์ซ้าย  AIN1 / AIN2', 'PA0 / PA1'],
    ['มอเตอร์ขวา  BIN1 / BIN2', 'PB0 / PB1'],
    ['เซนเซอร์เส้น L2 L1 C R1 R2', 'PB9 PB8 PB7 PB6 PB5'],
    ['Serial2  จาก ESP32 รีโมท', 'PA2 ส่ง / PA3 รับ'],
    ['Serial3  จาก ESP32-CAM', 'PB10 ส่ง / PB11 รับ'],
  ],
});
s.addText('ขาจริงของ Serial2 และ Serial3 ขึ้นกับบอร์ดและการตั้งค่าคอร์ ให้เช็กก่อนลงมือต่อสายทุกครั้ง', {
  x: M, y: 4.15, w: 7.2, h: 0.6, fontSize: 12, color: THEME.colors.accent6,
  align: 'left', valign: 'top', isTextBox: true, margin: 0, lineSpacingMultiple: 1.2,
});
card(s, { x: 8.1, y: 1.5, w: W - 7.5, h: 1.7,
  head: 'สายสื่อสารต้องไขว้', accent: THEME.colors.accent1,
  body: 'ขาส่งของฝั่งหนึ่งไปเข้าขารับของอีกฝั่งเสมอ และต้องต่อสาย GND ร่วมกันด้วย ไม่งั้นสัญญาณไม่มีจุดอ้างอิง' });
card(s, { x: 8.1, y: 3.45, w: W - 7.5, h: 1.55,
  head: 'เซนเซอร์เรียงจากซ้ายไปขวา', accent: THEME.colors.accent2,
  body: 'L2 คือซ้ายสุด R2 คือขวาสุด สลับลำดับแล้วหุ่นจะเลี้ยวออกจากเส้นแทนที่จะเลี้ยวเข้า' });
callout(s, { x: M, y: 5.6, w: W, h: 0.9, tone: 'warn',
  title: 'ห้ามสลับคู่ขามอเตอร์ในฟังก์ชัน setSpeed',
  body: 'สลับแล้วมอเตอร์กลับทิศทั้งคู่ เดินหน้ากลายเป็นถอยหลัง ซ้ายกลายเป็นขวา และหุ่นจะวิ่งหนีออกจากเส้น' });
s.addNotes('ขาจริงของ Serial2 กับ Serial3 ขึ้นกับบอร์ดและการตั้งค่าคอร์ ให้เช็กก่อนลงมือต่อสายทุกครั้ง อย่าเชื่อตารางอย่างเดียว');

// --- 8. ต่อสาย ESP32 ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'ติดตั้ง' });
s.addText('ต่อสาย ESP32 และ ESP32-CAM', { placeholder: 'title' });
table(s, {
  x: M, y: 1.5, w: 7.2, colW: [4.6, 2.6],
  head: ['หน้าที่', 'ขา'],
  rows: [
    ['จอ OLED  SDA', 'GPIO21'],
    ['จอ OLED  SCL', 'GPIO22'],
    ['ไป STM32 Serial2 รับ (PA3)', 'GPIO17 ส่ง'],
    ['จาก STM32 Serial2 ส่ง (PA2)', 'GPIO16 รับ'],
    ['CAM ไป STM32 Serial3 รับ (PB11)', 'GPIO14 ส่ง'],
  ],
});
s.addText('ESP32-CAM ต่อเข้า Serial3 ของ STM32 คนละช่องกับรีโมท ทั้งสองบอร์ดจึงสั่งงานหุ่นได้พร้อมกัน', {
  x: M, y: 4.15, w: 7.2, h: 0.6, fontSize: 12, color: THEME.colors.accent6,
  align: 'left', valign: 'top', isTextBox: true, margin: 0, lineSpacingMultiple: 1.2,
});
card(s, { x: 8.1, y: 1.5, w: W - 7.5, h: 1.7,
  head: 'จอ OLED ใช้แอดเดรส 0x3C', accent: THEME.colors.accent1,
  body: 'เป็นจอ I2C ขนาด 128x64 ถ้าโมดูลของคุณใช้ 0x3D ให้แก้ค่า OLED_ADDR ในโค้ด' });
card(s, { x: 8.1, y: 3.45, w: W - 7.5, h: 1.55,
  head: 'จอหายไปก็ยังวิ่งได้', accent: THEME.colors.accent2,
  body: 'ถ้าหาจอไม่เจอ โปรแกรมจะข้ามส่วนวาดจอไป ส่วนอื่นทั้งหมดยังทำงานปกติ' });
callout(s, { x: M, y: 5.6, w: W, h: 0.9, tone: 'warn',
  title: 'แนะนำให้บัดกรีสี่จุดของจอ ไม่ใช่เสียบสายแพ',
  body: 'สายหลวมทำให้สัญญาณขาดกลางเฟรม จอจะค้างดำทั้งที่โค้ดทำงานถูกต้อง และเป็นอาการที่หาต้นตอยากที่สุด' });
s.addNotes('เรื่องสายหลวมเคยทำให้เสียเวลาไล่หาสาเหตุทั้งวันมาแล้ว ย้ำกับคนใหม่ให้บัดกรีตั้งแต่แรก');

// --- 9. เตรียม ST-Link ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'ติดตั้ง' });
s.addText('เตรียม ST-Link สำหรับอัป STM32', { placeholder: 'title' });
step(s, { x: M, y: 1.6, w: 5.7, n: 1, head: 'ลง STM32CubeProgrammer',
  body: 'โหลดจากเว็บ ST ต้องสมัครบัญชีฟรีก่อน  ลงแบบ default ไม่ต้องเปลี่ยน path  ไดรเวอร์ของ ST-Link มากับตัวนี้ด้วย' });
step(s, { x: M, y: 2.95, w: 5.7, n: 2, head: 'ต่อสาย 4 เส้น',
  body: 'ตามตารางด้านขวา  จับคู่ตามชื่อที่พิมพ์บนตัว ST-Link อย่าดูตามเลขพิน เพราะของแต่ละล็อตสลับตำแหน่งกัน' });
step(s, { x: M, y: 4.3, w: 5.7, n: 3, head: 'ตั้งค่าใน Arduino IDE',
  body: 'Upload method เลือก STM32CubeProgrammer (SWD)  ส่วนเมนู Port ปล่อยว่างไว้  ช่องที่เหลือทั้งหมดอยู่ในหน้าถัดไป' });
table(s, {
  x: 6.9, y: 1.55, w: W - 6.3, colW: [2.1, 3.7],
  head: ['ST-Link', 'Blue Pill'],
  rows: [
    ['SWDIO', 'DIO   (PA13)'],
    ['SWCLK', 'DCLK  (PA14)'],
    ['GND', 'GND'],
    ['3.3V', '3V3  ต่อเมื่อไม่มีไฟอื่น'],
  ],
  fontSize: 12.5,
});
callout(s, { x: 6.9, y: 3.7, w: W - 6.3, h: 1.4, tone: 'warn',
  title: 'หา PA13 PA14 บนแถวพินข้างบอร์ดไม่เจอแน่นอน',
  body: 'ไล่ดูจะเห็นเลขกระโดดจาก A12 ไป A15 เลย  สองขานี้ถูกแยกไปไว้ที่หัวต่อ 4 พินตรงปลายบอร์ด ฝั่งตรงข้าม USB และพิมพ์ชื่อว่า DIO กับ DCLK' });
callout(s, { x: M, y: 5.8, w: W, h: 0.85, tone: 'info',
  title: 'สิ่งที่ได้มาจากการเปลี่ยนมาใช้ ST-Link',
  body: 'ไม่ต้องสลับ BOOT0 ไปมาอีกเลย  และไม่แย่ง COM port กับ Serial Monitor เพราะการอัปไม่ได้วิ่งผ่านพอร์ต COM' });
s.addNotes('สองเรื่องนี้คือต้นเหตุที่ทำให้อัป STM32 ไม่ติดมาตลอดในวิธีเดิม คือลืมสลับ BOOT0 กลับ กับ Serial Monitor จองพอร์ตค้างไว้\n\nพอเปลี่ยนมาใช้ ST-Link ทั้งสองเรื่องหายไปพร้อมกัน เสียบ ST-Link ค้างไว้ได้ตลอดด้วย เพราะ PA13 PA14 ไม่ได้ถูกใช้ที่ไหนในโค้ดหุ่นเลย');

// --- 10. ตั้งค่า Arduino IDE ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'ติดตั้ง' });
s.addText('ตั้งค่า Arduino IDE สำหรับ STM32', { placeholder: 'title' });
table(s, {
  x: M, y: 1.5, w: W, colW: [3.2, 4.1, 4.8],
  head: ['เมนูใน Tools', 'ตั้งเป็น', 'ถ้าตั้งผิดจะเจออะไร'],
  rows: [
    ['Board', 'Generic STM32F1 series', 'ไม่มีเมนู Upload method ให้เห็นเลยสักช่อง'],
    ['Board part number', 'BluePill F103C8', ''],
    ['Upload method', 'STM32CubeProgrammer (SWD)', 'อัปไม่ติด เพราะไปเรียกวิธีอื่น'],
    ['U(S)ART support', "Enabled (generic 'Serial')", 'ไม่มี Serial เลยสักตัว ล็อกหายหมด'],
    ['USB support', 'None', 'Serial ย้ายไปออกช่อง USB  ล็อกที่ PA9 เงียบไปเฉยๆ ไม่มี error บอก'],
    ['Optimize, Debug, C Runtime', 'ปล่อยค่าเริ่มต้นไว้ทั้งหมด', ''],
    ['Port', 'ปล่อยว่าง ไม่ต้องเลือก', ''],
  ],
  fontSize: 12.5,
});
callout(s, { x: M, y: 5.45, w: W, h: 1.05, tone: 'info',
  title: 'สามแถวที่มีคำอธิบายช่องขวา คือสามช่องที่พลาดแล้วหาสาเหตุยากที่สุด',
  body: 'ที่เหลือตั้งผิดก็ยังคอมไพล์ผ่านและใช้งานได้ ไม่ต้องกังวล\nส่วนช่อง Port ที่ว่างอยู่คือถูกต้องแล้ว เพราะ ST-Link ไม่เคยโผล่มาเป็น COM port ตั้งแต่แรก' });
s.addNotes('แถว Board คือจุดที่พลาดกันบ่อยที่สุด ถ้าเปิดเมนู Tools แล้วไม่เห็นคำว่า Upload method เลย แปลว่ายังเลือกบอร์ดจากคอร์อื่นอยู่ ไม่ใช่คอร์ STM32 ตัวทางการ\n\nแถว USB support หลอกที่สุด เพราะถ้าเผลอเลือก CDC มันจะย้าย Serial ไปออกทางช่อง USB ของบอร์ด ล็อกที่ขา PA9 จะเงียบสนิทโดยไม่มีอะไรฟ้อง ทั้งที่โค้ดทำงานปกติทุกอย่าง\n\nBluePill F103C8 คือ 64 KB ส่วนโค้ดใช้ไปราว 25 KB ถ้าวันไหนโตเกิน ค่อยเปลี่ยนไปเลือก BluePill F103CB ซึ่งปลดล็อกเป็น 128 KB ที่ชิปมีจริงอยู่แล้ว');

// --- 11. อัปเฟิร์มแวร์ ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'ติดตั้ง' });
s.addText('อัปเฟิร์มแวร์ทีละบอร์ด', { placeholder: 'title' });
step(s, { x: M, y: 1.6, w: 5.7, n: 1, head: 'STM32  ผ่าน ST-Link',
  body: 'เปิดโฟลเดอร์ stm32_robot_controller  เลือกบอร์ด Generic STM32F1 series แล้วกด Upload ได้เลย ไม่ต้องแตะ BOOT0 และไม่ต้องเลือก Port' });
step(s, { x: M, y: 3.1, w: 5.7, n: 2, head: 'ESP32 รีโมท  ผ่านสาย USB',
  body: 'เปิดโฟลเดอร์ esp32_remote_controller  เลือกบอร์ด ESP32 Dev Module  ต้องลงไลบรารี Adafruit GFX และ Adafruit SH110X ก่อน ไม่งั้นคอมไพล์ไม่ผ่าน' });
step(s, { x: M, y: 4.6, w: 5.7, n: 3, head: 'ESP32-CAM  ผ่าน USB-TTL',
  body: 'บอร์ดนี้ไม่มีช่อง USB  ต้องต่อ IO0 ลง GND ก่อนกดอัป แล้วถอดออกเมื่ออัปเสร็จ  เลือกบอร์ด AI Thinker ESP32-CAM' });
callout(s, { x: 6.9, y: 1.6, w: W - 6.3, h: 1.5, tone: 'warn',
  title: "ขึ้นว่า undefined reference to 'Serial2'",
  body: 'ไม่ใช่โค้ดผิด  คอร์ STM32 ตัวทางการไม่ได้สร้าง Serial2 กับ Serial3 ให้เอง สเก็ตช์จึงสร้างไว้ให้แล้วสองบรรทัดตรงหัวไฟล์  ห้ามลบทิ้ง' });
callout(s, { x: 6.9, y: 3.3, w: W - 6.3, h: 2.0, tone: 'info',
  title: 'เช็กว่าอัปติดจริงไหม',
  body: 'ฝั่ง ST-Link จะขึ้นคำว่า Download verified successfully ซึ่งแปลว่าอ่านกลับมาเทียบแล้วตรงทุกไบต์\n\nอีกทางคือเปิด Serial Monitor ที่ 115200 แล้วกดรีเซ็ต ดูว่าวันเวลาคอมไพล์ตรงกับที่เพิ่งอัปไหม' });
s.addNotes('ลำดับการอัปไม่สำคัญ แต่ที่สำคัญคือต้องเปลี่ยน Tools > Board ให้ตรงกับบอร์ดที่กำลังจะอัปทุกครั้ง\n\nลืมเปลี่ยนแล้วกดอัปคือความผิดพลาดที่เจอบ่อยที่สุด โค้ดจะถูกคอมไพล์ด้วยบอร์ดผิด ซึ่งบางทีก็อัปติดด้วยแล้วไปเพี้ยนตอนรัน\n\nถ้าแก้แค่โค้ดฝั่งกล้อง ก็อัปแค่ esp32_camera_server พอ ไม่ต้องอัปใหม่ทั้งสามตัว');

// --- 10. คั่น: ใช้งาน ---
pres.addSection({ title: 'ใช้งานจริง' });
s = pres.addSlide({ masterName: 'SECTION', sectionTitle: 'ใช้งานจริง' });
s.addText('02', { placeholder: 'eyebrow' });
s.addText('เปิดใช้งานจริง', { placeholder: 'title' });
s.addText('ลำดับการเปิด สิ่งที่เห็นบนจอ หน้าเว็บควบคุม และข้อมูลที่เก็บได้', { placeholder: 'body' });

// --- 11. ลำดับการเปิด ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'ใช้งานจริง' });
s.addText('เปิดใช้งาน 6 ขั้นตอน', { placeholder: 'title' });
{
  const colW = 5.7, gap = W - colW * 2;
  const items = [
    ['เปิดไฟหุ่น', 'จอจะขึ้นหน้าต้อนรับ 2.5 วินาที แล้วเปลี่ยนเป็นหน้าสถานะเอง'],
    ['ต่อ Wi-Fi ของหุ่น', 'ชื่อ My_Robot2 รหัส password1234  ต่อทั้งมือถือและเครื่อง PC'],
    ['เปิดหน้าเว็บ', 'พิมพ์ 192.168.4.1 ในเบราว์เซอร์  ภาพกล้องจะขึ้นเองเมื่อกล้องเกาะ Wi-Fi ได้'],
    ['เลื่อนความเร็ว', 'แถบเลื่อนเริ่มที่ 0 เสมอ ถ้าไม่เลื่อนขึ้นหุ่นจะไม่ขยับแม้อยู่ในโหมด Auto'],
    ['รันสคริปต์บน PC', 'สั่ง py pc_qr_scanner/qr_scanner.py  สคริปต์จะไล่หากล้องเองในวง'],
    ['วางหุ่นบนเส้น', 'ให้เซนเซอร์ตัวกลางทับเส้นดำ แล้วสลับไปโหมด Auto'],
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
  title: 'ไม่มีกล้องก็รันสคริปต์ได้',
  body: 'ถ้าหากล้องไม่เจอ สคริปต์จะทำงานต่อโดยไม่อ่าน QR แต่ยังเก็บข้อมูลการวิ่งลง CSV ครบเหมือนเดิม' });
s.addNotes('ข้อ 4 เป็นข้อที่คนใหม่พลาดบ่อยที่สุด เห็นหุ่นไม่ขยับแล้วคิดว่าเสีย ทั้งที่แค่ยังไม่ได้เลื่อนความเร็วขึ้นจากศูนย์\n\nที่ตั้งให้เริ่มที่ 0 เพราะไม่อยากให้หุ่นพุ่งทันทีที่เปิดหน้าเว็บ');

// --- 12. จอ OLED ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'ใช้งานจริง' });
s.addText('จอบอกอะไรเราบ้าง', { placeholder: 'title' });
table(s, {
  x: M, y: 1.5, w: W, colW: [2.6, 4.5, 5.0],
  head: ['หน้าจอ', 'ขึ้นตอนไหน', 'ความหมาย'],
  rows: [
    ['หน้าต้อนรับ', '2.5 วินาทีแรกหลังเปิดไฟ', 'บอร์ดบูตเสร็จแล้ว จอกับสายทำงานปกติ'],
    ['หน้าสถานะ', 'ตอนหุ่นหยุดอยู่กับที่', 'บอกโหมด ความเร็ว และ IP ของกล้อง'],
    ['ตากวาดซ้ายขวา', 'ตลอดเวลาที่หุ่นวิ่ง', 'หุ่นกำลังมองหา QR ตรงหน้า 600 ms แล้วมองซ้ายขวาข้างละ 450 ms'],
    ['หน้าเจอแล้ว', '1 วินาทีแรกหลังอ่าน QR ได้', 'รหัสผ่านเข้ามาแล้ว กำลังจะหยุด'],
    ['รูปสัตว์', 'ช่วงที่เหลือของการหยุด 5 วินาที', 'เฉลยว่าที่อ่านได้คือสัตว์อะไร'],
  ],
  fontSize: 12.5,
});
callout(s, { x: M, y: 5.5, w: W, h: 1.0, tone: 'info',
  title: 'จอคือเครื่องมือไล่ปัญหาที่ดีที่สุดที่มี',
  body: 'ไม่ต้องต่อสายคอมก็รู้ว่าบอร์ดถึงไหนแล้ว  ถ้าจอยังขึ้นหน้าต้อนรับค้างอยู่ แปลว่าโปรแกรมค้างตั้งแต่ตอนบูต ไม่ใช่เรื่อง Wi-Fi' });
s.addNotes('ทุกหน้ายกเว้นหน้าสถานะเป็นรูปบิตแมปเต็มจอ 128x64 ที่วาดตัวหนังสือไว้ในรูปแล้ว\n\nถ้าอยากเปลี่ยนรูป ให้แก้ไฟล์ PNG ใน assets/oled แล้วสั่ง py tools/png_to_bitmaps.py');

// --- 13. หน้าเว็บ ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'ใช้งานจริง' });
s.addText('หน้าเว็บควบคุม', { placeholder: 'title' });
{
  const cw = (W - 0.4 * 2) / 3, ch = 1.72;
  const items = [
    ['ปุ่มทิศทาง', 'เดินหน้า ถอยหลัง เลี้ยวซ้าย เลี้ยวขวา และหยุด ใช้ได้เฉพาะในโหมด Manual', THEME.colors.accent1],
    ['แถบความเร็ว', 'เริ่มที่ 0 เสมอ ค่าที่ตั้งคือค่าที่มอเตอร์ได้รับจริง ไม่มีการปัดขึ้นให้', THEME.colors.accent1],
    ['แถบ Trim', 'แก้อาการหุ่นเบี่ยงข้างเดียว ปรับได้ขณะหุ่นกำลังวิ่ง ไม่ต้องอัปโค้ดใหม่', THEME.colors.accent1],
    ['สวิตช์ Auto / Manual', 'Auto คือเดินตามเส้นเอง  Manual คือสั่งด้วยปุ่ม  สลับได้ตลอดเวลา', THEME.colors.accent2],
    ['ภาพจากกล้อง', 'ขึ้นเองเมื่อกล้องเกาะ Wi-Fi ได้แล้ว ถ้ากล้องเงียบไปนานภาพจะหายไปเอง', THEME.colors.accent2],
    ['ไม่มีตัวเลขสถานะ', 'ตั้งใจไม่ใส่ เพราะหน้านี้มีไว้ขับ ส่วนตัวเลขทั้งหมดไปอยู่ในไฟล์ Excel แทน', THEME.colors.accent1],
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
  title: 'เปิดหน้าเว็บแล้วหมุนค้าง ทั้งที่ต่อ Wi-Fi ถูกแล้ว',
  body: 'มือถือบางรุ่นเห็นว่าวงนี้ไม่มีอินเทอร์เน็ตแล้วแอบสลับกลับไปใช้เน็ตมือถือเอง ต้องเข้าไปปิดการสลับอัตโนมัติก่อน' });
s.addNotes('ถ้าเปิดหน้าเว็บไม่ขึ้น ให้เช็กว่ามือถือต่อ Wi-Fi ชื่อ My_Robot2 อยู่จริงไหม\n\nมือถือบางรุ่นเห็นว่า Wi-Fi วงนี้ไม่มีอินเทอร์เน็ตแล้วแอบสลับกลับไปใช้เน็ตมือถือเอง ต้องเข้าไปปิดการสลับอัตโนมัติก่อน');

// --- 14. ข้อมูลที่เก็บ ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'ใช้งานจริง' });
s.addText('ข้อมูลที่เก็บลง Excel', { placeholder: 'title' });
table(s, {
  x: M, y: 1.5, w: 7.5, colW: [2.6, 4.9],
  head: ['คอลัมน์', 'เก็บอะไร'],
  rows: [
    ['Timestamp  Event', 'เวลา และชนิดของแถว คือ START, RUN ทุกวินาที หรือ QR'],
    ['Mode  Action', 'โหมด และหุ่นกำลังทำอะไร เช่น STRAIGHT, BEND, SEARCH, FINISH'],
    ['Base_Speed', 'ความเร็วที่แถบเลื่อนสั่ง'],
    ['Left_PWM  Right_PWM', 'ค่าที่มอเตอร์ได้รับจริง หลังหัก Trim แล้ว'],
    ['Turn  Error', 'แรงเลี้ยวที่คำนวณได้ และระยะห่างจากเส้น'],
    ['Kp  Kd  Trim', 'ค่าที่ใช้อยู่ตอนนั้น เปิดไฟล์เก่าแล้วรู้ทันทีว่าจูนด้วยค่าอะไร'],
    ['S_L2 ถึง S_R2', 'เซนเซอร์ทีละดวง เลข 1 คือดวงนั้นทับเส้นดำอยู่'],
  ],
  fontSize: 12,
});
s.addText('เขียนหนึ่งแถวทุกวินาที บวกอีกหนึ่งแถวทุกครั้งที่อ่าน QR ได้  ลากคอลัมน์ Turn ไปพล็อตกราฟใน Excel จะเห็นทันทีว่าหุ่นออกแรงสู้ตรงไหนของสนามมากที่สุด', {
  x: M, y: 5.1, w: 7.5, h: 0.8, fontSize: 12, color: THEME.colors.accent6,
  align: 'left', valign: 'top', isTextBox: true, margin: 0, lineSpacingMultiple: 1.25,
});
card(s, { x: 8.4, y: 1.5, w: W - 7.8, h: 1.9,
  head: 'ไฟล์อยู่ที่ไหน', accent: THEME.colors.accent1,
  body: 'สร้างไว้บนเดสก์ท็อป ชื่อ robot_mission_log.csv  สคริปต์หาเดสก์ท็อปจริงให้เอง แม้จะย้ายไปอยู่ใน OneDrive' });
card(s, { x: 8.4, y: 3.6, w: W - 7.8, h: 1.9, dark: true,
  head: 'ปิด Excel ก่อนรันใหม่', accent: THEME.colors.accent1,
  body: 'Windows ล็อกไฟล์ที่เปิดค้างอยู่  ถ้าเขียนทับไม่ได้ สคริปต์จะสร้างไฟล์ชื่อใหม่ที่มีเวลาต่อท้ายให้แทน' });
s.addNotes('ที่แยกเซนเซอร์เป็นคอลัมน์ละดวง เพราะถ้ายุบเป็น 00100 ช่องเดียว Excel จะอ่านเป็นตัวเลขแล้วตัดศูนย์หน้าทิ้งจนเหลือ 100\n\nเอาคอลัมน์ Turn ไปพล็อตกราฟเทียบกับเวลา จะเห็นเลยว่าหุ่นออกแรงสู้ตรงไหนของสนามมากที่สุด');

// --- 15. คั่น: ปรับแต่ง ---
pres.addSection({ title: 'ปรับแต่งและแก้ปัญหา' });
s = pres.addSlide({ masterName: 'SECTION', sectionTitle: 'ปรับแต่งและแก้ปัญหา' });
s.addText('03', { placeholder: 'eyebrow' });
s.addText('ปรับแต่งและแก้ปัญหา', { placeholder: 'title' });
s.addText('ค่าที่ปรับบ่อย วิธีที่หุ่นใช้หาเส้น และอาการเสียที่เจอซ้ำๆ', { placeholder: 'body' });

// --- 16. ค่าที่ปรับบ่อย ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'ปรับแต่งและแก้ปัญหา' });
s.addText('ค่าที่ปรับบ่อยที่สุด', { placeholder: 'title' });
table(s, {
  x: M, y: 1.5, w: W, colW: [3.1, 1.3, 7.7],
  head: ['ชื่อค่า', 'ค่าตั้งต้น', 'ปรับเมื่อไหร่'],
  rows: [
    ['Kp', '45', 'หุ่นเข้าโค้งกว้างเกินให้เพิ่ม  ถ้าส่ายไปมาบนทางตรงให้ลด'],
    ['Kd', '35', 'ช่วยหน่วงไม่ให้แกว่ง เพิ่มเมื่อหุ่นแกว่งหลังเข้าโค้ง'],
    ['leftMotorOffset', '25', 'แก้อาการเบี่ยงข้างเดียว ปรับจากแถบ Trim บนหน้าเว็บได้เลย'],
    ['MIN_PWM', '0', 'สั่งเท่าไหร่ได้เท่านั้น ตั้ง 135 ถ้าอยากให้ค่าต่ำๆ ถูกดันขึ้นจนล้อหมุน'],
    ['LOST_CONFIRM_MS', '150', 'เส้นหายได้นานแค่ไหนก่อนหุ่นจะเริ่มหา เพิ่มถ้าหุ่นสะดุดตอนเข้าโค้งแคบ'],
    ['FINISH_CONFIRM_MS', '40', 'ดำครบ 5 ดวงนานแค่ไหนถึงนับเป็นเส้นชัย สั้นพอที่จะไม่วิ่งเลยเส้น'],
  ],
  fontSize: 12.5,
});
callout(s, { x: M, y: 4.75, w: W, h: 1.05, tone: 'warn',
  title: 'Kp สูงเกินไปทำให้หุ่นหมุนรอบตัวเองแทนที่จะเลี้ยว',
  body: 'ค่า Error สูงสุดคือ 4  ที่ Kp เท่ากับ 80 แรงเลี้ยวจะพุ่งถึง 320 ซึ่งเกินช่วง 255 ที่มอเตอร์รับได้ ล้อนอกจะตันและล้อในกลับทาง' });
s.addNotes('ปรับทีละค่า แล้วลองวิ่งจริงทุกครั้ง อย่าปรับพร้อมกันหลายค่า เพราะจะแยกไม่ออกว่าอันไหนช่วย\n\nค่า Kp กับ Kd ถูกเก็บลง CSV ทุกแถว เปิดไฟล์เก่ามาดูแล้วรู้ทันทีว่ารอบนั้นจูนด้วยค่าอะไร');

// --- 17. หุ่นหาเส้นยังไง ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'ปรับแต่งและแก้ปัญหา' });
s.addText('หุ่นหาเส้นที่หลุดไปยังไง', { placeholder: 'title' });
s.addText('เซนเซอร์ไม่เจอเส้นเลย ไม่ได้แปลว่าหลงทาง  ทางโค้งแคบจะเหวี่ยงเส้นออกนอกแถวเซนเซอร์ชั่วครู่เป็นเรื่องปกติ', {
  x: M, y: 1.35, w: W, h: 0.45, fontSize: 14, color: '4A5568', align: 'left', isTextBox: true, margin: 0,
});
{
  const cw = (W - 0.4 * 2) / 3;
  const items = [
    ['รอก่อน  150 ms', 'ยังไม่ตัดสินใจอะไรทั้งนั้น เลี้ยวค้างไปทางเดิมต่อ ซึ่งเป็นทิศที่ไล่ตามเส้นอยู่แล้ว ทางโค้งส่วนใหญ่จบก่อนเวลานี้'],
    ['ถอยหลัง  400 ms', 'ถอยเอียงไปทางที่เพิ่งเห็นเส้นครั้งสุดท้าย แก้กรณีเข้าโค้งเร็วเกินจนหลุด'],
    ['กวาดหา  ถึง 6 วินาที', 'หมุนอยู่กับที่ด้วยความเร็ว 55% สลับซ้ายขวา แต่ละรอบกว้างขึ้นเรื่อยๆ ครบ 6 วินาทีแล้วไม่เจอถึงจะยอมแพ้'],
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
  title: 'ขาวหมดกับดำหมด คนละความหมายกันคนละขั้ว',
  body: 'ไม่เจอเส้นเลยแปลว่าเส้นอยู่นอกแถวเซนเซอร์ หุ่นต้องดันตัวเองไปหาจนกว่าจะมีดวงใดดวงหนึ่งเจอ  ส่วนที่สั่งให้หยุดคือตรงข้ามกันพอดี คือดำครบทั้ง 5 ดวง ซึ่งแปลว่าถึงเส้นชัยแล้ว' });
s.addNotes('การกวาดหาเส้นตั้งใจให้ช้ากว่าการเลี้ยวปกติ ถ้าหมุนเร็วเกิน เส้นจะวิ่งผ่านใต้แถวเซนเซอร์เร็วกว่าที่เซนเซอร์จะอ่านทัน แล้วหุ่นจะเลยจุดที่ข้ามเส้นไปแล้วโดยไม่รู้ตัว\n\nทิศที่กวาดรอบแรกถูกเลือกครั้งเดียวตอนเริ่มหา ไม่คำนวณใหม่ระหว่างทาง ไม่งั้นจะกวาดไปทางเดิมซ้ำๆ แล้วเดินห่างจากเส้นออกไปเรื่อยๆ');

// --- 18. อาการที่เจอบ่อย ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'ปรับแต่งและแก้ปัญหา' });
s.addText('อาการที่เจอบ่อยและวิธีแก้', { placeholder: 'title' });
table(s, {
  x: M, y: 1.4, w: W, colW: [3.6, 4.0, 4.5],
  head: ['อาการ', 'สาเหตุที่เจอบ่อยที่สุด', 'วิธีแก้'],
  rows: [
    ['จอไม่ขึ้นตอนเปิดครั้งที่สอง', 'สาย I2C ถูกจอกดค้างจากรอบที่แล้ว', 'โค้ดรุ่นใหม่ปลดให้เองตอนบูต  ถ้ายังไม่ขึ้นให้ถอดไฟทิ้ง 10 วินาที'],
    ['อัป STM32 ไม่ได้', 'เลือกบอร์ดผิดชุด หรือยังไม่ได้ลง STM32CubeProgrammer', 'เลือก Generic STM32F1 series และ Upload method เป็น SWD'],
    ["undefined reference to 'Serial2'", 'คอร์ทางการไม่ได้สร้าง Serial2 Serial3 ให้เอง', 'สเก็ตช์สร้างไว้ให้แล้วตรงหัวไฟล์ อย่าลบสองบรรทัดนั้นทิ้ง'],
    ['เปิดหน้าเว็บไม่ได้', 'ยังไม่ได้ต่อ Wi-Fi ชื่อ My_Robot2', 'ต่อ Wi-Fi ก่อน แล้วปิดการสลับไปเน็ตมือถืออัตโนมัติในเครื่อง'],
    ['หุ่นเบี่ยงไปข้างเดียวตลอด', 'มอเตอร์สองตัวแรงไม่เท่ากัน', 'ปรับแถบ Trim บนหน้าเว็บขณะหุ่นวิ่ง ไม่ต้องอัปโค้ดใหม่'],
    ['หุ่นหยุดเองกลางสนาม', 'อ่านขั้วเซนเซอร์กลับด้าน เลยนึกว่าถึงเส้นชัย', 'ดูบรรทัด black ใน Serial  วางบนเส้นแล้วต้องได้ 0 0 1 0 0'],
    ['ไฟล์ CSV ไม่อัปเดต', 'เปิดไฟล์ค้างไว้ใน Excel', 'ปิดไฟล์ใน Excel แล้วรันสคริปต์ใหม่'],
    ['ภาพกล้องเป็นเส้นพาดขวาง', 'สัญญาณพิกเซลเร็วเกินจนรับไม่ทันบางแถว', 'โค้ดรุ่นใหม่ลดความเร็วสัญญาณลงครึ่งหนึ่งแล้ว ให้อัปเฟิร์มแวร์กล้องใหม่'],
    ['เลื่อนความเร็วแล้วหุ่นไม่ขยับ', 'แถบเลื่อนยังอยู่ที่ 0 หรือค่าต่ำเกินกว่าที่ล้อจะหมุน', 'เลื่อนขึ้นให้เกิน 135  ค่าที่ต่ำกว่านั้นมอเตอร์จะร้องแต่ไม่หมุน'],
    ['อ่าน QR ไม่ออกสักที', 'ภาพเบลอ หรือ QR อยู่ไกลเกินไป', 'ขยับให้ใกล้ขึ้น และส่องให้แสงตกบน QR ไม่ใช่เงาของตัวหุ่น'],
  ],
  fontSize: 12.5,
  headSize: 13,
});
s.addNotes('หน้านี้คือหน้าที่คนใหม่จะเปิดดูบ่อยที่สุด บอกให้เขาถ่ายรูปเก็บไว้ในมือถือได้เลย\n\nทุกข้อในตารางนี้เคยเกิดขึ้นจริงมาแล้วทั้งหมด ไม่ใช่กรณีสมมติ');

// --- 19. เช็กลิสต์ ---
s = pres.addSlide({ masterName: 'CONTENT', sectionTitle: 'ปรับแต่งและแก้ปัญหา' });
s.addText('เช็กลิสต์ก่อนวิ่งจริง', { placeholder: 'title' });
{
  const colW = 5.7, gap = W - colW * 2;
  const items = [
    'แบตเตอรี่เต็ม และจ่ายไฟมอเตอร์แยกจากบอร์ด',
    'สายจอ OLED ทั้งสี่เส้นบัดกรีแน่น ไม่ใช่แค่เสียบ',
    'สายสื่อสารไขว้ถูกคู่ และต่อ GND ร่วมกันครบ',
    'เปิด Serial Monitor เช็กว่าวันเวลาคอมไพล์ตรงกับที่เพิ่งอัป',
    'วางหุ่นบนเส้นแล้วอ่านค่าได้ 0 0 1 0 0',
    'มือถือและ PC ต่อ Wi-Fi ชื่อ My_Robot2 แล้วทั้งคู่',
    'ปิดไฟล์ CSV ใน Excel ให้หมดก่อนรันสคริปต์',
    'ลองโหมด Manual ให้เดินหน้าถอยหลังได้ก่อน ค่อยสลับไป Auto',
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
  title: 'ไล่จากบนลงล่างทุกครั้ง',
  body: 'แปดข้อนี้ใช้เวลาไม่ถึงสองนาที แต่ตัดสาเหตุที่ทำให้เสียเวลาทั้งวันออกไปได้เกือบหมด' });
s.addNotes('แนะนำให้พิมพ์หน้านี้แปะไว้ข้างสนามทดสอบ');

// --- 20. ปิดท้าย ---
s = pres.addSlide({ masterName: 'TITLE_DARK', sectionTitle: 'ปรับแต่งและแก้ปัญหา' });
s.addText('อ่านต่อได้ที่ไหน', { placeholder: 'eyebrow' });
s.addText('เริ่มลงมือได้เลย', { placeholder: 'title' });
s.addText('ทุกอย่างในสไลด์ชุดนี้มีรายละเอียดเต็มอยู่ในไฟล์ README.md ของโปรเจกต์ พร้อมเหตุผลว่าทำไมถึงตั้งค่าแต่ละตัวไว้แบบนั้น', { placeholder: 'body' });
{
  const cw = (W - 0.4 * 2) / 3;
  const items = [
    ['README.md', 'คู่มือฉบับเต็ม ทุกค่าคงที่และเหตุผลเบื้องหลัง'],
    ['tests/', 'สเก็ตช์ทดสอบจอและบัส I2C ไว้แยกว่าปัญหาอยู่ที่สายหรือที่โค้ด'],
    ['assets/oled/', 'ไฟล์ภาพต้นฉบับของทุกหน้าจอ แก้แล้วแปลงใหม่ได้'],
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
s.addNotes('ปิดท้ายด้วยการเปิด README.md ให้ดูจริงสักรอบ จะได้รู้ว่าเวลาติดปัญหาต้องไปเปิดที่ไหน');

// ---------- เขียนไฟล์ ----------

// บอก PowerPoint ให้ใช้ฟอนต์เดียวกันกับภาษาไทยด้วย
//
// PowerPoint แยกฟอนต์ออกเป็นหลายช่องตามชนิดของอักษร ภาษาไทยนับเป็น complex script
// จึงไม่ได้ใช้ช่อง latin ที่ pptxgenjs ตั้งค่าให้ แต่ไปหยิบจากช่องของตัวเองแทน
// ซึ่งค่าปริยายคือ Angsana New ที่เป็นเซริฟเส้นบางมาก ตัวเล็กและอ่านบนโปรเจกเตอร์ไม่ออก
// ผลคือสไลด์ออกมามีสองฟอนต์ปนกัน อังกฤษเป็นซานส์ ไทยเป็นเซริฟ ทั้งที่ตั้งไว้ตัวเดียว
async function useThaiFont(file, face) {
  const JSZip = require('jszip');
  const fs = require('fs');
  const zip = await JSZip.loadAsync(fs.readFileSync(file));
  const path = 'ppt/theme/theme1.xml';
  let xml = await zip.file(path).async('string');
  const before = xml;
  xml = xml.split('<a:font script="Thai" typeface="Angsana New"/>')
           .join('<a:font script="Thai" typeface="' + face + '"/>');
  xml = xml.split('<a:cs typeface=""/>').join('<a:cs typeface="' + face + '"/>');
  if (xml === before) throw new Error('หาช่องฟอนต์ไทยในธีมไม่เจอ ไฟล์อาจเปลี่ยนรูปแบบไปแล้ว');
  zip.file(path, xml);
  fs.writeFileSync(file, await zip.generateAsync({ type: 'nodebuffer', compression: 'DEFLATE' }));
}

const out = process.argv[2] || 'Tiny-Robot-Setup-Guide-TH.pptx';
pres.writeFile({ fileName: out }).then(async () => {
  const { applyTheme } = require(process.env.SKILL_DIR + '/scripts/apply_theme.js');
  await applyTheme(out, THEME);
  await useThaiFont(out, THEME.bodyFontFace);
  console.log('เขียนไฟล์เสร็จแล้ว: ' + out);
});
