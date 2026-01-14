"""ESPHome component for Studer Innotec devices via SCOM protocol."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID
from esphome.core import CORE

DEPENDENCIES = ["uart"]
CODEOWNERS = ["@kolins-cz"]

studer_ns = cg.esphome_ns.namespace("studer")
StuderComponent = studer_ns.class_("StuderComponent", cg.Component, uart.UARTDevice)

CONF_STUDER_ID = "studer_id"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(StuderComponent),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    """Generate C++ code for the component."""
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
