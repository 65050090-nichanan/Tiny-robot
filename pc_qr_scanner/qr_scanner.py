import cv2
import numpy as np
from pyzbar.pyzbar import decode, ZBarSymbol
import requests
import socket
import sys
import threading
import time
import csv
import os
from datetime import datetime


# ESP32-CAM เกาะ Wi-Fi ชื่อ My_Robot2 ที่ ESP32 รีโมทปล่อยออกมา จึงได้ IP ในวง 192.168.4.x
# เครื่อง PC ต้องต่อ Wi-Fi My_Robot2 ด้วย ถึงจะคุยกับกล้องได้
#
# IP ของกล้องเปลี่ยนได้ทุกครั้งที่เปิดเครื่อง ขึ้นกับว่าใครมาเกาะ Wi-Fi ก่อน
# สคริปต์จึงไล่หาเองว่ากล้องอยู่ IP ไหน แทนที่จะให้คนมานั่งแก้ตัวเลขทุกครั้ง
# ถ้ารู้ IP อยู่แล้วก็ระบุได้:  py pc_qr_scanner/qr_scanner.py 192.168.4.3

UDP_PORT = 1234

# ESP32 รีโมทเป็นตัวปล่อย Wi-Fi จึงเป็นเกตเวย์ของวงเสมอ ไม่เปลี่ยนเหมือน IP กล้อง
ROBOT_IP = '192.168.4.1'
TELEMETRY_URL = f'http://{ROBOT_IP}/telemetry'
TELEMETRY_EVERY_S = 1.0     # บันทึกสถานะหุ่นลง CSV ทุกกี่วินาที


def find_camera(explicit=None):
    """หา IP ของ ESP32-CAM บนวง 192.168.4.x"""
    if explicit:
        candidates = [explicit]
    else:
        # 192.168.4.1 คือ ESP32 รีโมทเสมอ กล้องจึงได้ตั้งแต่ .2 ขึ้นไป
        candidates = [f'192.168.4.{n}' for n in range(2, 21)]
        print('🔍 กำลังไล่หากล้องในวง 192.168.4.2 ถึง 192.168.4.20 ...')

    for ip in candidates:
        try:
            r = requests.get(f'http://{ip}/capture', timeout=1.5)
            if r.status_code == 200 and len(r.content) > 1000:
                print(f'✅ เจอกล้องที่ {ip}')
                return ip
        except Exception:
            pass

    print('❌ หากล้องไม่เจอ -- จะทำงานต่อโดยไม่มีการอ่าน QR')
    print('   เช็ก 3 อย่างนี้')
    print('   1. PC ต่อ Wi-Fi ชื่อ My_Robot2 แล้วหรือยัง (รหัส password1234)')
    print('   2. ESP32-CAM เปิดอยู่และเกาะ Wi-Fi ได้แล้วหรือยัง ดูจาก Serial Monitor')
    print('   3. เปิด http://192.168.4.1 ดูบรรทัด CAM: ว่าขึ้น IP อะไร')
    return None



class CameraFeed:
    """รับภาพจากกล้องในเธรดแยก แล้วเก็บไว้แค่เฟรมล่าสุดเฟรมเดียว

    ของเดิมยิง GET /capture ทีละรูปอยู่ในลูปหลัก กว่าจะได้รูปหนึ่งต้องรอ
    ขอรูป -> กล้องถ่าย -> ส่งข้าม Wi-Fi -> ถอด JPEG -> ไล่หา QR เรียงต่อกันเป็นทอดๆ
    ทุกขั้นหยุดรอขั้นก่อนหน้าจนจบ ภาพที่ได้เลยมาเป็นช่วงๆ ราว 2-3 เฟรมต่อวินาที

    ที่นี่ใช้ช่อง :81/stream ซึ่งกล้องส่งรูปไหลมาเองไม่ต้องขอทีละรูป
    เธรดนี้ทำหน้าที่รับอย่างเดียว ลูปหลักหยิบเฟรมล่าสุดไปอ่าน QR
    เฟรมที่หยิบไม่ทันถูกทิ้งทันที ซึ่งเป็นสิ่งที่ต้องการ
    เพราะภาพเก่าคือตำแหน่งที่หุ่นวิ่งเลยมาแล้ว ไม่มีประโยชน์จะไปอ่าน QR จากมัน
    """

    def __init__(self, ip):
        self.stream_url = f'http://{ip}:81/stream'
        self.capture_url = f'http://{ip}/capture'
        self.use_stream = True
        self.frames = 0          # จำนวนภาพที่รับมาได้ นับตั้งแต่รายงานรอบที่แล้ว
        self.errors = 0
        self.last_error = ''
        self._frame = None
        self._lock = threading.Lock()
        self._stop = threading.Event()
        threading.Thread(target=self._run, daemon=True).start()

    def _publish(self, jpg):
        img = cv2.imdecode(np.frombuffer(jpg, dtype=np.uint8), cv2.IMREAD_COLOR)
        if img is None:
            return          # เฟรมขาดกลางทาง รอเฟรมหน้า ไม่ต้องรายงานเป็นข้อผิดพลาด
        with self._lock:
            self._frame = img

    def _pump_stream(self):
        """เกาะช่องภาพสดไว้ ตัดรูปออกมาทีละใบ คืน True ถ้าได้ภาพมาอย่างน้อยหนึ่งใบ"""
        got = False
        # timeout เป็นคู่: 4 วินาทีสำหรับต่อให้ติด, 8 วินาทีสำหรับรอข้อมูลก้อนถัดไป
        with requests.get(self.stream_url, stream=True, timeout=(4, 8)) as r:
            r.raise_for_status()
            buf = bytearray()
            for chunk in r.iter_content(chunk_size=8192):
                if self._stop.is_set():
                    return got
                buf += chunk

                # ดึงรูปที่ครบใบออกให้หมดก่อนไปรับก้อนถัดไป
                #
                # ต้องวนจนหมด ไม่ใช่ใบเดียวต่อหนึ่งก้อน
                # เพราะตอนลูปหลักไปรอหุ่นหยุด 6 วินาที ภาพจะกองค้างอยู่ในสายเป็นสิบใบ
                # ถ้าหยิบออกทีละใบต่อหนึ่งก้อน จะไล่ตามของเก่าไปเรื่อยๆ ไม่ทันปัจจุบันสักที
                #
                # ไม่ต้องแกะหัว multipart ให้ยุ่ง JPEG ทุกใบขึ้นต้นด้วย FFD8 ลงท้ายด้วย FFD9
                newest = None
                while True:
                    start = buf.find(b'\xff\xd8')
                    if start == -1:
                        break
                    end = buf.find(b'\xff\xd9', start + 2)
                    if end == -1:
                        break
                    newest = bytes(buf[start:end + 2])
                    del buf[:end + 2]
                    self.frames += 1

                if newest is not None:
                    # ได้มาหลายใบก็ถอดเฉพาะใบล่าสุด ใบก่อนหน้าหมดอายุไปแล้วตั้งแต่ยังไม่ได้ดู
                    # การถอด JPEG แพงกว่าการหาหัวรูปมาก จึงไม่ถอดของที่รู้อยู่แล้วว่าจะทิ้ง
                    self._publish(newest)
                    got = True
                elif len(buf) > 1_000_000:
                    del buf[:]   # ไม่เจอหัวรูปเลยสักที แปลว่าหลงจังหวะ ล้างทิ้งแล้วตั้งต้นใหม่
        return got

    def _poll_capture(self):
        """ทางสำรอง ขอทีละรูปแบบเดิม ใช้เมื่อช่องภาพสดเปิดไม่ขึ้น"""
        r = requests.get(self.capture_url, timeout=3)
        r.raise_for_status()
        self._publish(r.content)
        self.frames += 1

    def _run(self):
        fails = 0
        while not self._stop.is_set():
            try:
                if self.use_stream:
                    if self._pump_stream():
                        fails = 0
                    else:
                        fails += 1
                else:
                    self._poll_capture()
            except Exception as e:
                fails += 1
                self.errors += 1
                self.last_error = f'{type(e).__name__}: {e}'
                time.sleep(0.3)

            # ลองสามครั้งแล้วยังไม่ได้ภาพสักใบ ก็ถอยไปวิธีเดิมแทนที่จะวนลองไปเรื่อยๆ
            if self.use_stream and fails >= 3:
                self.use_stream = False
                print('⚠️  เปิดช่องภาพสดไม่ได้ ถอยไปขอทีละรูปที่ /capture แทน (ภาพจะกระตุกกว่า)')
                if self.last_error:
                    print(f'    เหตุผล: {self.last_error}')

    def latest(self):
        """คืนเฟรมที่ยังไม่เคยถูกหยิบ ถ้าไม่มีของใหม่คืน None

        ที่ต้องเคลียร์ทิ้งหลังหยิบ เพราะไม่อยากให้ลูปหลักเอารูปเดิมมาไล่หา QR ซ้ำ
        ซึ่งเปลืองซีพียูฟรีๆ แล้วก็ได้คำตอบเดิมทุกครั้ง
        """
        with self._lock:
            frame, self._frame = self._frame, None
        return frame

    def stop(self):
        self._stop.set()


# บอก zbar ให้มองหาเฉพาะ QR
#
# ค่าปริยายคือไล่ถอดรหัสทุกชนิดที่รู้จัก ทั้ง PDF417, Code128, EAN, DataBar
# ซึ่งไม่มีทางโผล่มาในสนามนี้สักอย่าง แต่กินเวลาถอดรหัสทุกเฟรมและทุกชั้น
# แล้วตัวถอด PDF417 ก็เป็นต้นเหตุของคำเตือน Assertion "g[0] >= 0 ..." ที่ท่วมจออยู่
# มันเจอลายมั่วๆ จากนอยส์ของกล้องแล้วเข้าใจว่าเป็นบาร์โค้ดที่พัง
QR_ONLY = [ZBarSymbol.QRCODE]

_cv_qr = cv2.QRCodeDetector()


def scan_for_qr(img):
    """ไล่หา QR ในภาพหนึ่งใบ คืนลิสต์ของ (ข้อความ, กรอบ)

    กล้องตัวนี้ให้ภาพเบลอและมีนอยส์ ส่งภาพดิบเข้า zbar ใบเดียวจึงมักอ่านไม่ออก
    จึงลองเป็นชั้นๆ แล้วหยุดทันทีที่ชั้นไหนอ่านได้ เพราะชั้นหลังแพงกว่าชั้นแรกหลายเท่า
    ส่วนใหญ่จบตั้งแต่ชั้นแรก ชั้นที่เหลือมีไว้เผื่อตอนแสงไม่เข้าข้าง
    """
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

    # แต่ละชั้นเก็บตัวคูณไว้ด้วย เพราะชั้นที่ขยายภาพจะให้พิกัดกรอบใหญ่เกินจริงเท่านั้นเท่า
    attempts = [
        # 1. ภาพเทาดิบ เร็วสุด และพอ QR อยู่ใกล้แสงดีก็จบตั้งแต่ชั้นนี้
        (gray, 1),
        # 2. ปรับแสงทีละหย่อม ใช้ตอนครึ่งรูปโดนเงาตัวหุ่นบัง ซึ่งการตัดขาวดำทั้งภาพเอาไม่อยู่
        (cv2.adaptiveThreshold(gray, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C,
                               cv2.THRESH_BINARY, 31, 5), 1),
        # 3. ขยายสองเท่า ใช้ตอน QR อยู่ไกลจนช่องเล็กกว่าที่ zbar จะแยกออก
        (cv2.resize(gray, None, fx=2, fy=2, interpolation=cv2.INTER_CUBIC), 2),
    ]

    for candidate, scale in attempts:
        found = decode(candidate, symbols=QR_ONLY)
        if found:
            out = []
            for b in found:
                x, y, w, h = b.rect
                out.append((b.data.decode('utf-8', 'replace').strip().upper(),
                            (x // scale, y // scale, w // scale, h // scale)))
            return out

    # ชั้นสุดท้าย ตัวถอดของ OpenCV คนละอัลกอริทึมกับ zbar
    # บางมุมที่ zbar ยอมแพ้ตัวนี้อ่านได้ และกลับกันก็มี จึงคุ้มที่จะลองทั้งคู่
    try:
        data, points, _ = _cv_qr.detectAndDecode(gray)
    except cv2.error:
        return []
    if data and points is not None:
        pts = points.astype(int).reshape(-1, 2)
        x, y = pts[:, 0].min(), pts[:, 1].min()
        w, h = pts[:, 0].max() - x, pts[:, 1].max() - y
        return [(data.strip().upper(), (x, y, w, h))]
    return []


def find_desktop():
    """หา path จริงของ Desktop

    เดา ~/Desktop ตรงๆ ไม่ได้ เพราะถ้าเปิด OneDrive ไว้ Windows จะย้าย Desktop
    ไปเป็น ~/OneDrive/Desktop และถ้าตั้งภาษาไทย ชื่อโฟลเดอร์ก็อาจไม่ใช่คำว่า Desktop
    จึงถาม Windows เอาที่อยู่จริงก่อน แล้วค่อยไล่เดาเป็นทางเลือกสำรอง
    """
    if os.name == 'nt':
        try:
            import ctypes
            CSIDL_DESKTOPDIRECTORY = 16
            buf = ctypes.create_unicode_buffer(260)
            ctypes.windll.shell32.SHGetFolderPathW(None, CSIDL_DESKTOPDIRECTORY, None, 0, buf)
            if buf.value and os.path.isdir(buf.value):
                return buf.value
        except Exception:
            pass

    home = os.path.expanduser('~')
    for path in (os.path.join(home, 'OneDrive', 'Desktop'), os.path.join(home, 'Desktop')):
        if os.path.isdir(path):
            return path
    return os.path.dirname(os.path.abspath(__file__))   # หาไม่เจอจริงๆ ก็เขียนไว้ข้างสคริปต์


# หัวตารางของ CSV หนึ่งคอลัมน์ต่อหนึ่งค่า เปิดใน Excel แล้วลากคอลัมน์ไปพล็อตกราฟได้เลย
# ถ้ายุบหลายค่าไว้ช่องเดียวจะต้องมานั่งแยกข้อความทีหลัง
# เซนเซอร์แยกเป็นช่องละดวง L2 คือซ้ายสุด R2 คือขวาสุด 1 = ดวงนั้นทับเส้นดำอยู่
# แยกช่องเพราะถ้ายุบเป็น 00100 ช่องเดียว Excel จะอ่านเป็นตัวเลขแล้วตัดศูนย์หน้าทิ้งจนเหลือ 100
HEADER = ['Timestamp', 'Event', 'Animal', 'Sent_Code',
          'Mode', 'Action', 'Base_Speed', 'Left_PWM', 'Right_PWM',
          'Turn', 'Error', 'Kp', 'Kd', 'Trim',
          'S_L2', 'S_L1', 'S_C', 'S_R1', 'S_R2', 'Sensors_On_Line']


def open_log():
    """สร้างไฟล์บันทึกผล แล้วคืน path ที่เขียนได้จริง

    Windows ล็อกไฟล์ที่เปิดค้างอยู่ใน Excel ไว้ทั้งไฟล์ เขียนทับไม่ได้เลย
    ซึ่งเกิดขึ้นตลอดเวลาเพราะคนเปิดดูผลรอบที่แล้วค้างไว้แล้วสั่งรันรอบใหม่
    ถ้าชื่อหลักเขียนไม่ได้ก็เติมเวลาต่อท้ายแล้วใช้ชื่อใหม่ ดีกว่าวิ่งไปทั้งรอบแล้วไม่ได้ข้อมูลเลย
    """
    desktop = find_desktop()
    stamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    for path in (os.path.join(desktop, 'robot_mission_log.csv'),
                 os.path.join(desktop, f'robot_mission_log_{stamp}.csv'),
                 os.path.join(os.path.dirname(os.path.abspath(__file__)),
                              f'robot_mission_log_{stamp}.csv')):
        try:
            with open(path, mode='w', newline='', encoding='utf-8-sig') as f:
                csv.writer(f).writerow(HEADER)
            print(f"✅ สร้างไฟล์บันทึกผลแล้ว: {path}")
            return path
        except PermissionError:
            print(f"⚠️  เขียน {os.path.basename(path)} ไม่ได้ (น่าจะเปิดค้างอยู่ใน Excel) จะลองชื่ออื่น")
        except Exception as e:
            print(f"⚠️  เขียน {path} ไม่ได้: {e}")

    print("❌ เขียนไฟล์บันทึกผลไม่ได้เลยสักที่")
    return None


LOG_FILE = open_log()

# แผนผังรหัสสัตว์
ANIMAL_MAP = {
    "DOG": b'D', "CAT": b'C', "BIRD": b'B', "LION": b'L', "TIGER": b'T'
}

def send_to_robot(code):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.sendto(code, (ESP32_IP, UDP_PORT))
    except: pass

# โหมด, กำลังทำอะไร, ความเร็วฐาน, PWM ซ้าย, PWM ขวา, ค่าเลี้ยว, error, Kp, Kd, trim,
# เซนเซอร์ 5 ดวง, จำนวนที่ทับเส้น
TELEMETRY_FIELDS = 16
_telemetry_problem = ''  # ปัญหาล่าสุดที่บอกไปแล้ว ไว้กันพิมพ์ซ้ำทุกวินาที


def read_telemetry(session):
    """ดึงสถานะล่าสุดของหุ่นจาก ESP32 รีโมท

    ตอบกลับเป็นบรรทัดเดียว เช่น  A,TURN_LEFT,180,60,255,-180,-4,45.0,35.0,25,1,0,0,0,0,1

    คืนลิสต์ความยาวคงที่เสมอ ถ้าดึงไม่ได้จะเป็นช่องว่าง แถวใน CSV จะได้เรียงตรงกันทุกแถว
    ต่อให้บางจังหวะติดต่อหุ่นไม่ได้ ซึ่งจำเป็นตอนเอาไปเปิดใน Excel
    """
    global _telemetry_problem
    try:
        r = session.get(TELEMETRY_URL, timeout=1)
        body = r.text.strip()
        parts = body.split(',')
        if len(parts) == TELEMETRY_FIELDS:
            _telemetry_problem = ''
            return parts
        if r.status_code == 404:
            why = 'ESP32 ไม่มีหน้า /telemetry -- ยังไม่ได้อัปสเก็ตช์ esp32_remote_controller ตัวใหม่'
        elif not body:
            why = ('ESP32 ตอบมาว่างเปล่า -- แปลว่ายังไม่เคยได้ยินอะไรจาก STM32 เลย '
                   'เช็กว่า STM32 อัปตัวใหม่แล้ว และสาย PA2 -> GPIO16 กับ GND ต่อครบ')
        else:
            why = f'ESP32 ตอบมาไม่ตรงรูปแบบ: {body[:60]}'
    except Exception as e:
        why = (f'ติดต่อ {TELEMETRY_URL} ไม่ได้ ({type(e).__name__}) -- '
               'PC ต่อ Wi-Fi ชื่อ My_Robot2 แล้วหรือยัง')

    # บอกสาเหตุครั้งเดียวต่อหนึ่งปัญหา ไม่ใช่ทุกวินาทีจนท่วมจอ
    if why != _telemetry_problem:
        print(f'⚠️  {why}')
        _telemetry_problem = why
    return [''] * TELEMETRY_FIELDS


_log_problem = ''   # ปัญหาการเขียนล่าสุดที่บอกไปแล้ว


def save_log(event, animal, code_char, tlm):
    """เขียนหนึ่งแถวลง CSV"""
    global _log_problem
    if LOG_FILE is None:
        return
    now = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    try:
        with open(LOG_FILE, mode='a', newline='', encoding='utf-8-sig') as f:
            csv.writer(f).writerow([now, event, animal, code_char] + list(tlm))
        _log_problem = ''
    except Exception as e:
        # บอกครั้งเดียวต่อหนึ่งปัญหา ของเดิมพิมพ์ทุกวินาทีจนกลบข้อความอื่นหมด
        why = f'{type(e).__name__}: {e}'
        if why != _log_problem:
            print(f'❌ เขียนล็อกไม่ได้ -- {why}')
            if isinstance(e, PermissionError):
                print('    ปิดไฟล์นี้ใน Excel ก่อน แล้วรันใหม่')
            _log_problem = why

print("🚀 --- Robot Scanner & Data Logger Online ---")
session = requests.Session()

# หากล้องทีหลังจากที่สร้างไฟล์ล็อกเสร็จแล้ว
#
# เดิมหาก่อน แล้วถ้าไม่เจอก็จบโปรแกรมทันที ไฟล์ CSV จึงไม่เคยถูกสร้างเลย
# ทั้งที่การเก็บความเร็วกับการเลี้ยวไม่ได้ใช้กล้องสักนิด ค่าพวกนั้นมาจาก ESP32 รีโมท
# ไม่มีกล้องก็แค่อ่าน QR ไม่ได้ ส่วนการเก็บล็อกยังทำงานได้เต็มที่
ESP32_IP = find_camera(sys.argv[1] if len(sys.argv) > 1 else None)
camera = CameraFeed(ESP32_IP) if ESP32_IP else None
print(f"📷 เกาะช่องภาพสดที่ {camera.stream_url}" if camera
      else "📷 ไม่มีกล้อง เก็บเฉพาะข้อมูลการเดินของหุ่น")

last_report = time.time()
last_telemetry = 0.0

save_log('START', 'SYSTEM', '', read_telemetry(session))

try:
    while True:
        # ไม่มีกล้องก็ข้ามส่วนอ่าน QR ไป การเก็บล็อกด้านล่างยังทำงานตามปกติ
        if camera:
            img = camera.latest()
            if img is None:
                time.sleep(0.01)   # เธรดกล้องยังรับเฟรมหน้าไม่เสร็จ รอสั้นๆ ไม่ต้องหมุนเปล่า
            else:
                for data, (x, y, w, h) in scan_for_qr(img):
                    if data not in ANIMAL_MAP:
                        # อ่าน QR ออกแล้วแต่ข้อความไม่ตรงกับรายชื่อสัตว์ บอกให้รู้จะได้แก้ถูก
                        print(f'⚠️  อ่าน QR ได้ว่า "{data}" ซึ่งไม่ใช่ชื่อสัตว์ที่รับ')
                        print(f'    ต้องเป็นคำใดคำหนึ่งนี้เท่านั้น: {", ".join(ANIMAL_MAP)}')
                        cv2.rectangle(img, (x, y), (x + w, y + h), (0, 165, 255), 2)
                        continue

                    print(f'✅ Found: {data}')
                    code_to_send = ANIMAL_MAP[data]
                    char_sent = code_to_send.decode()

                    send_to_robot(code_to_send)
                    print(f"📡 Sent '{char_sent}' to ESP32-CAM")

                    save_log('QR', data, char_sent, read_telemetry(session))

                    cv2.rectangle(img, (x, y), (x + w, y + h), (0, 255, 0), 2)
                    cv2.putText(img, data, (x, max(y - 10, 20)),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
                    cv2.imshow('Robot Vision', img)
                    cv2.waitKey(1)

                    # รอให้หุ่นหยุดครบ 5 วินาทีก่อนสแกนรอบใหม่
                    # ระหว่างนี้เธรดกล้องยังรับภาพต่อ แต่ทับเฟรมเก่าทิ้งไปเรื่อยๆ
                    # พอกลับมาจึงได้ภาพของตอนนี้ ไม่ใช่ภาพค้างจากก่อนหุ่นหยุด
                    time.sleep(6)

                cv2.imshow('Robot Vision', img)
        else:
            time.sleep(0.2)   # ไม่มีภาพให้ประมวลผล อย่าให้ลูปหมุนกินซีพียูเปล่า

        # เก็บสถานะหุ่นลง CSV เป็นระยะ ไม่ใช่เฉพาะตอนเจอ QR
        # แถวต่อเนื่องแบบนี้พล็อตกราฟใน Excel ได้ เห็นว่าหุ่นเลี้ยวแรงตรงไหนของสนาม
        if time.time() - last_telemetry >= TELEMETRY_EVERY_S:
            last_telemetry = time.time()
            # เขียนแถวเสมอแม้ติดต่อหุ่นไม่ได้ ช่องจะว่างแต่เวลายังเดิน
            # เดิมข้ามแถวที่ว่างไปเงียบๆ ไฟล์เลยไม่โตขึ้นเลยโดยไม่มีอะไรบอกว่าทำไม
            save_log('RUN', '', '', read_telemetry(session))

        # รายงานสถานะทุก 5 วินาที จะได้รู้ว่าระบบเดินอยู่หรือค้าง
        if time.time() - last_report >= 5:
            elapsed = time.time() - last_report
            if camera and camera.frames:
                t = read_telemetry(session)
                state = f' | {t[1]} speed {t[2]} turn {t[5]}' if t[0] else ' | ยังไม่ได้ยินเสียงหุ่น'
                # บอกเป็นเฟรมต่อวินาที ไม่ใช่จำนวนรวม ตัวเลขนี้เทียบกับรอบก่อนได้ทันที
                # ว่าที่ปรับไปช่วยจริงไหม ส่วนจำนวนรวมต้องมาหารเองทุกครั้ง
                print(f'📷 {camera.frames / elapsed:.1f} เฟรมต่อวินาที | '
                      f'ติดต่อไม่ได้ {camera.errors} ครั้ง{state}')
            elif camera:
                print(f'⏳ ยังไม่ได้ภาพจากกล้องเลย ({camera.errors} ครั้งที่ลองแล้วไม่สำเร็จ)')
                print(f'    เช็ก: ต่อ Wi-Fi My_Robot2 แล้วหรือยัง '
                      f'และเปิด {camera.capture_url} ในเบราว์เซอร์ขึ้นไหม')
                if camera.last_error:
                    print(f'    ล่าสุด: {camera.last_error}')
            else:
                t = read_telemetry(session)
                if t[0]:
                    print(f'📝 เก็บข้อมูลหุ่นอยู่ | {t[1]} speed {t[2]} turn {t[5]}')
                else:
                    print(f'⏳ ติดต่อหุ่นที่ {TELEMETRY_URL} ไม่ได้ -- ต่อ Wi-Fi My_Robot2 แล้วหรือยัง')
            if camera:
                camera.frames = camera.errors = 0
            last_report = time.time()

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
except KeyboardInterrupt:
    pass


if camera:
    camera.stop()
cv2.destroyAllWindows()
if LOG_FILE:
    print(f"📁 บันทึกไว้ที่: {LOG_FILE}")