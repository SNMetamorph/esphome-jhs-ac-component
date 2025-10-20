import esphome.config_validation as cv
import esphome.codegen as cg

from esphome.components import climate, uart, binary_sensor
from esphome.const import (
    CONF_ID,
    CONF_INTERNAL
)
from esphome.components.climate import (
    ClimatePreset,
    validate_climate_fan_mode,
    ClimateFanMode,
)

CODEOWNERS = ["@SNMetamorph"]
DEPENDENCIES = ["climate", "uart"]
AUTO_LOAD = ["binary_sensor"]

CONF_PROTOCOL_VERSION = "protocol_version"
CONF_SUPPORTED_FAN_MODES = "supported_fan_modes"

CONF_WATER_TANK_STATUS = "water_tank_status"
ICON_WATER_TANK_STATUS = "mdi:water-alert"

jhs_ac_ns = cg.esphome_ns.namespace("jhs_ac")
JhsAirConditioner = jhs_ac_ns.class_(
    "JhsAirConditioner", climate.Climate, uart.UARTDevice, cg.Component
)

CONFIG_SCHEMA = cv.All(
    climate.climate_schema(JhsAirConditioner).extend(
        {
            cv.Required(CONF_PROTOCOL_VERSION): cv.int_range(1, 2),
            cv.Optional(CONF_SUPPORTED_FAN_MODES): cv.ensure_list(validate_climate_fan_mode),
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

    cg.add_define("JHS_AC_PROTOCOL_VERSION", config[CONF_PROTOCOL_VERSION])
    
    # Configure supported fan modes
    if CONF_SUPPORTED_FAN_MODES in config:
        for fan_mode in config[CONF_SUPPORTED_FAN_MODES]:
            cg.add(var.add_supported_fan_mode(fan_mode))
    else:
        # Default to LOW and HIGH for backward compatibility
        # Use the validation function to convert strings to enum values
        default_low = validate_climate_fan_mode("LOW")
        default_high = validate_climate_fan_mode("HIGH")
        cg.add(var.add_supported_fan_mode(default_low))
        cg.add(var.add_supported_fan_mode(default_high))
    
    if CONF_WATER_TANK_STATUS in config:
        conf = config[CONF_WATER_TANK_STATUS]
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(var.set_water_tank_sensor(sens))
