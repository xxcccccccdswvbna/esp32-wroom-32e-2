#include "ble_gateway.h"
#include "esphome/core/log.h"
#include <cctype>

namespace esphome {
namespace ble_gateway {

static const char *TAG = "ble_gateway";
BLEGateway *BLEGateway::instance_ = nullptr;

void BLEGateway::setup() {
    instance_ = this;
    ESP_LOGI(TAG, "ready");
    esp_ble_gap_register_callback(gap_callback);
}

void BLEGateway::loop() {
    const uint32_t now = millis();
    if (busy_ && now - t1_ < 500) return;
    busy_ = false;
    if (advertising_ && now - t0_ >= 200) {
        esp_ble_gap_stop_advertising();
        advertising_ = false;
        t1_ = now;
        busy_ = true;
        if (!tx_queue_.empty()) {
            t2_ = now + 800;
        }
    }
    if (!advertising_ && !busy_ && !tx_queue_.empty() && now >= t2_) {
        next_();
    }
}

void BLEGateway::send_hex(const std::string &hex) {
    size_t s = 0;
    while (s < hex.length()) {
        size_t p = hex.find('|', s);
        if (p == std::string::npos) p = hex.length();
        auto pkt = hex.substr(s, p - s);
        if (!pkt.empty()) {
            auto data = hex_to_bytes(pkt);
            tx_queue_.push(data);
        }
        s = p + 1;
    }
    if (!advertising_ && !busy_) next_();
}

void BLEGateway::next_() {
    if (tx_queue_.empty()) return;
    current_packet_ = std::move(tx_queue_.front());
    tx_queue_.pop();
    raw_("");
}

void BLEGateway::raw_(const std::string &) {
    if (current_packet_.size() < 5) return;
    esp_ble_gap_config_adv_data_raw(current_packet_.data(), current_packet_.size());
    esp_ble_adv_params_t p{};
    p.adv_int_min = 0x40;
    p.adv_int_max = 0x80;
    p.adv_type = ADV_TYPE_NONCONN_IND;
    p.channel_map = ADV_CHNL_ALL;
    esp_ble_gap_start_advertising(&p);
    t0_ = millis();
    advertising_ = true;
}

std::vector<uint8_t> BLEGateway::hex_to_bytes(const std::string &h) {
    std::vector<uint8_t> d;
    d.reserve(h.size() / 2);
    char pair[3]{};
    int n = 0;
    for (char c : h) {
        if (!std::isxdigit((unsigned char)c)) continue;
        pair[n++] = c;
        if (n == 2) {
            d.push_back((uint8_t)strtol(pair, nullptr, 16));
            n = 0;
        }
    }
    return d;
}

void BLEGateway::gap_callback(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    // 可选：处理GAP事件
}

}  // namespace ble_gateway
}  // namespace esphome
