#pragma once
#include "esphome/core/component.h"
#include "esphome/components/fan/fan.h"
#include "midea_ble_controller.h"
#include "components/ble_tx/ble_tx.h"

namespace esphome {
namespace midea_fan {

class MideaFan : public fan::Fan, public Component {
public:
    void set_mac(const std::string &mac) { mac_ = mac; }

    fan::FanTraits get_traits() override {
        return fan::FanTraits(false, true, false, 6);
    }

    void write_state() override {
        std::string cmd;
        if (state) {
            cmd = midea_fan_on(mac_, speed);
        } else {
            cmd = midea_fan_off(mac_);
        }
        if (!cmd.empty()) ble_tx_send(cmd);
    }

private:
    std::string mac_;
};

}  // namespace midea_fan
}  // namespace esphome
