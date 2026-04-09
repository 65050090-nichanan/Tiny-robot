import cv2
import numpy as np
from pyzbar.pyzbar import decode
import requests
import socket
import time
import csv
import os
from datetime import datetime


ESP32_IP = '172.20.10.4' 
CAP_URL = f'http://{ESP32_IP}/capture'
UDP_PORT = 1234 


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
session = requests.Session()

while True:
    try:
        response = session.get(CAP_URL, timeout=0.5)
        if response.status_code == 200:
            img_array = np.frombuffer(response.content, dtype=np.uint8)
            img = cv2.imdecode(img_array, cv2.IMREAD_COLOR)

            if img is not None:
                for barcode in decode(img):
                    data = barcode.data.decode('utf-8').strip().upper()
                    
                    if data in ANIMAL_MAP:
                        # 1. แสดงผลบน Console
                        print(f"✅ Found: {data}")
                        code_to_send = ANIMAL_MAP[data]
                        char_sent = code_to_send.decode()
                        
                        # 2. ส่งคำสั่งไปที่หุ่นยนต์
                        send_to_robot(code_to_send)
                        print(f"📡 Sent '{char_sent}' to ESP32-CAM")
                        
                        # 3. บันทึกข้อมูลลง CSV (เก็บผลการทดลองใส่บทที่ 4)
                        save_log(data, char_sent)
                        
                        # 4. วาดกรอบบนจอ
                        (x, y, w, h) = barcode.rect
                        cv2.rectangle(img, (x, y), (x + w, y + h), (0, 255, 0), 2)
                        cv2.putText(img, data, (x, y - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)
                        
                        cv2.imshow('Robot Vision', img)
                        cv2.waitKey(1)
                        
                        # หยุดรอรถทำงาน 6 วินาที
                        time.sleep(6)
                
                cv2.imshow('Robot Vision', img)
    except:
        pass

    if cv2.waitKey(1) & 0xFF == ord('q'): 
        break

cv2.destroyAllWindows()