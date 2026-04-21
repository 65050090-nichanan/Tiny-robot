import cv2
import numpy as np
from pyzbar.pyzbar import decode
import requests
import socket
import time
import csv
import os
from datetime import datetime

# --- ตั้งค่าการเชื่อมต่อ ---
ESP32_IP = '172.20.10.4' 
CAP_URL = f'http://{ESP32_IP}/capture'
UDP_PORT = 1234 
LOG_FILE = os.path.join(os.path.expanduser('~'), 'Desktop', 'robot_mission_log.csv')

# --- [จุดสำคัญ] ตัวแปรต้องอยู่นอกลูป While True เท่านั้น ---
processed_qrs = {}  # เก็บ { "ชื่อสัตว์": เวลาที่สแกนล่าสุด }
cooldown_time = 20  # เพิ่มเป็น 20 วินาที เพื่อให้รถวิ่งพ้นป้ายแน่นอน

ANIMAL_MAP = {"DOG": b'S', "CAT": b'S', "BIRD": b'S', "LION": b'S', "TIGER": b'S'}

def send_to_robot(code):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.sendto(code, (ESP32_IP, UDP_PORT))
        sock.close()
    except: pass

print("🚀 --- Robot Scanner Online (Anti-Duplicate Mode) ---")
session = requests.Session()

while True:
    try:
        response = session.get(CAP_URL, timeout=1)
        if response.status_code == 200:
            img_array = np.frombuffer(response.content, dtype=np.uint8)
            img = cv2.imdecode(img_array, cv2.IMREAD_COLOR)

            if img is not None:
                current_time = time.time()
                
                # 1. ตรวจจับ QR Code ในภาพ
                detected_barcodes = decode(img)
                
                for barcode in detected_barcodes:
                    data = barcode.data.decode('utf-8').strip().upper()
                    
                    if data in ANIMAL_MAP:
                        # 2. เช็คเงื่อนไข: ถ้าไม่เคยเจอ หรือ เจอแล้วแต่ผ่านไปนานกว่า cooldown_time
                        last_time = processed_qrs.get(data, 0)
                        
                        if (current_time - last_time) > cooldown_time:
                            # --- เริ่มกระบวนการทำงานเมื่อเจอตัวใหม่ ---
                            print(f"✅ [NEW SCAN] : {data}")
                            
                            # ส่งสัญญาณ 'S' ให้ STM32
                            send_to_robot(ANIMAL_MAP[data])
                            
                            # บันทึกข้อมูลลง CSV
                            now_str = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                            with open(LOG_FILE, mode='a', newline='', encoding='utf-8') as f:
                                csv.writer(f).writerow([now_str, data, 'STOP_5_SEC', 'S'])
                            
                            # อัปเดตเวลาที่สแกนตัวนี้ล่าสุด
                            processed_qrs[data] = current_time

                            # วาดกรอบและหยุดรอ
                            (x, y, w, h) = barcode.rect
                            cv2.rectangle(img, (x, y), (x + w, y + h), (0, 255, 0), 5)
                            cv2.imshow('Robot Vision', img)
                            cv2.waitKey(500)
                            
                            print(f"⏳ Waiting 6s for mission completion...")
                            time.sleep(6) 
                            
                            # ล้างภาพค้างในคิวทิ้งทั้งหมด
                            session.close()
                            session = requests.Session()
                            break # ออกจากลูป For เพื่อรับภาพใหม่ทันที
                        else:
                            # ถ้ายังอยู่ในช่วง Cooldown ให้ข้ามไป (ไม่ต้อง Print อะไร)
                            pass
                
                cv2.imshow('Robot Vision', img)
    except Exception as e:
        pass

    if cv2.waitKey(1) & 0xFF == ord('q'): break

cv2.destroyAllWindows()