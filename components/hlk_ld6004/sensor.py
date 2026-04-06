import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    UNIT_METER,
    ICON_SIGNAL,
    STATE_CLASS_MEASUREMENT,
)
from . import HLKLD6004Component, CONF_HLK_LD6004_ID, hlk_ld6004_ns

CONF_TARGET_COUNT = "target_count"

CONF_TARGET0_X = "target0_x"
CONF_TARGET0_Y = "target0_y"
CONF_TARGET0_Z = "target0_z"
CONF_TARGET0_DOPPLER = "target0_doppler"
CONF_TARGET0_ID = "target0_id"

CONF_TARGET1_X = "target1_x"
CONF_TARGET1_Y = "target1_y"
CONF_TARGET1_Z = "target1_z"
CONF_TARGET1_DOPPLER = "target1_doppler"
CONF_TARGET1_ID = "target1_id"

CONF_TARGET2_X = "target2_x"
CONF_TARGET2_Y = "target2_y"
CONF_TARGET2_Z = "target2_z"
CONF_TARGET2_DOPPLER = "target2_doppler"
CONF_TARGET2_ID = "target2_id"

ICON_RADAR = "mdi:radar"
ICON_TARGET = "mdi:target"
ICON_SPEEDOMETER = "mdi:speedometer"
ICON_IDENTIFIER = "mdi:identifier"

_target_schema = {
    "_x": sensor.sensor_schema(
        unit_of_measurement=UNIT_METER,
        accuracy_decimals=3,
        state_class=STATE_CLASS_MEASUREMENT,
        icon=ICON_TARGET,
    ),
    "_y": sensor.sensor_schema(
        unit_of_measurement=UNIT_METER,
        accuracy_decimals=3,
        state_class=STATE_CLASS_MEASUREMENT,
        icon=ICON_TARGET,
    ),
    "_z": sensor.sensor_schema(
        unit_of_measurement=UNIT_METER,
        accuracy_decimals=3,
        state_class=STATE_CLASS_MEASUREMENT,
        icon=ICON_TARGET,
    ),
    "_doppler": sensor.sensor_schema(
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
        icon=ICON_SPEEDOMETER,
    ),
    "_id": sensor.sensor_schema(
        accuracy_decimals=0,
        icon=ICON_IDENTIFIER,
    ),
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_HLK_LD6004_ID): cv.use_id(HLKLD6004Component),
        cv.Optional(CONF_TARGET_COUNT): sensor.sensor_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            icon=ICON_RADAR,
        ),
        # Target 0
        cv.Optional(CONF_TARGET0_X): _target_schema["_x"],
        cv.Optional(CONF_TARGET0_Y): _target_schema["_y"],
        cv.Optional(CONF_TARGET0_Z): _target_schema["_z"],
        cv.Optional(CONF_TARGET0_DOPPLER): _target_schema["_doppler"],
        cv.Optional(CONF_TARGET0_ID): _target_schema["_id"],
        # Target 1
        cv.Optional(CONF_TARGET1_X): _target_schema["_x"],
        cv.Optional(CONF_TARGET1_Y): _target_schema["_y"],
        cv.Optional(CONF_TARGET1_Z): _target_schema["_z"],
        cv.Optional(CONF_TARGET1_DOPPLER): _target_schema["_doppler"],
        cv.Optional(CONF_TARGET1_ID): _target_schema["_id"],
        # Target 2
        cv.Optional(CONF_TARGET2_X): _target_schema["_x"],
        cv.Optional(CONF_TARGET2_Y): _target_schema["_y"],
        cv.Optional(CONF_TARGET2_Z): _target_schema["_z"],
        cv.Optional(CONF_TARGET2_DOPPLER): _target_schema["_doppler"],
        cv.Optional(CONF_TARGET2_ID): _target_schema["_id"],
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_HLK_LD6004_ID])

    if CONF_TARGET_COUNT in config:
        sens = await sensor.new_sensor(config[CONF_TARGET_COUNT])
        cg.add(hub.set_target_count_sensor(sens))

    # Target 0/1/2
    target_configs = [
        (0, CONF_TARGET0_X, CONF_TARGET0_Y, CONF_TARGET0_Z, CONF_TARGET0_DOPPLER, CONF_TARGET0_ID),
        (1, CONF_TARGET1_X, CONF_TARGET1_Y, CONF_TARGET1_Z, CONF_TARGET1_DOPPLER, CONF_TARGET1_ID),
        (2, CONF_TARGET2_X, CONF_TARGET2_Y, CONF_TARGET2_Z, CONF_TARGET2_DOPPLER, CONF_TARGET2_ID),
    ]

    for idx, cx, cy, cz, cd, ci in target_configs:
        if cx in config:
            sens = await sensor.new_sensor(config[cx])
            cg.add(hub.set_target_x_sensor(idx, sens))
        if cy in config:
            sens = await sensor.new_sensor(config[cy])
            cg.add(hub.set_target_y_sensor(idx, sens))
        if cz in config:
            sens = await sensor.new_sensor(config[cz])
            cg.add(hub.set_target_z_sensor(idx, sens))
        if cd in config:
            sens = await sensor.new_sensor(config[cd])
            cg.add(hub.set_target_doppler_sensor(idx, sens))
        if ci in config:
            sens = await sensor.new_sensor(config[ci])
            cg.add(hub.set_target_id_sensor(idx, sens))
