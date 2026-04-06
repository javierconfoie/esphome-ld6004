import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import CONF_ID, ENTITY_CATEGORY_CONFIG
from . import HLKLD6004Component, CONF_HLK_LD6004_ID, hlk_ld6004_ns

HLKLD6004Select = hlk_ld6004_ns.class_("HLKLD6004Select", select.Select, cg.Parented.template(HLKLD6004Component))

CONF_SENSITIVITY = "sensitivity"
CONF_TRIGGER_SPEED = "trigger_speed"
CONF_INSTALL_METHOD = "install_method"
CONF_OPERATING_MODE = "operating_mode"
CONF_P20_MODE = "p20_mode"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_HLK_LD6004_ID): cv.use_id(HLKLD6004Component),
        cv.Optional(CONF_SENSITIVITY): select.select_schema(
            HLKLD6004Select,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:signal-cellular-3",
        ),
        cv.Optional(CONF_TRIGGER_SPEED): select.select_schema(
            HLKLD6004Select,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:speedometer",
        ),
        cv.Optional(CONF_INSTALL_METHOD): select.select_schema(
            HLKLD6004Select,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:wrench",
        ),
        cv.Optional(CONF_OPERATING_MODE): select.select_schema(
            HLKLD6004Select,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:cog",
        ),
        cv.Optional(CONF_P20_MODE): select.select_schema(
            HLKLD6004Select,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:electric-switch",
        ),
    }
)


async def _new_select(config, key, hub, setter, select_type, options):
    if key not in config:
        return
    sel = await select.new_select(config[key], options=options)
    await cg.register_parented(sel, config[CONF_HLK_LD6004_ID])
    cg.add(getattr(hub, setter)(sel))
    cg.add(sel.set_select_type(select_type))


async def to_code(config):
    hub = await cg.get_variable(config[CONF_HLK_LD6004_ID])

    await _new_select(config, CONF_SENSITIVITY, hub, "set_sensitivity_select",
                      "sensitivity", ["Low", "Medium", "High"])
    await _new_select(config, CONF_TRIGGER_SPEED, hub, "set_trigger_speed_select",
                      "trigger_speed", ["Slow", "Medium", "Fast"])
    await _new_select(config, CONF_INSTALL_METHOD, hub, "set_install_method_select",
                      "install_method", ["Top", "Side"])
    await _new_select(config, CONF_OPERATING_MODE, hub, "set_operating_mode_select",
                      "operating_mode", ["Low Power", "Normal", "Off P20 High", "Off P20 Low", "High Reflectivity"])
    await _new_select(config, CONF_P20_MODE, hub, "set_p20_mode_select",
                      "p20_mode", ["High Present", "Low Present", "Always Low", "Always High", "Pulse Low", "Pulse High"])
