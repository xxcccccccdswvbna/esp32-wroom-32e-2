#pragma once
#include "esphome/core/component.h"
#include "esphome/components/fan/fan.h"
#include "midea_ble_controller.h"

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
        if (!cmd.empty()) {
            // 调用 BLE 网关发送
            // 注意：需要在组件中获取 BLEGateway 实例，这里使用全局方式或通过 ID 获取
            extern void ble_gw_send(const std::string &hex);
            ble_gw_send(cmd);
        }
    }

private:
    std::string mac_;
};

}  // namespace midea_fan
}  // namespace esphome
