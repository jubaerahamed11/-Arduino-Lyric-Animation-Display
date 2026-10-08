#include <Arduino.h>
#include <Wire.h>
#include <U8x8lib.h>
#include "video_data.h"

// If yours is SSD1306, replace the constructor with:
// U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE);

// U8x8 has no internal frame buffer, which saves 1024 bytes of SRAM
// compared with the U8g2 "_F_" full-buffer constructors.
U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE);

// 128x64 image in the OLED's own page layout: 8 pages x 128 bytes, each byte is a
// column of 8 pixels (bit0 = top). This is the only large buffer; it holds the
// previous frame for delta decoding and is sent to the display as-is.
uint8_t frameBuf[1024];

uint16_t videoPos = 0;  // read position inside videoData (frames are stored back to back)

// Stream per frame: [skip][n][n literal bytes] ... then [0][0] = end of frame.
// Each literal byte is XORed into the previous frame at the current position.
void decodeFrame() {
  uint16_t out = 0;
  for (;;) {
    uint8_t skip = pgm_read_byte(&videoData[videoPos++]);
    uint8_t n = pgm_read_byte(&videoData[videoPos++]);
    if (skip == 0 && n == 0) break;  // end-of-frame marker

    out += skip;
    while (n--) {
      uint8_t v = pgm_read_byte(&videoData[videoPos++]);
      if (out < 1024) frameBuf[out++] ^= v;
    }
  }
}

void showFrame() {
  for (uint8_t page = 0; page < 8; page++) {
    u8x8.drawTile(0, page, 16, &frameBuf[page * 128]);  // 16 tiles x 8 bytes = one page
  }
}

void setup() {
  u8x8.begin();
  u8x8.setBusClock(400000);  // faster I2C so a frame fits comfortably in the ~83 ms frame time
  u8x8.setContrast(255);
  memset(frameBuf, 0, sizeof(frameBuf));
}

void loop() {
  static uint16_t frame = 0;
  static unsigned long last = 0;
  const unsigned long interval = 1000UL / VIDEO_FPS;

  if (millis() - last >= interval) {
    last = millis();

    decodeFrame();
    showFrame();

    frame++;
    if (frame >= VIDEO_FRAMES) {
      frame = 0;
      videoPos = 0;  // restart: the first frame is stored relative to a blank screen
      memset(frameBuf, 0, sizeof(frameBuf));
    }
  }
}
