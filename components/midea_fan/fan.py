import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import fan
from esphome.const import CONF_ID, CONF_MAC

DEPENDENCIES = ['ble_tx']

midea_fan_ns = cg.esphome_ns.namespace('midea_fan')
MideaFan = midea_fan_ns.class_('MideaFan', fan.Fan, cg.Component)

CONFIG_SCHEMA = fan.FAN_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(MideaFan),
    cv.Required(CONF_MAC): cv.string,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await fan.register_fan(var, config)
    cg.add(var.set_mac(config[CONF_MAC]))
    cg.add(var.set_speed_count(6))
