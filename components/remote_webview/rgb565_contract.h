#pragma once

#include <cstdint>

namespace esphome::remote_webview::rgb565 {

struct PixelBytes {
  uint8_t first;
  uint8_t second;

  constexpr bool operator==(const PixelBytes &other) const {
    return this->first == other.first && this->second == other.second;
  }
};

// The ESP32-P4 JPEG decoder's BGR element order emits little-endian RGB565.
// Software fallback uses the same representation so both paths can use the
// MIPI-DSI fast path without a byte swap or an intermediate buffer.
inline constexpr bool P4_HARDWARE_JPEG_BIG_ENDIAN = false;
inline constexpr bool P4_SOFTWARE_JPEG_BIG_ENDIAN = false;

constexpr PixelBytes pixel_bytes(uint16_t pixel, bool big_endian) {
  return big_endian ? PixelBytes{static_cast<uint8_t>(pixel >> 8),
                                 static_cast<uint8_t>(pixel)}
                    : PixelBytes{static_cast<uint8_t>(pixel),
                                 static_cast<uint8_t>(pixel >> 8)};
}

static_assert(P4_HARDWARE_JPEG_BIG_ENDIAN == P4_SOFTWARE_JPEG_BIG_ENDIAN);
static_assert(pixel_bytes(0xF800, P4_HARDWARE_JPEG_BIG_ENDIAN) ==
              PixelBytes{0x00, 0xF8});

} // namespace esphome::remote_webview::rgb565
