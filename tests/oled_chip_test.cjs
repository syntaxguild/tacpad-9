// Exercise the compiled Wokwi chip's real I2C callbacks and framebuffer.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');

(async () => {
  const definition = JSON.parse(fs.readFileSync(path.join(__dirname, '../chips/primadon-oled.chip.json')));
  const { width, height } = definition.display;
  assert.equal(width / height, 4);
  assert.equal(width, 128 * 2);
  assert.equal(height, 32 * 2);
  const memory = new WebAssembly.Memory({ initial: 256, maximum: 256 });
  let bus, pixels;
  const pins = new Map();
  const { instance } = await WebAssembly.instantiate(
    fs.readFileSync(path.join(__dirname, '../chips/primadon-oled.chip.wasm')),
    { env: {
      memory,
      framebufferInit(w, h) {
        const view = new DataView(memory.buffer);
        view.setUint32(w, width, true);
        view.setUint32(h, height, true);
        return 1;
      },
      pinInit(ptr) {
        const bytes = new Uint8Array(memory.buffer);
        const end = bytes.indexOf(0, ptr);
        const name = Buffer.from(bytes.subarray(ptr, end)).toString();
        if (!pins.has(name)) pins.set(name, pins.size + 1);
        return pins.get(name);
      },
      i2cInit(ptr) { bus = Array.from(new Uint32Array(memory.buffer, ptr, 8)); return 1; },
      bufferWrite(handle, offset, ptr, length) {
        assert.equal(handle, 1);
        assert.equal(offset, 0);
        assert.equal(length, width * height * 4);
        pixels = Uint8Array.from(new Uint8Array(memory.buffer, ptr, length));
      },
    } },
  );
  instance.exports.chipInit();
  assert.equal(bus[1], 0x3C);
  assert.equal(bus[2], pins.get('SCL'));
  assert.equal(bus[3], pins.get('SDA'));
  const table = instance.exports.__indirect_function_table;
  const connect = table.get(bus[4]), write = table.get(bus[6]), disconnect = table.get(bus[7]);
  const transfer = (bytes) => {
    assert.equal(connect(bus[0], 0x3C, 0), 1);
    for (const byte of bytes) assert.equal(write(bus[0], byte), 1);
    disconnect(bus[0]);
  };
  const lit = (x, y) => pixels[(y * width + x) * 4] !== 4;
  const litCount = () => {
    let count = 0;
    for (let y = 0; y < height; ++y)
      for (let x = 0; x < width; ++x) if (lit(x, y)) ++count;
    return count;
  };
  assert.equal(connect(bus[0], 0x3C, 1), 0);
  // SSD1306 128x32 initialization used by Adafruit, including ignored settings.
  transfer([0, 0xAE, 0xD5, 0x80, 0xA8, 31, 0xD3, 0, 0x40, 0x8D, 0x14,
    0x20, 0, 0xA1, 0xC8, 0xDA, 2, 0x81, 0x8F, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0x2E, 0xAF]);
  assert.equal(litCount(), 0);
  transfer([0, 0x21, 0, 127, 0x22, 0, 7]);
  const frame = new Uint8Array(512);
  frame[0] = 1;
  frame[511] = 0x80;
  for (let i = 0; i < frame.length; i += 31) transfer([0x40, ...frame.slice(i, i + 31)]);
  assert.equal(litCount(), 8);
  assert(lit(0, 0) && lit(1, 1) && lit(254, 62) && lit(255, 63));
  transfer([0, 0xA7]);
  assert.equal(litCount(), width * height - 8);
  transfer([0, 0xAE]);
  assert.equal(litCount(), 0);
  transfer([0, 0xAF, 0xA6, 0xA5]);
  assert.equal(litCount(), width * height);
  console.log('PASS: compiled OLED chip ACK, SSD1306 init, 512-byte frame, 2x scale, 4:1 aspect, inversion and display power.');
})().catch(error => { console.error(error); process.exitCode = 1; });
