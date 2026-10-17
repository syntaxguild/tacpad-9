# PRIMADON | TacPad-9

ESP32 9-Key Macro Pad + Tic-Tac-Toe พร้อม Minimax AI ที่ไม่มีวันแพ้เมื่อผู้เล่นเริ่มก่อน
เฟิร์มแวร์เป็น Arduino C++ ใช้ PlatformIO และ Wokwi ใน VS Code

รุ่น Industrial OLED Edition ใช้ **custom OLED 128×32 แนวนอน อัตราส่วน 4:1** อยู่บนซ้าย และ Knob อยู่บนขวาในระดับเดียวกัน
ตัวจำลองรับคำสั่ง SSD1306 จริงผ่าน I2C และขยายพิกเซล 2× สำหรับแสดงบนผังให้เห็นชัด (พื้นที่แสดงผล 256×64; ความละเอียดเฟิร์มแวร์ยังเป็น 128×32)
ปุ่มทั้งเก้าจัดเป็นตาราง 3×3 ระยะห่างแนวนอนและแนวตั้งเท่ากัน 140 หน่วย โดยบอร์ด ESP32 อยู่กึ่งกลางใต้แผงปุ่ม
สายสัญญาณเดินตามช่องว่างระหว่างแถวและขอบด้านนอกของแผง

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

- เริ่มต้นใน **Macro Pad Mode**: บรรทัดบนแสดง `PRIMADON` กับ `MACRO`; ด้านล่างแสดงคีย์ที่กดล่าสุด, Volume เป็นเปอร์เซ็นต์ และแถบระดับเสียง
- กดปุ่มเพื่อจำลอง shortcut ผ่าน Serial พร้อมไฮไลต์ชื่อคีย์บน OLED
- หมุน encoder ตามเข็มเพิ่ม Volume ครั้งละ 5%; ทวนเข็มลด Volume (0–100%)
- กดสวิตช์ encoder ค้าง **1.5 วินาที** เพื่อสลับระหว่าง Macro กับเกม ค้างต่อจะไม่สลับซ้ำจนกว่าจะปล่อยแล้วกดใหม่
- ใน Wokwi คลิก encoder เพื่อเลือก แล้วกด **Space ค้าง** สำหรับ SW; ใช้ลูกศรเพื่อหมุน
- เกม: ผู้เล่นเป็น **X**, AI เป็น **O** บนจอขาวดำ; กระดาน 24×24 พิกเซลอยู่ฝั่งซ้าย คะแนน X / O / D และผลเกมอยู่ฝั่งขวา
- ปุ่มจริง 1–9 ตรงกับช่องบนจอจากซ้ายไปขวา บนลงล่าง
- คลิกปุ่มหรือกดเลข `1`–`9` ขณะ diagram มี focus
- เมื่อจบเกม ให้ปล่อยปุ่ม แล้วกดปุ่มใดก็ได้เพื่อเริ่มรอบใหม่ การกดเริ่มรอบใหม่ยังไม่ลง X
- การหมุน encoder ในโหมดเกมไม่มีผลต่อกระดาน
- คะแนนสะสมอยู่ตลอด session แม้เริ่มรอบใหม่หรือสลับโหมด; รีเซ็ตเมื่อรีสตาร์ต ESP32 (แต่ละช่องสะสมสูงสุด 65,535)

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
| OLED SDA | GPIO21 / สีน้ำเงิน |
| OLED SCL | GPIO22 / สีเหลือง |
| OLED VCC | 3V3 / สีแดง |
| OLED GND | GND / สีดำ |
| Encoder DT | GPIO34 + pull-up 10kΩ ไป 3V3 |
| Encoder CLK | GPIO35 + pull-up 10kΩ ไป 3V3 |
| Encoder SW | GPIO32 / INPUT_PULLUP |
| Encoder VCC / GND | 3V3 / GND |
| ปุ่มทั้ง 9 | GPIO ตามตาราง / INPUT_PULLUP; อีก contact ลง GND |

- OLED ใช้ `chip-primadon-oled` กับ `chips/primadon-oled.chip.wasm`; เฟิร์มแวร์ใช้ framebuffer 128×32, I2C address 0x3C และความเร็ว 400kHz
- `board-ssd1306` มาตรฐานของ Wokwi เป็นจอ 128×64 และไม่รองรับการเปลี่ยนทรงด้วย `attrs.height`; จึงเปลี่ยนเป็น custom chip ที่มีจอแถบยาวจริง
- เริ่มต้นด้วย `Wire.begin(21, 22)` ก่อนเรียก `display.begin(SSD1306_SWITCHCAPVCC, 0x3C)` โดยตรง จึงไม่มี address-only probe มาขัดขวางการส่งคำสั่ง init
- Encoder ใช้ชื่อ part `wokwi-ky-040`; ตรวจด้วย Wokwi CLI แล้ว `wokwi-rotary-encoder` เป็น unknown part type
- โปรไฟล์ `esp32dev` ใช้ **Adafruit SSD1306** กับ **Adafruit GFX Library** ทั้ง simulator และฮาร์ดแวร์จริง
- GPIO34/35 ไม่มี pull-up ภายใน จึงใช้ `INPUT` กับตัวต้านทานภายนอก ส่วน SW และปุ่มทั้ง 9 ใช้ `INPUT_PULLUP`
- สีสาย: GND ดำ, VCC แดง, SDA น้ำเงิน, SCL เหลือง, สัญญาณปุ่ม/encoder เขียวหรือม่วง
- หากจัดสรร framebuffer ไม่สำเร็จ เฟิร์มแวร์แจ้งทาง Serial และยังรับคำสั่งจากปุ่ม/Knob ผ่าน Serial ได้ ทั้งนี้ `display.begin()` ของไลบรารีไม่ได้ยืนยันว่าอุปกรณ์ตอบ ACK บนบัสจริง

สำหรับฮาร์ดแวร์จริง ต่อ OLED I2C ตามตาราง แล้ว build/upload ด้วย:

```sh
pio run
pio run -t upload
pio device monitor -b 115200
```

`wokwi.toml` ยังคงใช้เส้นทาง `esp32dev` เดิม จึงเริ่ม simulator หลัง build ได้ทันที
ไฟล์ `.wasm` ของจอรวมอยู่ใน repository แล้ว จึงไม่ต้องติดตั้ง compiler ของ custom chip เพื่อรัน
หลังอัปเดตจากรุ่นจอสี่เหลี่ยม ให้ Stop แล้ว Start simulator ใหม่เพื่อโหลดโมเดลจอใหม่

หากแก้ C source ของโมเดลจอ ให้ build ใหม่ด้วย [Wokwi CLI](https://docs.wokwi.com/guides/custom-chips-to-wasm):

```sh
cd chips
wokwi-cli chip compile primadon-oled.chip.c -o primadon-oled.chip.wasm
```

## โครงสร้างและการทดสอบ

```text
TacPad-9/
  platformio.ini
  diagram.json
  wokwi.toml
  src/main.cpp          # input, UI, animation, modes, serial
  include/tictactoe.h   # game rules + Minimax / alpha-beta pruning
  chips/primadon-oled.chip.c     # SSD1306-compatible I2C display model
  chips/primadon-oled.chip.json  # 4:1 display presentation
  chips/primadon-oled.chip.wasm # ready-to-run Wokwi model
  tests/minimax_test.cpp
  tests/oled_smoke_test.cpp
  tests/host/          # Arduino / I2C / OLED doubles for host tests
  tests/oled_chip_test.cjs      # tests the compiled WASM model
  .vscode/extensions.json
```

`loop()` ใช้ `millis()` สำหรับ debounce 20ms, highlight, long-press, เอฟเฟกต์สลับโหมด และป้ายผลเกม ไม่มี `delay()` ใน application loop
Encoder อ่าน quadrature ผ่าน interrupt; งาน I2C และ Minimax อยู่ใน main loop
OLED ส่ง framebuffer เฉพาะเมื่อ UI เปลี่ยน โดยเว้นอย่างน้อย 33ms หลังจบเฟรมก่อนหน้า ไม่มีการรอค้างสำหรับแอนิเมชันหรือการกดปุ่ม
Minimax ค้นหาจนครบเกมบนกระดานเก้าช่อง ใช้ depth scoring เพื่อเลือกชนะเร็วและชะลอการแพ้ พร้อม alpha-beta pruning

ทดสอบ AI ด้วย C++ compiler บนเครื่อง (เช่น MinGW / GCC / Clang):

```sh
g++ -std=c++11 -O2 -Wall -Wextra -pedantic -Iinclude tests/minimax_test.cpp -o tests/minimax_test.exe
./tests/minimax_test.exe
```

ตัวทดสอบเทียบทุกตำแหน่งที่ AI เดินได้กับ full-tree oracle และไล่ทุกลำดับการเดินของผู้เล่นกับ AI ตัวจริง โดย assert ว่าผู้เล่นชนะเป็นศูนย์
การทดสอบนี้ตรวจ game logic; การตรวจจอ/การกดปุ่มจริงให้ทำใน Wokwi ตามขั้นตอนเริ่มใช้งาน

ทดสอบการเริ่มต้น OLED และ UI จาก `src/main.cpp` โดยตรงด้วยฮาร์ดแวร์จำลองบน host:

```sh
g++ -std=c++11 -O2 -Wall -Wextra -pedantic -Itests/host -Iinclude tests/oled_smoke_test.cpp -o .pio/oled_smoke_test.exe
./.pio/oled_smoke_test.exe
```

ตรวจว่า I2C ใช้ขา 21/22 ก่อน init, ไม่มี probe gate เดิม, ข้อความและรูปทั้งหมดอยู่ในขอบ 128×32 รวมถึงคะแนนสูงสุด, ปุ่มทั้ง 9, ขอบเขต Volume, long-press 1.5 วินาที, AI ตอบทันที และการเริ่มรอบใหม่
ตัวทดสอบใช้ doubles ของอุปกรณ์ จึงไม่ยืนยันการแสดงภาพหรือ ACK ใน Wokwi จริง

ทดสอบไฟล์ WASM ของโมเดลจอจริงด้วย Node.js (ไม่ใช้ npm packages):

```sh
node tests/oled_chip_test.cjs
```

ผลตรวจในสภาพแวดล้อมนี้:

- `pio run`: SUCCESS; RAM 22,600 bytes, Flash 304,621 bytes
- OLED host smoke test: PASS ทั้ง initialization, ขอบเขต UI 128×32 และการควบคุม
- C++ test: AI ตัดสินใจตรงกับ oracle ทั้ง 2,097 ตำแหน่ง
- ทุกลำดับผู้เล่น 521 แบบ: X ชนะ 0, O ชนะ 368, เสมอ 153
- Wokwi CLI 0.28.1 `lint`: exit code 0 ไม่มี error/warning; มี info ว่า `board-esp32-devkit-c-v4` เป็นชนิดบอร์ดที่ไม่มีเอกสารอ้างอิงใน registry ของ linter
- รัน firmware ใน Wokwi VS Code แล้ว จอแถบยาวแสดง PRIMADON / MACRO, KEYS 1-9 READY และแถบระดับเสียงได้จริง พร้อมทดสอบหมุน Knob ให้ Volume เปลี่ยนจาก 50% เป็น 55%
- WASM chip test ตรวจ ACK ที่ 0x3C, การ init ของ Adafruit, framebuffer 512 bytes, อัตราส่วน 4:1, การขยายภาพ 2× และคำสั่ง invert/on/off

เอกสารอ้างอิง: [Wokwi OLED](https://docs.wokwi.com/parts/board-ssd1306), [Rotary encoder](https://docs.wokwi.com/parts/wokwi-ky-040), [Wokwi project config](https://docs.wokwi.com/vscode/project-config), [ESP32 GPIO](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)
