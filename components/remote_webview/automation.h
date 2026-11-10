#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/version.h"
#include "remote_webview.h"

#include <functional>

// ESPHome 2025.11 changed Action::play() to take its arguments by const reference.
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2025, 11, 0)
#define REMOTE_WEBVIEW_PLAY_ARGS const Ts &...x
#else
#define REMOTE_WEBVIEW_PLAY_ARGS Ts... x
#endif

namespace esphome {
namespace remote_webview {

template<typename... Ts> class ShowOverlayAction : public Action<Ts...> {
 public:
  explicit ShowOverlayAction(RemoteWebView *parent) : parent_(parent) {}

  void set_rect(int x, int y, int w, int h) {
    x_ = x; y_ = y; w_ = w; h_ = h;
  }
  void set_writer(std::function<void(display::Display &, Ts...)> &&writer) { writer_ = std::move(writer); }

  void play(REMOTE_WEBVIEW_PLAY_ARGS) override {
    // The writer runs before show_overlay() returns, so the trigger arguments stay valid.
    this->parent_->show_overlay(x_, y_, w_, h_, [&](display::Display &it) {
      if (this->writer_) this->writer_(it, x...);
    });
  }

 protected:
  RemoteWebView *parent_;
  int x_{-1}, y_{-1}, w_{0}, h_{0};
  std::function<void(display::Display &, Ts...)> writer_;
};

template<typename... Ts> class HideOverlayAction : public Action<Ts...> {
 public:
  explicit HideOverlayAction(RemoteWebView *parent) : parent_(parent) {}

  void play(REMOTE_WEBVIEW_PLAY_ARGS) override { this->parent_->hide_overlay(); }

 protected:
  RemoteWebView *parent_;
};

// Automation things below
template<typename... Ts> class TriggerOnFrameUpdateAction : public Action<Ts...> {
 public:
  explicit TriggerOnFrameUpdateAction(RemoteWebView *ea) : ea_(ea) {}

  void play(Ts... x) override {
    this->ea_->trigger_on_frame_update();
  }

 protected:
  RemoteWebView *ea_;
};

// On Frame Update Trigger
class OnFrameUpdateTrigger : public Trigger<> {
 public:
  explicit OnFrameUpdateTrigger(RemoteWebView *parent) {
    parent->add_on_frame_update_callback([this]() { this->trigger(); });
  }
};

}  // namespace remote_webview
}  // namespace esphome
