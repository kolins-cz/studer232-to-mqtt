"""Sensor platform for Studer component."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    CONF_ADDRESS,
    STATE_CLASS_MEASUREMENT,
)
from . import StuderComponent, studer_ns, CONF_STUDER_ID

DEPENDENCIES = ["studer"]

StuderSensor = studer_ns.class_("StuderSensor", sensor.Sensor, cg.Component)

CONF_PARAMETER = "parameter"
CONF_MULTIPLY = "multiply"

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        StuderSensor,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
            cv.GenerateID(CONF_STUDER_ID): cv.use_id(StuderComponent),
            cv.Required(CONF_ADDRESS): cv.int_range(min=0, max=715),
            cv.Required(CONF_PARAMETER): cv.int_range(min=0, max=65535),
            cv.Optional(CONF_MULTIPLY, default=1.0): cv.float_,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    """Generate C++ code for the sensor."""
    parent = await cg.get_variable(config[CONF_STUDER_ID])
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    
    cg.add(var.set_parent(parent))
    cg.add(var.set_address(config[CONF_ADDRESS]))
    cg.add(var.set_parameter(config[CONF_PARAMETER]))
    cg.add(var.set_multiply(config[CONF_MULTIPLY]))
    
    cg.add(parent.register_sensor(var))
