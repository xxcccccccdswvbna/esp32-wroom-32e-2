import esphome.codegen as cg

ble_gateway_ns = cg.esphome_ns.namespace('ble_gateway')
BLEGateway = ble_gateway_ns.class_('BLEGateway', cg.Component)

CONFIG_SCHEMA = cg.ComponentSchema()

async def to_code(config):
    var = cg.new_Pvariable(config[cv.CONF_ID])
    await cg.register_component(var, config)
