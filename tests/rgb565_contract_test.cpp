#include "rgb565_contract.h"

#include <array>
#include <cstdint>

using esphome::remote_webview::rgb565::P4_HARDWARE_JPEG_BIG_ENDIAN;
using esphome::remote_webview::rgb565::P4_SOFTWARE_JPEG_BIG_ENDIAN;
using esphome::remote_webview::rgb565::pixel_bytes;
using esphome::remote_webview::rgb565::PixelBytes;

int main() {
  constexpr std::array<uint16_t, 6> pixels{0x0000, 0xFFFF, 0xF800,
                                           0x07E0, 0x001F, 0x7BEF};

  for (const uint16_t pixel : pixels) {
    const PixelBytes hardware = pixel_bytes(pixel, P4_HARDWARE_JPEG_BIG_ENDIAN);
    const PixelBytes software = pixel_bytes(pixel, P4_SOFTWARE_JPEG_BIG_ENDIAN);
    if (!(hardware == software))
      return 1;
  }

  if (!(pixel_bytes(0xF800, P4_HARDWARE_JPEG_BIG_ENDIAN) ==
        PixelBytes{0x00, 0xF8}))
    return 2;
  if (!(pixel_bytes(0x07E0, P4_HARDWARE_JPEG_BIG_ENDIAN) ==
        PixelBytes{0xE0, 0x07}))
    return 3;
  if (!(pixel_bytes(0x001F, P4_HARDWARE_JPEG_BIG_ENDIAN) ==
        PixelBytes{0x1F, 0x00}))
    return 4;
  return 0;
}
