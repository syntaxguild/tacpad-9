# Tacpad 9 / Model Nine — Primadon

แมโครแพด 9 ปุ่ม พร้อมปุ่มหมุน OLED และเกม XO ออกแบบเป็นเคสพิมพ์ 3D พร้อมโค้ด Blender และเฟิร์มแวร์ USB HID

![Tacpad 9](assets/tacpad9-wide.png)

รุ่น **0.2.0 เป็น digital prototype**: build เฟิร์มแวร์และตรวจโมเดลด้วย Blender แล้ว แต่ยังไม่ได้พิมพ์ประกอบหรือเสียบ USB ทดสอบบนบอร์ดจริง ภาพทั้งหมดเป็นภาพเรนเดอร์จากโมเดลจริงของโปรเจกต์

## เริ่มต้น

1. ดู [อะไหล่ งบ และขา GPIO](hardware.md) ใช้ ESP32-S3-Zero-style FH4R2 แบบไม่บัดกรี pin header
2. ติดตั้ง PlatformIO แล้วรัน `pio run -e tacpad_s3`
3. กด BOOT ค้างตอนเสียบสาย USB data ครั้งแรก แล้วรัน `pio run -e tacpad_s3 -t upload`
4. ต่อปุ่มแต่ละปุ่มระหว่าง GPIO กับ GND; จอและ encoder ใช้ 3.3 V
5. ทดสอบวงจรบนโต๊ะก่อนพิมพ์เคสเต็ม ตาม [ขั้นตอนประกอบ](assembly.md)

## งบรวมไม่เกิน 500 บาท

แผนอะไหล่ **402.20 บาท + สำรองค่าส่ง 95 บาท = 497.20 บาท** ใช้เครื่องพิมพ์ วัสดุพิมพ์ และเครื่องมือที่มี พิมพ์ keycap และ knob เอง โลหะในภาพเป็นแนวทางตกแต่ง รุ่นในร้านและค่าส่งจริงต้องตรวจที่ checkout ก่อนซื้อ ถ้าเกิน 500 บาทให้รวมร้านหรือใช้สาย USB ที่มี ห้ามตีความเงินสำรองว่าเป็นค่าส่งที่ยืนยันแล้ว

ลิงก์ร้านจริง ราคาอ้างอิง และรายการที่เป็นงบเผื่ออยู่ใน [BOM](hardware.md#฿500-purchasing-plan)

## ปุ่มและการใช้งาน

ปุ่ม 1–9: Copy, Paste, Build, Blender, Undo, Redo, Play/Pause, Mute, Terminal หมุน encoder เพื่อปรับเสียงบนคอม กดค้าง 1.5 วินาทีสลับเข้าเกม XO ปุ่ม Blender ส่ง Ctrl+Alt+Shift+B ต้องตั้ง shortcut เปิดแอปในคอมเอง

ตัวจำลอง Wokwi เดิมยังใช้ได้ผ่าน `pio run -e esp32dev` แต่เป็นการจำลอง UI/เกมและข้อความ Serial ไม่ใช่การทดสอบ native USB ของ S3 ดู [เฟิร์มแวร์และการควบคุม](firmware.md)

## โมเดล ภาพ และไฟล์ดาวน์โหลด

เปิด `mechanical/model.py` จากไฟล์ใน Scripting tab ของ Blender 4.x โดยมี `reference.json` อยู่ข้างกัน หรือรัน:

```sh
blender -b --python-exit-code 1 --python mechanical/validate.py
blender -b --python-exit-code 1 --python mechanical/render.py
blender -b --python-exit-code 1 --python mechanical/service.py
```

ไฟล์ที่สร้างอยู่ใน `mechanical/exports/` ส่วนไฟล์พร้อมดาวน์โหลดอยู่ใน [Release v0.2.0](https://github.com/syntaxguild/tacpad-9/releases/tag/v0.2.0): Blender, STL แยกชิ้น, OBJ, เฟิร์มแวร์ และ checksum

![ชิ้นส่วนแยกประกอบ](assets/tacpad9-exploded.png)

ก่อนพิมพ์ทั้งชุด ให้วัดโมดูลจริง โดยเฉพาะความสูงแกน encoder และขนาด OLED แล้วพิมพ์ coupon กับปุ่มเดียวเพื่อลอง fit ระยะเผื่อในไฟล์เป็นค่าตั้งต้น ไม่ใช่ผลทดสอบจากชิ้นงานพิมพ์แล้ว

[สถานะการตรวจสอบ](validation.md) · [README หลัก](../README.md) · [ประวัติรุ่น](../CHANGELOG.md)
