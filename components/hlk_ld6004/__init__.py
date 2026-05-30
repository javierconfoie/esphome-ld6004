import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

CODEOWNERS = ["@javiermugueta"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["binary_sensor", "sensor", "text_sensor", "select", "number", "switch", "button"]
MULTI_CONF = True

CONF_HLK_LD6004_ID = "hlk_ld6004_id"

hlk_ld6004_ns = cg.esphome_ns.namespace("hlk_ld6004")
HLKLD6004Component = hlk_ld6004_ns.class_(
    "HLKLD6004Component", cg.Component, uart.UARTDevice
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(HLKLD6004Component),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "hlk_ld6004", baud_rate=115200, require_rx=True, require_tx=True
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
