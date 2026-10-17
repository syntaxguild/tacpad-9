# Hardware & purchasing

The reference controller is an **ESP32-S3-Zero-style FH4R2 board**, 18 × 23.5 mm, without tall headers. Its native USB port carries keyboard, consumer control and USB CDC. The classic ESP32 in the Wokwi diagram remains a simulation target only.

The [Waveshare documentation](https://docs.waveshare.com/ESP32-S3-Zero) is the reference for the board family, flash configuration, USB and pinout. Budget marketplace boards are compatible-board candidates, not a claim of genuine Waveshare manufacture. Compare their printed pin labels and outline before ordering. Select **S3 / FH4R2**, not C3, S2 or an ESP32 UART-only board.

## ฿500 purchasing plan

Prices checked on 17 October 2026; this is a purchasing plan, not a paid order or delivered-cost quote. The [machine-readable BOM](bom.json) separates listed prices from allowances.

| Item | Qty | Subtotal THB | Source / basis |
|---|---:|---:|---|
| S3-Zero-style FH4R2 | 1 | 207.20 | [QIUWU listing](https://www.lazada.co.th/products/pdp-i5850646815.html), observed through Lazada category; verify variant |
| 0.91-inch SSD1306 128 × 32 | 1 | 53.00 | [MCUCity M2541-B91](https://www.mcucity.com/product/2541/oled-0-91-inch-128x32-i2c-blue-module-ssd1306) |
| KY-040 encoder | 1 | 32.00 | [ModuleMore](https://www.modulemore.com/product/220/ky-040-โมดูล-knob-เอ็นโค้ดเดอร์-วัดความเร็วรอบ-และสวิตซ์-rotary-encoder-module-ky-040), regular price |
| Outemu MX switches | 9 | 45.00 | [Shopee](https://shopee.co.th/ปุ่มสวิตช์-Outemu-Switch-Mechanical-Keyboard-Red-Blue-Brown-Black-สินค้าพร้อมส่งในไทย-i.160376915.44412084514), ฿5 starting variant |
| M2 screws: 4 × 8 mm, 4 × 6 mm, 2 × 10 mm | set | 20.00 | Allowance |
| Thin wire / insulation | set | 10.00 | Allowance |
| USB-C **data** cable | 1 | 30.00 | Allowance |
| Optional protective film / adhesive | set | 5.00 | Allowance |
| **Parts plan** | | **402.20** | |
| Delivery reserve | | **95.00** | Combined checkout allowance, not verified shipping |
| **Planned total** | | **497.20** | **฿2.80 headroom** |

Use the existing printer, filament, soldering tools and solder. Print the enclosure, nine caps and knob; the aluminium appearance in the renders is a finish study. Rubber feet are optional existing stock and are not needed to operate the device. No PCB fabrication, hot-swap sockets, diodes or heat-set inserts are required for nine separately wired inputs.

**Checkout gate:** sum the selected variants, required pack sizes and all delivery fees before paying. Do not order if the total exceeds ฿500. Shipping across several sellers can consume the reserve; consolidate sellers or reuse an existing data cable. The plan does not guarantee an arbitrary postcode's delivery price.

## Wiring: native USB hardware

| Function | S3 GPIO | Connection |
|---|---|---|
| Keys 1–9, reading left to right | 1, 2, 4, 5, 6, 7, 8, 9, 10 | Switch between its GPIO and common GND |
| Encoder CLK / DT / SW | 11 / 12 / 13 | Active low, pull-ups enabled |
| OLED SDA / SCL | 17 / 18 | I2C, address 0x3C |
| OLED VCC / encoder + | 3V3 | Use the board's regulated 3.3 V rail |
| All ground connections | GND | Common return |
| Native USB D− / D+ | 19 / 20 | Already connected to the board's USB-C connector; leave free |

Do not connect 5 V pull-ups to the GPIO inputs. A KY-040 is a passive switch network with pull-ups; power those pull-ups from 3V3, or use the bare contacts and the S3 internal pull-ups. The vendor's Arduino 5 V example is not the wiring for this S3 build. Leave GPIO0, GPIO3 and onboard RGB GPIO21 unused. The authoritative assignments are [board_config.h](../include/board_config.h).

Hand-wire each key separately; the 3 × 3 describes its physical layout. Daisy-chain only the ground contacts. Direct-solder short flexible wires to the board edges and remove tall module headers where necessary. Check continuity and shorts before connecting USB.

## Mechanical selection gate

[reference.json](../mechanical/reference.json) records the selected board family and provisional module envelopes. Controller outline comes from the manufacturer; OLED PCB, component heights, encoder shaft and MX fit still need measurement of the delivered parts. Generic seller names do not specify a mechanical tolerance.

In particular, ModuleMore / [its linked KY-040 sheet](https://www.handsontec.com/dataspecs/module/Rotary%20Encoder.pdf) gives an overall 30 × 18 × 30 mm envelope, rather than a dimensioned shaft drawing. The model's compact-module PCB and shaft positions are fit assumptions. Do not interpret the rendered reference electronics as an exact supplier STEP model. Measure the module and update the reference/model before printing the whole enclosure. The clamp carrier and USB opening target the stated board layout; a different S3 DevKit will require changes.

See [assembly and printing](assembly.md) for the fit coupon and order of work.
