import esphome.config_validation as cv
import esphome.codegen as cg

from esphome.components import climate, uart, binary_sensor
from esphome.const import (
    CONF_ID,
    CONF_INTERNAL
)
from esphome.components.climate import (
    ClimatePreset,
)

CODEOWNERS = ["@SNMetamorph"]
DEPENDENCIES = ["climate", "uart"]
AUTO_LOAD = ["binary_sensor"]

CONF_WATER_TANK_STATUS = "water_tank_status"
ICON_WATER_TANK_STATUS = "mdi:water-alert"

jhs_ac_ns = cg.esphome_ns.namespace("jhs_ac")
JhsAirConditioner = jhs_ac_ns.class_(
    "JhsAirConditioner", climate.Climate, uart.UARTDevice, cg.Component
)

CONFIG_SCHEMA = cv.All(
    climate.climate_schema(JhsAirConditioner).extend(
        {
            cv.Optional(CONF_WATER_TANK_STATUS): binary_sensor.binary_sensor_schema(
                icon=ICON_WATER_TANK_STATUS,
            ).extend(
                {
                    cv.Optional(CONF_INTERNAL, default="true"): cv.boolean,
                }
            ),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA),
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
    await uart.register_uart_device(var, config)

    if CONF_WATER_TANK_STATUS in config:
        conf = config[CONF_WATER_TANK_STATUS]
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(var.set_water_tank_sensor(sens))
