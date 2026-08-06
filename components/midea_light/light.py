import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_ID, CONF_NAME, CONF_MAC
from esphome.core import CORE

DEPENDENCIES = ['ble_tx']

midea_light_ns = cg.esphome_ns.namespace('midea_light')
MideaLight = midea_light_ns.class_('MideaLight', light.LightOutput, cg.Component)

CONFIG_SCHEMA = light.BRIGHTNESS_COLOR_TEMPERATURE_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(MideaLight),
    cv.Required(CONF_MAC): cv.string,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await light.register_light(var, config)
    cg.add(var.set_mac(config[CONF_MAC]))
    
    # 注入头文件
    CORE.add_include('midea_ble_controller.h')
    CORE.add_include('ble_tx.h')
    
    # 注册类定义
    cg.add_define('MIDEA_LIGHT_DEFINED')
