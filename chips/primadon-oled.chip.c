// SSD1306-compatible I2C simulation for the TacPad-9 128x32 OLED strip.
// Wokwi Chips API: https://docs.wokwi.com/chips-api/getting-started
#include "wokwi-api.h"
#include <stdlib.h>
#include <string.h>

// The controller remains 128x32. Draw each pixel 2x2 so the physical strip is
// large enough to read beside the knob, while keeping its real 4:1 aspect.
enum { WIDTH = 128, HEIGHT = 32, PAGES = HEIGHT / 8, SCALE = 2 };
typedef struct {
  buffer_t framebuffer;
  uint8_t ram[WIDTH * PAGES];
  uint8_t pixels[WIDTH * HEIGHT * SCALE * SCALE * 4];
  uint8_t column, page, column_start, column_end, page_start, page_end;
  uint8_t addressing, command, argument_count, argument_index, arguments[6];
  bool first_byte, data_mode, enabled, inverted, all_on, dirty;
} oled_t;

static void paint(oled_t *oled) {
  if (!oled->dirty) return;
  for (unsigned y = 0; y < HEIGHT; ++y) {
    for (unsigned x = 0; x < WIDTH; ++x) {
      bool lit = oled->all_on || ((oled->ram[(y / 8) * WIDTH + x] >> (y % 8)) & 1);
      lit = oled->enabled && (lit != oled->inverted);
      for (unsigned dy = 0; dy < SCALE; ++dy) {
        for (unsigned dx = 0; dx < SCALE; ++dx) {
          unsigned p = ((y * SCALE + dy) * WIDTH * SCALE + x * SCALE + dx) * 4;
          oled->pixels[p] = lit ? 185 : 4;
          oled->pixels[p + 1] = lit ? 230 : 8;
          oled->pixels[p + 2] = lit ? 255 : 12;
          oled->pixels[p + 3] = 255;
        }
      }
    }
  }
  buffer_write(oled->framebuffer, 0, oled->pixels, sizeof(oled->pixels));
  oled->dirty = false;
}

static void apply_arguments(oled_t *oled) {
  switch (oled->command) {
    case 0x20: oled->addressing = oled->arguments[0] & 3; break;
    case 0x21:
      oled->column_start = oled->arguments[0] & 127;
      oled->column_end = oled->arguments[1] & 127;
      oled->column = oled->column_start;
      break;
    case 0x22:
      oled->page_start = oled->arguments[0] % PAGES;
      // Adafruit uses end page 7 even for a 32px panel: clamp to 4 pages.
      oled->page_end = oled->arguments[1] < PAGES ? oled->arguments[1] : PAGES - 1;
      oled->page = oled->page_start;
      break;
    default: break;  // Other SSD1306 init parameters do not change pixel RAM.
  }
}

static void command_byte(oled_t *oled, uint8_t value) {
  if (oled->argument_index < oled->argument_count) {
    oled->arguments[oled->argument_index++] = value;
    if (oled->argument_index == oled->argument_count) apply_arguments(oled);
    return;
  }
  oled->command = value;
  oled->argument_index = 0;
  oled->argument_count = 0;
  switch (value) {
    case 0x20: case 0x81: case 0x8D: case 0xA8: case 0xD3:
    case 0xD5: case 0xD9: case 0xDA: case 0xDB:
      oled->argument_count = 1; break;
    case 0x21: case 0x22: case 0xA3:
      oled->argument_count = 2; break;
    case 0x26: case 0x27: oled->argument_count = 6; break;
    case 0x29: case 0x2A: oled->argument_count = 5; break;
    case 0xAE: oled->enabled = false; break;
    case 0xAF: oled->enabled = true; break;
    case 0xA4: oled->all_on = false; break;
    case 0xA5: oled->all_on = true; break;
    case 0xA6: oled->inverted = false; break;
    case 0xA7: oled->inverted = true; break;
    default:
      if ((value & 0xF0) == 0xB0) oled->page = (value & 7) % PAGES;
      else if ((value & 0xF0) == 0x00) oled->column = (oled->column & 0x70) | (value & 15);
      else if ((value & 0xF0) == 0x10) oled->column = (oled->column & 15) | ((value & 7) << 4);
      break;
  }
  oled->dirty = true;
}

static bool connect(void *user_data, uint32_t address, bool read) {
  (void)address;
  oled_t *oled = user_data;
  oled->first_byte = true;
  return !read;  // SSD1306 I2C interface is write-only, address 0x3C.
}

static bool write_byte(void *user_data, uint8_t value) {
  oled_t *oled = user_data;
  if (oled->first_byte) {
    oled->first_byte = false;
    oled->data_mode = (value & 0x40) != 0;
  } else if (!oled->data_mode) {
    command_byte(oled, value);
  } else {
    oled->ram[oled->page * WIDTH + oled->column] = value;
    oled->dirty = true;
    if (oled->addressing == 1) {
      if (++oled->page > oled->page_end) {
        oled->page = oled->page_start;
        if (++oled->column > oled->column_end) oled->column = oled->column_start;
      }
    } else if (++oled->column > oled->column_end) {
      oled->column = oled->column_start;
      if (oled->addressing == 0 && ++oled->page > oled->page_end)
        oled->page = oled->page_start;
    }
  }
  return true;
}

static void disconnect(void *user_data) { paint(user_data); }

void chip_init(void) {
  oled_t *oled = calloc(1, sizeof(oled_t));
  uint32_t width, height;
  oled->framebuffer = framebuffer_init(&width, &height);
  oled->column_end = WIDTH - 1;
  oled->page_end = PAGES - 1;
  oled->dirty = true;
  pin_init("VCC", INPUT);
  pin_init("GND", INPUT);
  const i2c_config_t config = {
    .address = 0x3C,
    .scl = pin_init("SCL", INPUT_PULLUP),
    .sda = pin_init("SDA", INPUT_PULLUP),
    .connect = connect,
    .write = write_byte,
    .disconnect = disconnect,
    .user_data = oled,
  };
  i2c_init(&config);
  paint(oled);
}
