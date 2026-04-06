import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID, ENTITY_CATEGORY_CONFIG
from . import HLKLD6004Component, CONF_HLK_LD6004_ID, hlk_ld6004_ns

HLKLD6004Switch = hlk_ld6004_ns.class_("HLKLD6004Switch", switch.Switch, cg.Parented.template(HLKLD6004Component))

CONF_POINT_CLOUD = "point_cloud"
CONF_TARGET_DISPLAY = "target_display"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_HLK_LD6004_ID): cv.use_id(HLKLD6004Component),
        cv.Optional(CONF_POINT_CLOUD): switch.switch_schema(
            HLKLD6004Switch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:scatter-plot",
        ),
        cv.Optional(CONF_TARGET_DISPLAY): switch.switch_schema(
            HLKLD6004Switch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:target-account",
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_HLK_LD6004_ID])

    if CONF_POINT_CLOUD in config:
        sw = await switch.new_switch(config[CONF_POINT_CLOUD])
        await cg.register_parented(sw, config[CONF_HLK_LD6004_ID])
        cg.add(hub.set_point_cloud_switch(sw))
        cg.add(sw.set_switch_type("point_cloud"))

    if CONF_TARGET_DISPLAY in config:
        sw = await switch.new_switch(config[CONF_TARGET_DISPLAY])
        await cg.register_parented(sw, config[CONF_HLK_LD6004_ID])
        cg.add(hub.set_target_display_switch(sw))
        cg.add(sw.set_switch_type("target_display"))
