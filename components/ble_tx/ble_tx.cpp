#include "ble_tx.h"
#include "esphome/core/log.h"
#include "esp_gap_ble_api.h"
#include <cctype>

namespace esphome { namespace ble_tx {
static const char *TAG = "homlux_ble_tx";
void BLETx::setup() { ESP_LOGI(TAG, "sender ready; GAP callback intentionally not replaced (BLE scan/proxy preserved)"); }
void BLETx::send_hex(const std::string &hex) {
  const size_t split = hex.find('|');
  Job j;
  j.command = split == std::string::npos ? hex : hex.substr(0, split);
  if (split != std::string::npos) j.end = hex.substr(split + 1);
  if (j.command.empty()) { ESP_LOGW(TAG, "empty command rejected"); return; }
  if (queue_.size() >= 24) { ESP_LOGW(TAG, "queue full: command rejected"); return; }
  queue_.push_back(std::move(j));
  ESP_LOGI(TAG, "queued (%u waiting)", (unsigned)queue_.size());
  pump_();
}
void BLETx::loop() {
  const uint32_t now = millis();
  if ((state_ == State::CONFIGURING || state_ == State::STOPPING || state_ == State::ADVERTISING || state_ == State::GAP) && (int32_t)(now - deadline_) >= 0) {
    if (state_ == State::CONFIGURING) start_();
    else if (state_ == State::ADVERTISING) stop_();
    else if (state_ == State::STOPPING) next_phase_();
    else if (state_ == State::GAP) {
      if (retries_ > 0) { state_ = State::IDLE; configure_(); }
      else if (end_pending_) { state_ = State::IDLE; configure_(); }
      else { state_ = State::IDLE; current_ = {}; pump_(); }
    }
  } else if (state_ == State::IDLE) pump_();
}
void BLETx::pump_() {
  if (state_ != State::IDLE || queue_.empty()) return;
  current_ = std::move(queue_.front()); queue_.pop_front();
  phase_ = 0; retries_ = 0; end_pending_ = false;
  configure_();
}
void BLETx::configure_() {
  const std::string &packet = phase_ == 0 ? current_.command : current_.end;
  if (packet.empty()) {
    if (phase_ == 0 && !current_.end.empty()) { phase_ = 1; end_pending_ = true; configure_(); }
    else { end_pending_ = false; current_ = {}; state_ = State::GAP; deadline_ = millis() + 200; }
    return;
  }
  data_ = decode_(packet);
  if (data_.size() < 5 || data_.size() > 31) { fail_("raw packet length invalid (must be 5..31)"); return; }
  const esp_err_t err = esp_ble_gap_config_adv_data_raw(data_.data(), data_.size());
  if (err != ESP_OK) { fail_("config raw advertising data rejected"); return; }
  // ESP-IDF GAP data configuration is asynchronous. Avoid replacing ESPHome's GAP callback;
  // instead allow a short settling window before starting the broadcast.
  state_ = State::CONFIGURING; deadline_ = millis() + 80;
}
void BLETx::start_() {
  esp_ble_adv_params_t p{};
  p.adv_int_min = 0x40; p.adv_int_max = 0x80;
  p.adv_type = ADV_TYPE_NONCONN_IND; p.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
  p.channel_map = ADV_CHNL_ALL; p.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
  const esp_err_t err = esp_ble_gap_start_advertising(&p);
  if (err != ESP_OK) { fail_("start advertising rejected"); return; }
  state_ = State::ADVERTISING;
  deadline_ = millis() + (phase_ == 0 ? 1200U : 600U);
  retries_ = 0;
  ESP_LOGI(TAG, "phase %u broadcast %u ms, packet=%u bytes", phase_, phase_ == 0 ? 1200 : 600, (unsigned)data_.size());
}
void BLETx::stop_() {
  const esp_err_t err = esp_ble_gap_stop_advertising();
  if (err != ESP_OK) { fail_("stop advertising rejected"); return; }
  state_ = State::STOPPING; deadline_ = millis() + 80;
}
void BLETx::next_phase_() {
  state_ = State::IDLE;
  if (phase_ == 0 && !current_.end.empty()) {
    phase_ = 1; end_pending_ = true; deadline_ = millis() + 150; state_ = State::GAP; return;
  }
  end_pending_ = false; deadline_ = millis() + 200; state_ = State::GAP;
}
void BLETx::fail_(const char *why) {
  ESP_LOGW(TAG, "%s; phase=%u retry=%u", why, phase_, retries_);
  if (retries_ < 2) { ++retries_; state_ = State::GAP; deadline_ = millis() + (250U * retries_); return; }
  retries_ = 0;
  // Give up on this packet rather than wedging the queue; if the control failed,
  // skip its END frame too. The next queued operation will still be processed.
  current_ = {}; end_pending_ = false; state_ = State::GAP; deadline_ = millis() + 200;
}
std::vector<uint8_t> BLETx::decode_(const std::string &hex) {
  std::vector<uint8_t> out; int high = -1;
  for (char c : hex) {
    if (!std::isxdigit(static_cast<unsigned char>(c))) continue;
    const int v = std::isdigit(static_cast<unsigned char>(c)) ? c-'0' : std::tolower(c)-'a'+10;
    if (high < 0) high = v; else { out.push_back((uint8_t)((high << 4) | v)); high = -1; }
  }
  if (high >= 0) ESP_LOGW(TAG, "odd hex nibble ignored");
  return out;
}
} }
