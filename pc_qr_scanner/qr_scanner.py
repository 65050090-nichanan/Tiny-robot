import cv2
import numpy as np
from pyzbar.pyzbar import decode
import requests
import socket
import sys
import time
import csv
import os
from datetime import datetime


# ESP32-CAM เกาะ Wi-Fi ชื่อ My_Robot ที่ ESP32 รีโมทปล่อยออกมา จึงได้ IP ในวง 192.168.4.x
# เครื่อง PC ต้องต่อ Wi-Fi My_Robot ด้วย ถึงจะคุยกับกล้องได้
#
# IP ของกล้องเปลี่ยนได้ทุกครั้งที่เปิดเครื่อง ขึ้นกับว่าใครมาเกาะ Wi-Fi ก่อน
# สคริปต์จึงไล่หาเองว่ากล้องอยู่ IP ไหน แทนที่จะให้คนมานั่งแก้ตัวเลขทุกครั้ง
# ถ้ารู้ IP อยู่แล้วก็ระบุได้:  py pc_qr_scanner/qr_scanner.py 192.168.4.3

UDP_PORT = 1234


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

    print('❌ หากล้องไม่เจอ')
    print('   เช็ก 3 อย่างนี้')
    print('   1. PC ต่อ Wi-Fi ชื่อ My_Robot แล้วหรือยัง (รหัส password1234)')
    print('   2. ESP32-CAM เปิดอยู่และเกาะ Wi-Fi ได้แล้วหรือยัง ดูจาก Serial Monitor')
    print('   3. เปิด http://192.168.4.1 ดูบรรทัด CAM: ว่าขึ้น IP อะไร')
    sys.exit(1)


ESP32_IP = find_camera(sys.argv[1] if len(sys.argv) > 1 else None)
CAP_URL = f'http://{ESP32_IP}/capture'



LOG_FILE = os.path.join(os.path.expanduser('~'), 'Desktop', 'robot_mission_log.csv')


try:
    with open(LOG_FILE, mode='w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerow(['Timestamp', 'Animal_Detected', 'Status', 'Sent_Code'])
        
        writer.writerow([datetime.now().strftime('%Y-%m-%d %H:%M:%S'), 'SYSTEM', 'START_LOGGING', 'INIT'])
    print(f"✅ บังคับสร้างไฟล์สำเร็จ! อยู่ที่หน้า Desktop: {LOG_FILE}")
except Exception as e:
    print(f"❌ ไม่สามารถสร้างไฟล์ที่ Desktop ได้: {e}")

# แผนผังรหัสสัตว์
ANIMAL_MAP = {
    "DOG": b'D', "CAT": b'C', "BIRD": b'B', "LION": b'L', "TIGER": b'T'
}

def send_to_robot(code):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.sendto(code, (ESP32_IP, UDP_PORT))
    except: pass

def save_log(animal_name, code_char):
    """ฟังก์ชันบันทึกข้อมูลลงไฟล์ CSV"""
    now = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    try:
        with open(LOG_FILE, mode='a', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            writer.writerow([now, animal_name, 'STOP_5_SEC', code_char])
        print(f"📁 Data Logged to CSV: {animal_name} at {now}")
    except Exception as e:
        print(f"❌ Logging Error: {e}")

print("🚀 --- Robot Scanner & Data Logger Online ---")
print(f"📷 กล้องที่ใช้: {CAP_URL}")
session = requests.Session()

frames = 0          # จำนวนภาพที่ดึงมาได้สำเร็จ
errors = 0          # จำนวนครั้งที่ติดต่อกล้องไม่ได้
last_report = time.time()
last_error_msg = ''

while True:
    try:
        # เดิมตั้ง timeout ไว้ 0.5 วินาทีซึ่งสั้นเกินไปสำหรับ ESP32-CAM
        # ถ้ากล้องตอบช้ากว่านั้นทุกรอบจะ timeout แล้ววนเปล่าโดยไม่มีอะไรขึ้นเลย
        response = session.get(CAP_URL, timeout=3)
        if response.status_code != 200:
            raise RuntimeError(f'กล้องตอบ HTTP {response.status_code}')

        img_array = np.frombuffer(response.content, dtype=np.uint8)
        img = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
        if img is None:
            raise RuntimeError('ถอดรหัสภาพจากกล้องไม่ได้')

        frames += 1

        for barcode in decode(img):
            data = barcode.data.decode('utf-8').strip().upper()
            (x, y, w, h) = barcode.rect

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

            save_log(data, char_sent)

            cv2.rectangle(img, (x, y), (x + w, y + h), (0, 255, 0), 2)
            cv2.putText(img, data, (x, y - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)
            cv2.imshow('Robot Vision', img)
            cv2.waitKey(1)

            time.sleep(6)   # รอให้หุ่นหยุดครบ 5 วินาทีก่อนสแกนรอบใหม่

        cv2.imshow('Robot Vision', img)

    except Exception as e:
        errors += 1
        msg = f'{type(e).__name__}: {e}'
        if msg != last_error_msg:      # พิมพ์เฉพาะตอนที่ปัญหาเปลี่ยนไป ไม่ให้ท่วมจอ
            print(f'❌ ติดต่อกล้องไม่ได้ -- {msg}')
            last_error_msg = msg
        time.sleep(0.3)

    # รายงานสถานะทุก 5 วินาที จะได้รู้ว่าระบบเดินอยู่หรือค้าง
    if time.time() - last_report >= 5:
        if frames:
            print(f'📷 ดึงภาพมาแล้ว {frames} เฟรม | ติดต่อไม่ได้ {errors} ครั้ง')
        else:
            print(f'⏳ ยังไม่ได้ภาพจากกล้องเลย ({errors} ครั้งที่ลองแล้วไม่สำเร็จ)')
            print(f'    เช็ก: ต่อ Wi-Fi My_Robot แล้วหรือยัง และเปิด {CAP_URL} ในเบราว์เซอร์ขึ้นไหม')
        frames = errors = 0
        last_report = time.time()

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cv2.destroyAllWindows()
