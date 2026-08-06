import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_ID, CONF_MAC
from esphome.core import CORE

DEPENDENCIES = ['ble_tx']

midea_light_ns = cg.esphome_ns.namespace('midea_light')
MideaLight = midea_light_ns.class_('MideaLight', light.LightOutput, cg.Component)

# 🔥 手动注册 C++ 头文件
CORE.add_include('midea_ble_controller.h')
CORE.add_include('components/ble_tx/ble_tx.h')

CONFIG_SCHEMA = light.BRIGHTNESS_COLOR_TEMPERATURE_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(MideaLight),
    cv.Required(CONF_MAC): cv.string,
}).extend(cv.COMPONENT_SCHEMA)

# 🔥 注入 C++ 实现
@cg.automation.register_action(
    'midea_light.setup',
    MideaLight,
    cv.Schema({})
)
async def midea_light_setup_to_code(config, action_id, template_arg, args):
    return cg.RawExpression('')

# 🔥 编译时注入类定义
cg.add_define('MIDEA_LIGHT_IMPLEMENTATION', '''
#include "esphome/core/component.h"
#include "esphome/components/light/light_output.h"
#include "midea_ble_controller.h"
#include "components/ble_tx/ble_tx.h"

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
        if (!cmd.empty()) ble_tx_send(cmd);
    }

private:
    std::string mac_;
};

}  // namespace midea_light
}  // namespace esphome
''')

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await light.register_light(var, config)
    cg.add(var.set_mac(config[CONF_MAC]))
