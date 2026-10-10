"""แปลงรูป PNG ใน assets/oled/ ให้เป็น animal_bitmaps.h สำหรับจอ OLED SSD1306

รูปต้นฉบับเป็น 128x64 เท่าขนาดจอพอดี จึงไม่ต้องย่อหรือขยาย
แปลงเป็นขาวดำ 1 บิตด้วยวิธี Otsu ซึ่งหาเส้นแบ่งสว่าง/มืดจากตัวรูปเอง
จำเป็นเพราะรูปชุด scan วาดมาด้วยความสว่างต่ำ (สูงสุดแค่ 84-142 จาก 255)
ถ้าใช้เส้นแบ่งตายตัวที่ 128 รูปชุดนั้นจะกลายเป็นดำสนิททั้งภาพ

วิธีใช้:  py tools/png_to_bitmaps.py            ภาพทึบตามต้นฉบับ
         py tools/png_to_bitmaps.py --outline  เก็บเฉพาะเส้นขอบ กินไฟน้อยกว่า
ผลลัพธ์:  esp32_remote_controller/animal_bitmaps.h  (และแสดงภาพตัวอย่างเป็น ASCII)

อยากเปลี่ยนรูปก็แก้ไฟล์ PNG ใน assets/oled/ แล้วรันใหม่ ไม่ต้องแตะโค้ด
ชื่อไฟล์จะกลายเป็นชื่อตัวแปร เช่น Dog.png -> bmp_dog
"""

import os
import sys
from PIL import Image, ImageFilter

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

SRC_DIR = os.path.join("assets", "oled")
OUT_HEADER = os.path.join("esp32_remote_controller", "animal_bitmaps.h")
W, H = 128, 64  # ขนาดจอ OLED

# ลำดับในไฟล์ผลลัพธ์ จัดกลุ่มให้อ่านง่าย ไฟล์ที่ไม่อยู่ในนี้จะต่อท้ายให้เอง
ORDER = ["Start", "scan1", "scan2", "scan3", "Detected",
         "Dog", "cat", "Bird", "Lion", "Tiger"]


def otsu_threshold(hist):
    """หาเส้นแบ่งขาว/ดำที่แยกสองกลุ่มความสว่างได้ดีที่สุดในรูปนั้นๆ"""
    total = sum(hist)
    sum_all = sum(i * h for i, h in enumerate(hist))
    sum_bg = weight_bg = 0.0
    best_t, best_var = 0, -1.0
    for t in range(256):
        weight_bg += hist[t]
        if weight_bg == 0:
            continue
        weight_fg = total - weight_bg
        if weight_fg == 0:
            break
        sum_bg += t * hist[t]
        mean_bg = sum_bg / weight_bg
        mean_fg = (sum_all - sum_bg) / weight_fg
        var = weight_bg * weight_fg * (mean_bg - mean_fg) ** 2
        if var > best_var:
            best_var, best_t = var, t
    return best_t


def load_mono(path):
    img = Image.open(path).convert("L")
    if img.size != (W, H):
        print(f"  ปรับขนาดจาก {img.size} เป็น ({W}, {H})")
        img = img.resize((W, H), Image.LANCZOS)
    thr = otsu_threshold(img.histogram())
    return img.point(lambda p: 255 if p > thr else 0, mode="1"), thr


def to_outline(mono):
    """เก็บเฉพาะพิกเซลที่อยู่ขอบของรูป เพื่อให้จอติดไฟน้อยลง

    วิธีคือเอาภาพเดิมลบด้วยภาพที่ถูกกัดขอบเข้ามาหนึ่งพิกเซล
    พื้นที่ทึบใหญ่ๆ จะเหลือแค่โครงร่าง แต่เส้นบางอย่างตัวหนังสือจะถูกเก็บไว้ครบ
    เพราะพอกัดขอบทีเดียวมันหายไปทั้งเส้น ผลต่างจึงคือตัวมันเองทั้งหมด
    """
    grey = mono.convert("L")
    eroded = grey.filter(ImageFilter.MinFilter(3))  # กัดขอบเข้ามา 1 พิกเซล
    src, ero = grey.load(), eroded.load()
    out = Image.new("1", (W, H), 0)
    dst = out.load()
    for y in range(H):
        for x in range(W):
            dst[x, y] = 1 if (src[x, y] and not ero[x, y]) else 0
    return out


def count_lit(mono):
    px = mono.load()
    return sum(1 for y in range(H) for x in range(W) if px[x, y])


def to_bytes(mono):
    """รูปแบบที่ Adafruit_GFX ต้องการ: 1 บิตต่อพิกเซล เรียงบิตจากซ้ายไปขวา"""
    px = mono.load()
    data = []
    for y in range(H):
        for byte_x in range(W // 8):
            b = 0
            for bit in range(8):
                if px[byte_x * 8 + bit, y]:
                    b |= 0x80 >> bit
            data.append(b)
    return data


def preview(name, mono):
    px = mono.load()
    print(f"\n--- {name} ---")
    for y in range(0, H, 2):  # เว้นบรรทัด เพราะตัวอักษรในเทอร์มินัลสูงกว่ากว้าง
        print("".join("#" if px[x, y] else "." for x in range(W)))


def main():
    if not os.path.isdir(SRC_DIR):
        sys.exit(f"ไม่เจอโฟลเดอร์ {SRC_DIR}")

    stems = [os.path.splitext(f)[0] for f in os.listdir(SRC_DIR) if f.lower().endswith(".png")]
    ordered = [s for s in ORDER if s in stems] + sorted(s for s in stems if s not in ORDER)
    if not ordered:
        sys.exit(f"ไม่เจอไฟล์ .png ใน {SRC_DIR}")

    outline = "--outline" in sys.argv
    mode_note = "เก็บเฉพาะเส้นขอบเพื่อให้จอกินไฟน้อยลง" if outline else "ภาพทึบตามต้นฉบับ"

    lines = [
        "// สร้างอัตโนมัติโดย tools/png_to_bitmaps.py -- อย่าแก้ไฟล์นี้ด้วยมือ",
        "// โหมด: " + mode_note,
        "// ต้นฉบับเป็นไฟล์ PNG ใน assets/oled/ อยากเปลี่ยนรูปให้แก้ที่นั่นแล้วรันสคริปต์ใหม่",
        f"// รูปขาวดำ {W}x{H} เต็มจอ สำหรับ OLED SSD1306 (ใช้กับ Adafruit_GFX drawBitmap)",
        "#pragma once",
        "#include <Arduino.h>",
        "",
        f"#define OLED_BMP_W {W}",
        f"#define OLED_BMP_H {H}",
        "",
    ]

    total_before = total_after = 0
    for stem in ordered:
        mono, thr = load_mono(os.path.join(SRC_DIR, stem + ".png"))
        before = count_lit(mono)
        if outline:
            mono = to_outline(mono)
        after = count_lit(mono)
        total_before += before
        total_after += after
        preview(f"{stem}  (otsu={thr}, ติดไฟ {after} พิกเซล)", mono)
        data = to_bytes(mono)
        lines.append(f"// จาก assets/oled/{stem}.png")
        lines.append(f"const unsigned char PROGMEM bmp_{stem.lower()}[] = {{")
        for i in range(0, len(data), 12):
            lines.append("  " + ", ".join(f"0x{b:02X}" for b in data[i:i + 12]) + ",")
        lines.append("};")
        lines.append("")

    with open(OUT_HEADER, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))

    size = W * H // 8
    print(f"\nเขียนไฟล์แล้ว: {OUT_HEADER}")
    print(f"{len(ordered)} รูป x {size} ไบต์ = {len(ordered) * size} ไบต์ใน PROGMEM")
    print(f"โหมด: {mode_note}")
    if outline and total_before:
        cut = 100 - total_after * 100 // total_before
        print(f"พิกเซลที่ติดไฟรวม {total_before} -> {total_after} (ลดลง {cut}%)")
    else:
        print(f"พิกเซลที่ติดไฟรวม {total_after}")
    print("ตัวแปรที่ได้: " + ", ".join(f"bmp_{s.lower()}" for s in ordered))


if __name__ == "__main__":
    main()
