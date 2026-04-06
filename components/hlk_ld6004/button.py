import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import CONF_ID, ENTITY_CATEGORY_CONFIG
from . import HLKLD6004Component, CONF_HLK_LD6004_ID, hlk_ld6004_ns

HLKLD6004Button = hlk_ld6004_ns.class_("HLKLD6004Button", button.Button, cg.Parented.template(HLKLD6004Component))

CONF_RESET_UNMANNED = "reset_unmanned"
CONF_AUTO_INTERFERENCE = "auto_interference"
CONF_CLEAR_INTERFERENCE = "clear_interference"
CONF_RESET_DETECTION = "reset_detection"
CONF_CLEAR_DWELL = "clear_dwell"
CONF_REFRESH_CONFIG = "refresh_config"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_HLK_LD6004_ID): cv.use_id(HLKLD6004Component),
        cv.Optional(CONF_RESET_UNMANNED): button.button_schema(
            HLKLD6004Button,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:restart",
        ),
        cv.Optional(CONF_AUTO_INTERFERENCE): button.button_schema(
            HLKLD6004Button,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:shield-search",
        ),
        cv.Optional(CONF_CLEAR_INTERFERENCE): button.button_schema(
            HLKLD6004Button,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:shield-off",
        ),
        cv.Optional(CONF_RESET_DETECTION): button.button_schema(
            HLKLD6004Button,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:restore",
        ),
        cv.Optional(CONF_CLEAR_DWELL): button.button_schema(
            HLKLD6004Button,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:map-marker-off",
        ),
        cv.Optional(CONF_REFRESH_CONFIG): button.button_schema(
            HLKLD6004Button,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:refresh",
        ),
    }
)


async def _new_button(config, key, hub, setter, button_type):
    if key not in config:
        return
    btn = await button.new_button(config[key])
    await cg.register_parented(btn, config[CONF_HLK_LD6004_ID])
    cg.add(getattr(hub, setter)(btn))
    cg.add(btn.set_button_type(button_type))


async def to_code(config):
    hub = await cg.get_variable(config[CONF_HLK_LD6004_ID])

    await _new_button(config, CONF_RESET_UNMANNED, hub, "set_reset_unmanned_button", "reset_unmanned")
    await _new_button(config, CONF_AUTO_INTERFERENCE, hub, "set_auto_interference_button", "auto_interference")
    await _new_button(config, CONF_CLEAR_INTERFERENCE, hub, "set_clear_interference_button", "clear_interference")
    await _new_button(config, CONF_RESET_DETECTION, hub, "set_reset_detection_button", "reset_detection")
    await _new_button(config, CONF_CLEAR_DWELL, hub, "set_clear_dwell_button", "clear_dwell")
    await _new_button(config, CONF_REFRESH_CONFIG, hub, "set_refresh_config_button", "refresh_config")
