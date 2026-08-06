import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_ID

DEPENDENCIES = ['ble_tx']

# 🔥 自定义 mac 配置键
CONF_DEVICE_MAC = "mac"

midea_light_ns = cg.esphome_ns.namespace('midea_light')
MideaLight = midea_light_ns.class_('MideaLight', light.LightOutput, cg.Component)

CONFIG_SCHEMA = light.BRIGHTNESS_COLOR_TEMPERATURE_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(MideaLight),
    cv.Required(CONF_DEVICE_MAC): cv.string,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await light.register_light(var, config)
    cg.add(var.set_mac(config[CONF_DEVICE_MAC]))
