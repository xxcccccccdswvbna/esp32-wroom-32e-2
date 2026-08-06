#pragma once
#include "esphome/core/component.h"
#include <queue>
#include <string>
#include "esp_gap_ble_api.h"

namespace esphome {
namespace ble_gateway {

class BLEGateway : public Component {
public:
    void setup() override;
    void loop() override;
    void send_hex(const std::string &hex);

    static BLEGateway *instance_;
    static void gap_callback(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

private:
    void raw_(const std::string &h);
    void next_();
    std::vector<uint8_t> hex_to_bytes(const std::string &h);
    std::queue<std::vector<uint8_t>> tx_queue_;
    std::vector<uint8_t> current_packet_;
    esp_ble_adv_params_t adv_params_{};
    bool advertising_{false};
    bool busy_{false};
    uint32_t t0_{0}, t1_{0}, t2_{0};
};

}  // namespace ble_gateway
}  // namespace esphome
