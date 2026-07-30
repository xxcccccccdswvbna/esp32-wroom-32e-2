#pragma once
#include "esphome/core/component.h"
#include <deque>
#include <string>
#include <vector>

namespace esphome {
namespace ble_tx {

class BLETx : public Component {
public:
    void setup() override;
    void loop() override;
    void send_hex(const std::string &hex);

private:
    static constexpr uint32_t ADV_MS   = 200;
    static constexpr uint32_t COOL_MS  = 500;
    static constexpr uint32_t GAP_MS   = 800;

    bool     adv_{false}, cool_{false}, wait_{false};
    uint32_t t0_{0}, t1_{0}, t2_{0};
    std::deque<std::string> q_;

    void raw_(const std::string &h);
    void next_();
    static std::vector<uint8_t> bytes_(const std::string &h);
};

}  // namespace ble_tx
}  // namespace esphome
