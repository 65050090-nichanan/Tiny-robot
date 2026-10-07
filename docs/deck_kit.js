// เครื่องมือวาดสไลด์ที่ใช้ร่วมกันระหว่างคู่มือฉบับไทยกับฉบับอังกฤษ
//
// สองฉบับใช้ธีม เลย์เอาต์ และรูปแบบการ์ดชุดเดียวกันทุกอย่าง ต่างกันแค่ข้อความ
// ถ้าปล่อยให้แต่ละฉบับถือโค้ดวาดของตัวเอง พอแก้ระยะห่างทีก็ต้องตามไปแก้สองที่
// แล้วสักพักสองฉบับก็จะหน้าตาไม่เหมือนกันโดยไม่มีใครตั้งใจ

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

const M = 0.6;            // ระยะขอบสไลด์
const W = 13.3 - M * 2;   // ความกว้างที่ใช้ได้ = 12.1

// สร้างเด็คเปล่าพร้อมธีม เลย์เอาต์ และชุดฟังก์ชันวาด
function createDeck({ title, author, subject, footer }) {
  const pres = new pptxgen();
  pres.layout = 'LAYOUT_WIDE';   // 13.3 x 7.5 นิ้ว -- ต้องตั้งก่อนเพิ่มสไลด์
  pres.theme = { headFontFace: THEME.headFontFace, bodyFontFace: THEME.bodyFontFace };
  pres.title = title;
  pres.author = author;
  pres.subject = subject;

  const C = pres.SchemeColor;

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
      { text: { text: footer, options: { x: M, y: 6.85, w: 8, h: 0.3,
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

  // กล่องคำสั่งพื้นเข้ม สำหรับบรรทัดที่ต้องพิมพ์ตามเป๊ะ
  function codeBox(slide, { x, y, w, h, text, size = 11 }) {
    slide.addShape(pres.shapes.ROUNDED_RECTANGLE, {
      x, y, w, h, rectRadius: 0.1,
      fill: { color: C.text1 }, line: { color: C.text1, width: 1 }, objectName: 'cmd-box',
    });
    slide.addText(text, {
      x: x + 0.28, y: y + 0.17, w: w - 0.56, h: h - 0.34,
      fontSize: size, color: THEME.colors.accent1, fontFace: 'Courier New',
      align: 'left', valign: 'top', isTextBox: true, margin: 0, lineSpacingMultiple: 1.2,
    });
  }

  // ---------- เขียนไฟล์ ----------

  // บอก PowerPoint ให้ใช้ฟอนต์เดียวกันกับภาษาไทยด้วย
  //
  // PowerPoint แยกฟอนต์ออกเป็นหลายช่องตามชนิดของอักษร ภาษาไทยนับเป็น complex script
  // จึงไม่ได้ใช้ช่อง latin ที่ pptxgenjs ตั้งค่าให้ แต่ไปหยิบจากช่องของตัวเองแทน
  // ซึ่งค่าปริยายคือ Angsana New ที่เป็นเซริฟเส้นบางมาก ตัวเล็กและอ่านบนโปรเจกเตอร์ไม่ออก
  // ผลคือสไลด์ออกมามีสองฟอนต์ปนกัน อังกฤษเป็นซานส์ ไทยเป็นเซริฟ ทั้งที่ตั้งไว้ตัวเดียว
  //
  // ฉบับอังกฤษก็แก้ด้วย เผื่อมีคำไทยหลุดเข้าไปในสไลด์หรือในโน้ตผู้บรรยาย
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

  async function finish(out) {
    await pres.writeFile({ fileName: out });
    const { applyTheme } = require(process.env.SKILL_DIR + '/scripts/apply_theme.js');
    await applyTheme(out, THEME);
    await useThaiFont(out, THEME.bodyFontFace);
    console.log('เขียนไฟล์เสร็จแล้ว: ' + out);
  }

  return { pres, C, M, W, THEME, card, numberDot, step, node, arrow, table, callout, codeBox, finish };
}

module.exports = { THEME, M, W, createDeck };
