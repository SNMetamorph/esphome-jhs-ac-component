import esphome.config_validation as cv
import esphome.codegen as cg

from esphome.components import climate, uart
from esphome.const import (
    CONF_ID,
    CONF_SUPPORTED_PRESETS,
)
from esphome.components.climate import (
    ClimatePreset,
)

CODEOWNERS = ["@SNMetamorph"]
DEPENDENCIES = ["climate", "uart"]

jhs_ac_ns = cg.esphome_ns.namespace("jhs_ac")
JhsAirConditioner = jhs_ac_ns.class_(
    "JhsAirConditioner", climate.Climate, uart.UARTDevice, cg.Component
)

CONFIG_SCHEMA = cv.All(
    climate.climate_schema(JhsAirConditioner)
    .extend(uart.UART_DEVICE_SCHEMA),
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
    await uart.register_uart_device(var, config)
