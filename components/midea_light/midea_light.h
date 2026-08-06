#pragma once
#include "esphome/core/component.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_state.h"

namespace esphome {
namespace midea_light {

class MideaLight : public light::LightOutput, public Component {
public:
    void set_mac(const std::string &mac) { mac_ = mac; }
    
    light::LightTraits get_traits() override {
        auto traits = light::LightTraits();
        traits.set_supports_brightness(true);
        traits.set_supports_color_temperature(true);
        traits.set_min_mireds(153);
        traits.set_max_mireds(370);
        return traits;
    }
    
    void write_state(light::LightState *state) override {
        float bri = state->current_values.get_brightness();
        float ct = state->current_values.get_color_temperature();
        int pct = (int)(bri * 100.0f);
        int kelvin = (int)(1000000.0f / ct);
        
        std::string cmd;
        if (state->current_values.is_on()) {
            cmd = midea_light_on(mac_, pct, kelvin);
        } else {
            cmd = midea_light_off(mac_);
        }
        ble_tx_send(cmd);
    }
    
private:
    std::string mac_;
};

} // namespace midea_light
} // namespace esphome
