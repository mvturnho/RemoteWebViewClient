[![Stand With Ukraine](https://raw.githubusercontent.com/vshymanskyy/StandWithUkraine/main/banner-direct-single.svg)](https://stand-with-ukraine.pp.ua)

# Remote WebView Client

![Guition-ESP32-S3-4848S040 running Remote WebView](/images/image-001.jpg)

[Demo video](https://youtu.be/rD2aYUUrv5o)

This is a client that connects to [Remote WebView Server](https://github.com/strange-v/RemoteWebViewServer) — a headless browser that renders target web pages (e.g., Home Assistant dashboards) and streams them as image tiles over WebSocket to lightweight clients (ESP32 displays).

## ESPHome component

The latest version of the client is implemented as an ESPHome external component, which greatly simplifies installation and configuration for end users. It leverages the display and touchscreen components to render images and handle touch input.

### Configuration example (Guition-ESP32-S3-4848S040)

```yaml
esphome:
  name: esp32-4848s040-t1
  friendly_name: ESP32-4848S040-T1
  platformio_options:
    board_build.flash_mode: dio

esp32:
  board: esp32-s3-devkitc-1
  variant: esp32s3
  flash_size: 16MB
  framework:
    type: esp-idf
    sdkconfig_options:
      CONFIG_ESP32S3_DEFAULT_CPU_FREQ_240: "y"
      CONFIG_ESP32S3_DATA_CACHE_64KB: "y"
      CONFIG_ESP32S3_DATA_CACHE_LINE_64B: "y"
      CONFIG_SPIRAM_FETCH_INSTRUCTIONS: y
      CONFIG_SPIRAM_RODATA: y
    components:
      - name: "espressif/esp_websocket_client"
        ref: 1.5.0
      - name: "bitbank2/jpegdec"
        source: https://github.com/strange-v/jpegdec-esphome

psram:
  mode: octal
  speed: 80MHz

external_components:
  - source: github://strange-v/RemoteWebViewClient@main
    refresh: 0s
    components: [ remote_webview ]

logger:
  hardware_uart: UART0

api:
  encryption:
    key: "XXXXXXXXX"

ota:
  - platform: esphome
    password: "XXXXXXXXX"

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

captive_portal:
    
spi:
  clk_pin: GPIO48
  mosi_pin: GPIO47

i2c:
  - id: bus_a
    sda: GPIO19
    scl: GPIO45

display:
  - platform: st7701s
    show_test_card: False
    update_interval: never
    auto_clear_enabled: False
    spi_mode: MODE3
    data_rate: 2MHz
    color_order: RGB
    invert_colors: False
    dimensions:
      width: 480
      height: 480
    cs_pin: 39
    de_pin: 18
    hsync_pin: 16
    vsync_pin: 17
    pclk_pin: 21
    pclk_frequency: 12MHz
    pclk_inverted: False
    hsync_pulse_width: 8
    hsync_front_porch: 10
    hsync_back_porch: 20
    vsync_pulse_width: 8
    vsync_front_porch: 10
    vsync_back_porch: 10
    init_sequence:
      - 1
      - [0xFF, 0x77, 0x01, 0x00, 0x00, 0x10]
      - [0xCD, 0x00]
    data_pins:
      red:
        - GPIO11
        - GPIO12
        - GPIO13
        - GPIO14
        - GPIO0
      green:
        - GPIO8
        - GPIO20
        - GPIO3
        - GPIO46
        - GPIO9
        - GPIO10
      blue:
        - GPIO4
        - GPIO5
        - GPIO6
        - GPIO7
        - GPIO15

touchscreen:
  platform: gt911
  transform:
    mirror_x: false
    mirror_y: false
  i2c_id: bus_a

output:
  - platform: ledc
    pin: GPIO38
    id: backlight_pwm

light:
  - platform: monochromatic
    output: backlight_pwm
    name: "Display Backlight"
    id: back_light
    restore_mode: ALWAYS_ON

remote_webview:
  id: rwv
  server: 172.16.0.252:8081
  url: http://172.16.0.252:8123/dashboard-mobile/0  # set url: "self-test" to initiate the self-test
  full_frame_tile_count: 1
  max_bytes_per_msg: 61440
  jpeg_quality: 85
  # ESP32-P4 only; hardware decoder options are ignored on other targets.
  hw_decode: true
  hw_decode_timeout_ms: 200
  tile_size: 48  # ESP32-P4: every tile width is then decoded in hardware (see hw_decode below)
  # Temporary P4 timeout diagnostics; leave disabled during normal use.
  # hw_decode_diagnostics: true
  # hw_decode_driver_debug: true
  
  on_frame_update:
    - logger.log: "The display just received a frame update!"
  current_url_sensor:
    name: "Remote Display Current URL"

text:
  - platform: template
    id: rwv_url
    name: "URL"
    optimistic: true
    restore_value: false
    mode: TEXT
    min_length: 1
    set_action:
      - lambda: |-
          if (!id(rwv).open_url(std::string(x.c_str()))) {
            id(rwv).set_url(std::string(x.c_str()));
            ESP_LOGI("remote_webview", "URL queued (not connected): %s", x.c_str());
          }

```

### Supported Parameters

| YAML key                | Type      | Required | Example                          | Description |
|-------------------------|-----------|:--------:|----------------------------------|-------------|
| `display_id`            | id        | ❌       | `panel`                           | Display to draw on. Optional, if only one display is defined in the YAML.|
| `touchscreen_id`        | id        | ❌       | `touch`                           | Touch input source. Optional, if only one touchscreen is defined in the YAML. |
| `server`                | string    | ✅       | `172.16.0.252:8081`              | WebSocket server address. Must be `hostname_or_IP:port`. |
| `url`                   | string    | ✅       | `http://…/dashboard`             | Page to open on connect. |
| `device_id`             | string    | ❌       | `"my-device"` or auto (`esp32-<mac>`) | Identifier used by the server. If not set, the component derives `esp32-<mac>` from the chip MAC and still sends it. |
| `tile_size`             | int       | ❌       | `32`                              | Tile edge size in pixels. Helps the server choose tile packing; it’s best to keep it a multiple of 16. On ESP32-P4 with `hw_decode`, use `48`. |
| `full_frame_tile_count` | int       | ❌       | `4`                               | Number of tiles the server should use for full-frame updates. |
| `full_frame_area_threshold` | float | ❌       | `0.50`                            | Area delta (fraction of screen) above which the server should send a full frame. |
| `full_frame_every`      | int       | ❌       | `50`                              | Force a full-frame update every N frames (0 disables). |
| `every_nth_frame`       | int       | ❌       | `1`                               | Frame-rate divider. Server should send only every Nth frame. |
| `min_frame_interval`    | int (ms)  | ❌       | `80`                              | Minimum time between frames on the wire, in milliseconds. |
| `jpeg_quality`          | int       | ❌       | `85`                              | JPEG quality hint for the server’s encoder. |
| `max_bytes_per_msg`     | int (B)   | ❌       | `14336` or `61440`                | Upper bound for a single WS binary message. |
| `big_endian`            | bool      | ❌       | `true` or `false`                 | RGB565 pixel byte order for software JPEG output. Defaults to `false` on ESP32-P4 and `true` on other targets. P4 hardware fallback always stays little-endian to match the hardware decoder. |
| `hw_decode`             | bool      | ❌       | `true`                            | ESP32-P4 only. Use the hardware JPEG decoder; defaults to `true`. |
| `hw_decode_timeout_ms`  | int (ms)  | ❌       | `200`                             | ESP32-P4 only. Hardware JPEG decoder timeout. |
| `hw_decode_diagnostics` | bool      | ❌       | `false`                           | Log firmware/chip versions and detailed metadata plus CRC32 when a P4 hardware decode times out. |
| `hw_decode_driver_debug` | bool     | ❌       | `false`                           | Enable `CONFIG_JPEG_ENABLE_DEBUG_LOG` to show whether DMA2D starts a JPEG transaction. Diagnostic only; significantly slows JPEG processing. |
| `rotation`              | int       | ❌       | 0, 90, 180, 270                   | Enables software rotation for both the display and touchscreen. |
| `scale`                 | float     | ❌       | `1.5`                             | Page zoom (0.5–4). The page is laid out at the display size divided by the scale and rendered at that device scale factor, so text and controls get bigger. For sharp output the server needs `MAX_DEVICE_SCALE_FACTOR` at least this value. |
| `on_frame_update`       | action    | ❌       | `- logger.log: "The display just received a frame update!"`  | Action that gets triggered each time the display updates (maximum of 1 trigger per second) |
| `stats_interval`        | time      | ❌       | `10s`                             | Logs performance stats (fps, per-frame rx/queue/decode/draw time, queue depth, drops, touch-to-frame latency) at INFO level every interval. Disabled by default. |
| `current_url_sensor`    | text_sensor | ❌      | `name: "Current URL"`            | Exposes the URL currently loaded in the server's headless browser as an ESPHome Text Sensor. Supports on_value automations. |


## Recommendations

- **full_frame_tile_count** set to 1 is the most efficient way to do a full-screen update; use it if your network/device memory allows it.
- **every_nth_frame** must be 1 if you don’t want to miss changes (though increasing it may reduce server load). I recommend keeping it set to 1.
- **wifi power_save_mode** — for maximum responsiveness, set `power_save_mode: none` in the `wifi:` section. ESPHome's default power saving lets the radio sleep between packets, which lowers power draw but adds latency: in testing on an ESP32-S3 it added ~30 ms to every update, cut throughput from ~1.6 MB/s to ~330 KB/s and made a full-screen change take 154 ms instead of 88 ms. Keep the default if power draw or Wi-Fi coexistence matters more than speed. The self-test shows the effect ("Wi-Fi: … ms per update").
- **min_frame_interval** caps how often the server sends frames. It protects the device from frames arriving faster than it can decode them (e.g. while scrolling), so it should be at least the device's decode time for a full-screen frame. Run the self-test (`url: "self-test"`): it measures that time and recommends a value.
- **max_bytes_per_msg** should be larger than your maximum tile size (full-frame or partial).
- **jpeg_quality** — lower values encode faster and reduce bandwidth (but increase artifacts). Start at **85**, drop toward **70–75** if you need speed.
- **big_endian** — defaults to **false** on ESP32-P4 so hardware and software-fallback tiles use the same little-endian MIPI-DSI representation. Other targets retain the previous **true** default.
- **hw_decode** — ESP32-P4 only; defaults to **true**. Set to `false` to use the software JPEG decoder. Only tiles whose width is a whole number of the hardware decoder's 2D-DMA output blocks (48 px for the server's 4:2:0 JPEGs, 64 px for 4:2:2, 32 px for 4:4:4) are decoded in hardware. Other tiles are decoded in software, because in hardware a narrower tile times out after `hw_decode_timeout_ms` and a tile with a partial last block shows grey blocks. Set `tile_size: 48` so every tile qualifies; with the default `32`, only tiles 96, 192, 288, … px wide do.
- **hw_decode_timeout_ms** — ESP32-P4 only; defaults to **200**. Timeout passed to the hardware JPEG decoder engine.
- **Hardware JPEG diagnostics** — enable `hw_decode_diagnostics` for timeout metadata. Add `hw_decode_driver_debug` only for a short diagnostic run and use DEBUG logging; a `jpeg.decoder: 2ddma transaction callbacks start` line proves that DMA2D picked up the transaction. The IDF driver warns that this debug mode makes JPEG processing substantially slower.
- **Display ownership** — `remote_webview` draws from its own task, so nothing else may draw to the same display: use `update_interval: never`, no `lambda`/`pages`, `auto_clear_enabled: false`, `show_test_card: false`, and don't call `component.update` on it. To draw something on the device itself, use an [overlay](#overlay-popup). Prefer a touchscreen on a separate bus (e.g. I2C) rather than sharing the display's SPI bus.
- **Red tile / red screen** — this indicates a tile payload exceeded `max_bytes_per_msg`. Increase `max_bytes_per_msg` or reduce tile size/JPEG quality so each tile fits.

## Overlay (popup)

An overlay is a popup drawn by the device itself over the web view, without a round trip to the server. Use it for feedback that must appear right away, such as "listening" for a voice assistant.

```yaml
font:
  - file: "gfonts://Roboto"
    id: font_popup
    size: 28

image:
  - file: mdi:microphone
    id: icon_mic
    resize: 48x48
    type: BINARY
    transparency: chroma_key

voice_assistant:
  # ...
  on_listening:
    - remote_webview.show_overlay:
        id: rwv
        width: 320
        height: 96
        y: 40            # omit x and/or y to center the overlay on that axis
        lambda: |-
          it.fill(Color(30, 30, 30));
          it.filled_circle(48, 48, 32, Color(3, 169, 244));
          it.image(24, 24, id(icon_mic), Color(255, 255, 255));
          it.print(100, 48, id(font_popup), Color(255, 255, 255), TextAlign::CENTER_LEFT, "Listening...");
  on_end:
    - remote_webview.hide_overlay: rwv
```

- `remote_webview.show_overlay` draws `lambda` on a `width` × `height` canvas with the usual ESPHome display calls (`it.print`, `it.image`, `it.filled_rectangle`, …). The lambda also gets the trigger's arguments, so `on_stt_end` can show the recognized text with `x.c_str()`. Coordinates are display pixels in the page's orientation (`rotation` is applied for you, `scale` is not).
- Pixels the lambda doesn't draw stay transparent and keep what was on screen when the overlay appeared. Only fully drawn or untouched pixels exist: there is no partial transparency.
- While an overlay is shown, tiles under its rectangle are not drawn. `remote_webview.hide_overlay` (or showing a new overlay that doesn't cover the old one) requests a full frame from the server to repaint the area.
- `show_overlay` replaces the current overlay. From a lambda: `id(rwv).show_overlay(x, y, w, h, [](display::Display &it) { ... })` and `id(rwv).hide_overlay()`, with `-1` for a centered axis.
- The canvas is rendered in the main loop and drawn by the `remote_webview` task. It uses `width × height × 3` bytes of PSRAM until it is drawn.

## On-screen keyboard

The device does not provide a native on-screen keyboard.

You can enable and use a server-side keyboard in [Remote WebView Server](https://github.com/strange-v/RemoteWebViewServer#on-screen-keyboard).
You can also use [Chrome DevTools](https://github.com/strange-v/RemoteWebViewServer#accessing-the-servers-tab-with-chrome-devtools) when input is needed.
