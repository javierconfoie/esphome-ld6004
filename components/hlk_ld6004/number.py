import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import (
    CONF_ID,
    ENTITY_CATEGORY_CONFIG,
    UNIT_SECOND,
)
from . import HLKLD6004Component, CONF_HLK_LD6004_ID, hlk_ld6004_ns

HLKLD6004Number = hlk_ld6004_ns.class_("HLKLD6004Number", number.Number, cg.Parented.template(HLKLD6004Component))

CONF_DELAY_TIME = "delay_time"
CONF_SLEEP_TIME = "sleep_time"
CONF_OUTPUT_INTERVAL = "output_interval"
CONF_DWELL_LIFECYCLE = "dwell_lifecycle"
CONF_Z_RANGE_MIN = "z_range_min"
CONF_Z_RANGE_MAX = "z_range_max"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_HLK_LD6004_ID): cv.use_id(HLKLD6004Component),
        cv.Optional(CONF_DELAY_TIME): number.number_schema(
            HLKLD6004Number,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:timer-sand",
            unit_of_measurement=UNIT_SECOND,
        ),
        cv.Optional(CONF_SLEEP_TIME): number.number_schema(
            HLKLD6004Number,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:sleep",
            unit_of_measurement="ms",
        ),
        cv.Optional(CONF_OUTPUT_INTERVAL): number.number_schema(
            HLKLD6004Number,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:metronome",
        ),
        cv.Optional(CONF_DWELL_LIFECYCLE): number.number_schema(
            HLKLD6004Number,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:timer-outline",
        ),
        cv.Optional(CONF_Z_RANGE_MIN): number.number_schema(
            HLKLD6004Number,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:arrow-collapse-down",
            unit_of_measurement="m",
        ),
        cv.Optional(CONF_Z_RANGE_MAX): number.number_schema(
            HLKLD6004Number,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:arrow-collapse-up",
            unit_of_measurement="m",
        ),
    }
)


async def _new_number(config, key, hub, setter, number_type, min_val, max_val, step):
    if key not in config:
        return
    num = await number.new_number(
        config[key],
        min_value=min_val,
        max_value=max_val,
        step=step,
    )
    await cg.register_parented(num, config[CONF_HLK_LD6004_ID])
    cg.add(getattr(hub, setter)(num))
    cg.add(num.set_number_type(number_type))


async def to_code(config):
    hub = await cg.get_variable(config[CONF_HLK_LD6004_ID])

    await _new_number(config, CONF_DELAY_TIME, hub, "set_delay_time_number",
                      "delay_time", 0, 65535, 1)
    await _new_number(config, CONF_SLEEP_TIME, hub, "set_sleep_time_number",
                      "sleep_time", 0, 5000, 50)
    await _new_number(config, CONF_OUTPUT_INTERVAL, hub, "set_output_interval_number",
                      "output_interval", 1, 100, 1)
    await _new_number(config, CONF_DWELL_LIFECYCLE, hub, "set_dwell_lifecycle_number",
                      "dwell_lifecycle", 20, 65535, 1)
    await _new_number(config, CONF_Z_RANGE_MIN, hub, "set_z_range_min_number",
                      "z_range_min", -6.0, 6.0, 0.1)
    await _new_number(config, CONF_Z_RANGE_MAX, hub, "set_z_range_max_number",
                      "z_range_max", -6.0, 6.0, 0.1)
