import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID, ENTITY_CATEGORY_DIAGNOSTIC
from . import HLKLD6004Component, CONF_HLK_LD6004_ID, hlk_ld6004_ns

CONF_FIRMWARE_VERSION = "firmware_version"
CONF_DETECTION_ZONES = "detection_zones"
CONF_INTERFERENCE_ZONES = "interference_zones"
CONF_DWELL_ZONES = "dwell_zones"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_HLK_LD6004_ID): cv.use_id(HLKLD6004Component),
        cv.Optional(CONF_FIRMWARE_VERSION): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:chip",
        ),
        cv.Optional(CONF_DETECTION_ZONES): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:vector-square",
        ),
        cv.Optional(CONF_INTERFERENCE_ZONES): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:vector-square-minus",
        ),
        cv.Optional(CONF_DWELL_ZONES): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:seat-outline",
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_HLK_LD6004_ID])

    if CONF_FIRMWARE_VERSION in config:
        sens = await text_sensor.new_text_sensor(config[CONF_FIRMWARE_VERSION])
        cg.add(hub.set_firmware_text_sensor(sens))

    if CONF_DETECTION_ZONES in config:
        sens = await text_sensor.new_text_sensor(config[CONF_DETECTION_ZONES])
        cg.add(hub.set_detection_zones_text_sensor(sens))

    if CONF_INTERFERENCE_ZONES in config:
        sens = await text_sensor.new_text_sensor(config[CONF_INTERFERENCE_ZONES])
        cg.add(hub.set_interference_zones_text_sensor(sens))

    if CONF_DWELL_ZONES in config:
        sens = await text_sensor.new_text_sensor(config[CONF_DWELL_ZONES])
        cg.add(hub.set_dwell_zones_text_sensor(sens))
