import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID
from . import HLKLD6004Component, CONF_HLK_LD6004_ID, hlk_ld6004_ns

CONF_PRESENCE = "presence"
CONF_ZONE0_PRESENCE = "zone0_presence"
CONF_ZONE1_PRESENCE = "zone1_presence"
CONF_ZONE2_PRESENCE = "zone2_presence"
CONF_ZONE3_PRESENCE = "zone3_presence"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_HLK_LD6004_ID): cv.use_id(HLKLD6004Component),
        cv.Optional(CONF_PRESENCE): binary_sensor.binary_sensor_schema(
            device_class="occupancy",
        ),
        cv.Optional(CONF_ZONE0_PRESENCE): binary_sensor.binary_sensor_schema(
            device_class="occupancy",
        ),
        cv.Optional(CONF_ZONE1_PRESENCE): binary_sensor.binary_sensor_schema(
            device_class="occupancy",
        ),
        cv.Optional(CONF_ZONE2_PRESENCE): binary_sensor.binary_sensor_schema(
            device_class="occupancy",
        ),
        cv.Optional(CONF_ZONE3_PRESENCE): binary_sensor.binary_sensor_schema(
            device_class="occupancy",
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_HLK_LD6004_ID])

    if CONF_PRESENCE in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_PRESENCE])
        cg.add(hub.set_presence_binary_sensor(sens))

    for i, key in enumerate([CONF_ZONE0_PRESENCE, CONF_ZONE1_PRESENCE,
                             CONF_ZONE2_PRESENCE, CONF_ZONE3_PRESENCE]):
        if key in config:
            sens = await binary_sensor.new_binary_sensor(config[key])
            cg.add(hub.set_zone_presence_binary_sensor(i, sens))
