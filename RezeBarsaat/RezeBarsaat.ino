
// Keep video_data.h in the same folder as this file.

#include <Arduino.h>
#include <Wire.h>
#include <U8x8lib.h>
#include "video_data.h"


// Most 1.3" modules are SH1106; most 0.96" modules are SSD1306.
U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE);
// U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE);
// -----------------------------------------------------------------------------

// Previous frame in the OLED's page layout (8 pages x 128 bytes, bit0 = top).
// Each new frame is XOR-decoded into this buffer, which is sent as-is.
uint8_t frameBuf[1024];

uint32_t videoPos = 0;  // read position inside videoData

// Stream per frame: [skip][n][n literal bytes] ... then [0][0] = end of frame.
void decodeFrame() {
  uint16_t out = 0;
  for (;;) {
    uint8_t skip = pgm_read_byte(&videoData[videoPos++]);
    uint8_t n    = pgm_read_byte(&videoData[videoPos++]);
    if (skip == 0 && n == 0) break;
    out += skip;
    while (n--) {
      uint8_t v = pgm_read_byte(&videoData[videoPos++]);
      if (out < 1024) frameBuf[out++] ^= v;
    }
  }
}

void showFrame() {
  for (uint8_t page = 0; page < 8; page++) {
    u8x8.drawTile(0, page, 16, &frameBuf[page * 128]);
  }
}

void setup() {
  u8x8.begin();
  u8x8.setBusClock(400000);   // fast I2C so a frame fits inside the frame time
  u8x8.setContrast(255);
  memset(frameBuf, 0, sizeof(frameBuf));
}

void loop() {
  static uint16_t frame = 0;
  static unsigned long last = 0;
  const unsigned long interval = 1000UL / VIDEO_FPS;

  if (millis() - last >= interval) {
    last += interval;                       // no drift
    if (millis() - last > 500) last = millis();  // resync after a stall

    decodeFrame();
    showFrame();

    if (++frame >= VIDEO_FRAMES) {          // loop: first frame is relative to blank
      frame = 0;
      videoPos = 0;
      memset(frameBuf, 0, sizeof(frameBuf));
    }
  }
}
