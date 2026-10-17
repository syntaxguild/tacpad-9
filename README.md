# PRIMADON | TacPad-9

ESP32 9-Key Macro Pad + Tic-Tac-Toe พร้อม Minimax AI ที่ไม่มีวันแพ้เมื่อผู้เล่นเริ่มก่อน
เฟิร์มแวร์เป็น Arduino C++ ใช้ PlatformIO และ Wokwi ใน VS Code

## เริ่มใช้งาน

1. เปิดโฟลเดอร์นี้ใน VS Code และติดตั้งส่วนขยายที่แนะนำ: **PlatformIO IDE** และ **Wokwi Simulator**
2. เปิด PlatformIO Terminal แล้วรัน:

   ```sh
   pio run
   ```

3. กด `F1` → **Wokwi: Start Simulator** หาก Wokwi ขอเปิดใช้งาน license ให้ทำตามขั้นตอนของส่วนขยาย
4. ดูข้อความ Macro / Volume / การเดินเกมใน Serial Monitor ของ Wokwi (115200 baud)

`wokwi.toml` ชี้ไปที่ `.pio/build/esp32dev/firmware.bin` และ `firmware.elf` อยู่แล้ว

หาก PowerShell หา `pio` ไม่เจอ แต่ติดตั้งผ่าน PlatformIO IDE ไว้แล้ว:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
```

## การควบคุม

- เริ่มต้นใน **Macro Pad Mode**: กดปุ่มเพื่อจำลอง shortcut ผ่าน Serial และไฮไลต์คีย์บนจอ
- หมุน encoder ตามเข็มเพิ่ม Volume ครั้งละ 5%; ทวนเข็มลด Volume (0–100%)
- กดสวิตช์ encoder ค้าง **1.5 วินาที** เพื่อสลับระหว่าง Macro กับเกม ค้างต่อจะไม่สลับซ้ำจนกว่าจะปล่อยแล้วกดใหม่
- ใน Wokwi คลิก encoder เพื่อเลือก แล้วกด **Space ค้าง** สำหรับ SW; ใช้ลูกศรเพื่อหมุน
- เกม: ผู้เล่นเป็น **X สีฟ้า**, AI เป็น **O สีแดง**; ปุ่มจริง 1–9 ตรงกับช่องบนจอจากซ้ายไปขวา บนลงล่าง
- คลิกปุ่มหรือกดเลข `1`–`9` ขณะ diagram มี focus
- เมื่อจบเกม ให้ปล่อยปุ่ม แล้วกดปุ่มใดก็ได้เพื่อเริ่มรอบใหม่ การกดเริ่มรอบใหม่ยังไม่ลง X
- การหมุน encoder ในโหมดเกมไม่มีผลต่อกระดาน

| ตำแหน่ง | ปุ่ม / GPIO | Macro | ข้อความจำลอง |
|---|---|---|---|
| บนซ้าย | Btn1 / 13 | COPY | CTRL+C |
| บนกลาง | Btn2 / 12 | PASTE | CTRL+V |
| บนขวา | Btn3 / 14 | BUILD | CTRL+SHIFT+B |
| กลางซ้าย | Btn4 / 27 | BLENDER | LAUNCH BLENDER |
| กลาง | Btn5 / 26 | UNDO | CTRL+Z |
| กลางขวา | Btn6 / 25 | REDO | CTRL+Y |
| ล่างซ้าย | Btn7 / 33 | PLAY | MEDIA PLAY/PAUSE |
| ล่างกลาง | Btn8 / 15 | MUTE | MEDIA MUTE |
| ล่างขวา | Btn9 / 16 | TERMINAL | CTRL + backtick |

Macro และ Volume เป็นข้อความจำลองตามโจทย์ ยังไม่ได้ส่ง USB/BLE HID หรือปรับเสียงบนคอมพิวเตอร์จริง

## วงจรและจอ

| อุปกรณ์ / ขา | ESP32 |
|---|---|
| TFT MOSI | GPIO23 |
| TFT SCK | GPIO18 |
| TFT CS | GPIO5 |
| TFT D/C | GPIO2 |
| TFT RST | GPIO4 |
| TFT VCC / LED | 3V3 |
| TFT GND | GND |
| Encoder DT | GPIO34 + pull-up 10kΩ ไป 3V3 |
| Encoder CLK | GPIO35 + pull-up 10kΩ ไป 3V3 |
| Encoder SW | GPIO32 / INPUT_PULLUP |
| Encoder VCC / GND | 3V3 / GND |
| ปุ่มทั้ง 9 | GPIO ตามตาราง / INPUT_PULLUP; อีก contact ลง GND |

- Wokwi ใช้ชื่อ part ที่เอกสารทางการรองรับคือ `wokwi-ili9341` และ `wokwi-ky-040` สำหรับจอและ rotary encoder
- จอ ILI9341 มีขนาด 240×320; UI ใช้เฉพาะพื้นที่ 240×240 ด้านบน ส่วน 80 พิกเซลด้านล่างเป็นพื้นหลัง
- โปรไฟล์เริ่มต้น `esp32dev` ใช้ **Adafruit ILI9341** เพื่อให้ driver ตรงกับจอใน simulator
- โปรไฟล์ `st7789` ใช้ **Adafruit ST7735 and ST7789 Library** ซึ่งเป็นชื่อไลบรารี ST7789 จริงใน PlatformIO Registry
- GPIO34/35 ไม่มี pull-up ภายใน จึงใช้ `INPUT` กับตัวต้านทานภายนอก ส่วน SW และปุ่มทั้ง 9 ใช้ `INPUT_PULLUP`
- TFT MISO ไม่ต้องต่อ เพราะโปรเจกต์ส่งข้อมูลไปที่จออย่างเดียว

สำหรับจอ ST7789 240×240 จริง ต่อ SPI ตามตาราง แล้ว build/upload ด้วย:

```sh
pio run -e st7789
pio run -e st7789 -t upload
pio device monitor -b 115200
```

โปรไฟล์ `st7789` ใช้กับจอจริง ส่วน `wokwi.toml` ยังคงใช้ `esp32dev` เพื่อให้ตรงกับวงจรจำลอง

## โครงสร้างและการทดสอบ

```text
TacPad-9/
  platformio.ini
  diagram.json
  wokwi.toml
  src/main.cpp          # input, UI, animation, modes, serial
  include/tictactoe.h   # game rules + Minimax / alpha-beta pruning
  tests/minimax_test.cpp
  .vscode/extensions.json
```

`loop()` ใช้ `millis()` สำหรับ debounce 20ms, highlight, long-press, เอฟเฟกต์สลับโหมด และป้ายผลเกม ไม่มี `delay()` ใน application loop
Encoder อ่าน quadrature ผ่าน interrupt; งาน SPI และ Minimax อยู่ใน main loop
Minimax ค้นหาจนครบเกมบนกระดานเก้าช่อง ใช้ depth scoring เพื่อเลือกชนะเร็วและชะลอการแพ้ พร้อม alpha-beta pruning

ทดสอบ AI ด้วย C++ compiler บนเครื่อง (เช่น MinGW / GCC / Clang):

```sh
g++ -std=c++11 -O2 -Wall -Wextra -pedantic -Iinclude tests/minimax_test.cpp -o tests/minimax_test.exe
./tests/minimax_test.exe
```

ตัวทดสอบเทียบทุกตำแหน่งที่ AI เดินได้กับ full-tree oracle และไล่ทุกลำดับการเดินของผู้เล่นกับ AI ตัวจริง โดย assert ว่าผู้เล่นชนะเป็นศูนย์
การทดสอบนี้ตรวจ game logic; การตรวจจอ/การกดปุ่มจริงให้ทำใน Wokwi ตามขั้นตอนเริ่มใช้งาน

ผลตรวจในสภาพแวดล้อมนี้:

- `pio run`: SUCCESS; RAM 22,352 bytes, Flash 290,785 bytes
- `pio run -e st7789`: SUCCESS; RAM 22,344 bytes, Flash 291,633 bytes
- C++ test: AI ตัดสินใจตรงกับ oracle ทั้ง 2,097 ตำแหน่ง
- ทุกลำดับผู้เล่น 521 แบบ: X ชนะ 0, O ชนะ 368, เสมอ 153
- Wokwi CLI 0.28.1 `lint`: exit code 0 ไม่มี error/warning; มี info ว่า `board-esp32-devkit-c-v4` เป็นชนิดบอร์ดที่ไม่มีเอกสารอ้างอิงใน registry ของ linter
- ยังไม่ได้รันจำลองจอและอินพุตจริงใน VS Code

เอกสารอ้างอิง: [Wokwi TFT](https://docs.wokwi.com/parts/wokwi-ili9341), [Rotary encoder](https://docs.wokwi.com/parts/wokwi-ky-040), [Wokwi project config](https://docs.wokwi.com/vscode/project-config), [ESP32 GPIO](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)
