#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"

#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif
#ifdef USE_SELECT
#include "esphome/components/select/select.h"
#endif
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif
#ifdef USE_BUTTON
#include "esphome/components/button/button.h"
#endif

#include <cstring>
#include <string>

namespace esphome {
namespace hlk_ld6004 {

static const uint8_t MAX_TARGETS = 3;
static const uint8_t MAX_ZONES = 4;

// --- Data structures ---

struct Target {
  float x{0};
  float y{0};
  float z{0};
  int32_t doppler{0};
  int32_t id{0};
};

struct Zone {
  float x_min{0}, x_max{0};
  float y_min{0}, y_max{0};
  float z_min{0}, z_max{0};
};

// --- Forward declarations for platform classes ---

#ifdef USE_SELECT
class HLKLD6004Select;
#endif
#ifdef USE_NUMBER
class HLKLD6004Number;
#endif
#ifdef USE_SWITCH
class HLKLD6004Switch;
#endif
#ifdef USE_BUTTON
class HLKLD6004Button;
#endif

// --- Main hub component ---

class HLKLD6004Component : public Component, public uart::UARTDevice {
 public:
  float get_setup_priority() const override { return setup_priority::DATA; }
  void setup() override;
  void loop() override;
  void dump_config() override;

  // --- Binary sensor setters ---
#ifdef USE_BINARY_SENSOR
  void set_presence_binary_sensor(binary_sensor::BinarySensor *s) { this->presence_binary_sensor_ = s; }
  void set_zone_presence_binary_sensor(uint8_t zone, binary_sensor::BinarySensor *s) {
    if (zone < MAX_ZONES) this->zone_presence_binary_sensors_[zone] = s;
  }
#endif

  // --- Sensor setters ---
#ifdef USE_SENSOR
  void set_target_count_sensor(sensor::Sensor *s) { this->target_count_sensor_ = s; }
  void set_target_x_sensor(uint8_t idx, sensor::Sensor *s) { if (idx < MAX_TARGETS) this->target_x_sensors_[idx] = s; }
  void set_target_y_sensor(uint8_t idx, sensor::Sensor *s) { if (idx < MAX_TARGETS) this->target_y_sensors_[idx] = s; }
  void set_target_z_sensor(uint8_t idx, sensor::Sensor *s) { if (idx < MAX_TARGETS) this->target_z_sensors_[idx] = s; }
  void set_target_doppler_sensor(uint8_t idx, sensor::Sensor *s) { if (idx < MAX_TARGETS) this->target_doppler_sensors_[idx] = s; }
  void set_target_id_sensor(uint8_t idx, sensor::Sensor *s) { if (idx < MAX_TARGETS) this->target_id_sensors_[idx] = s; }
#endif

  // --- Text sensor setters ---
#ifdef USE_TEXT_SENSOR
  void set_firmware_text_sensor(text_sensor::TextSensor *s) { this->firmware_text_sensor_ = s; }
#endif

  // --- Select setters ---
#ifdef USE_SELECT
  void set_sensitivity_select(HLKLD6004Select *s) { this->sensitivity_select_ = s; }
  void set_trigger_speed_select(HLKLD6004Select *s) { this->trigger_speed_select_ = s; }
  void set_install_method_select(HLKLD6004Select *s) { this->install_method_select_ = s; }
  void set_operating_mode_select(HLKLD6004Select *s) { this->operating_mode_select_ = s; }
  void set_p20_mode_select(HLKLD6004Select *s) { this->p20_mode_select_ = s; }
#endif

  // --- Number setters ---
#ifdef USE_NUMBER
  void set_delay_time_number(HLKLD6004Number *n) { this->delay_time_number_ = n; }
  void set_sleep_time_number(HLKLD6004Number *n) { this->sleep_time_number_ = n; }
  void set_output_interval_number(HLKLD6004Number *n) { this->output_interval_number_ = n; }
  void set_dwell_lifecycle_number(HLKLD6004Number *n) { this->dwell_lifecycle_number_ = n; }
  void set_z_range_min_number(HLKLD6004Number *n) { this->z_range_min_number_ = n; }
  void set_z_range_max_number(HLKLD6004Number *n) { this->z_range_max_number_ = n; }
#endif

  // --- Switch setters ---
#ifdef USE_SWITCH
  void set_point_cloud_switch(HLKLD6004Switch *s) { this->point_cloud_switch_ = s; }
  void set_target_display_switch(HLKLD6004Switch *s) { this->target_display_switch_ = s; }
#endif

  // --- Button setters ---
#ifdef USE_BUTTON
  void set_reset_unmanned_button(HLKLD6004Button *b) { this->reset_unmanned_button_ = b; }
  void set_auto_interference_button(HLKLD6004Button *b) { this->auto_interference_button_ = b; }
  void set_clear_interference_button(HLKLD6004Button *b) { this->clear_interference_button_ = b; }
  void set_reset_detection_button(HLKLD6004Button *b) { this->reset_detection_button_ = b; }
  void set_clear_dwell_button(HLKLD6004Button *b) { this->clear_dwell_button_ = b; }
  void set_refresh_config_button(HLKLD6004Button *b) { this->refresh_config_button_ = b; }
#endif

  // --- Accessors for platform classes ---
  float get_z_range_min() const { return this->z_range_min_; }
  float get_z_range_max() const { return this->z_range_max_; }

  // --- Commands (callable by platform classes) ---
  void send_control(uint32_t subcmd);
  void send_set_delay(uint32_t seconds);
  void send_set_sleep_time(uint32_t ms);
  void send_set_output_interval(uint32_t interval);
  void send_set_dwell_lifecycle(uint32_t lifecycle);
  void send_set_z_range(float z_min, float z_max);
  void send_set_zone(uint8_t area_id, float x_min, float x_max,
                     float y_min, float y_max, float z_min, float z_max);
  void send_set_baud_rate(uint8_t rate);
  void request_firmware_version();
  void query_all_config_();

 protected:
  // --- TinyFrame protocol ---
  static const uint8_t TF_SOF = 0x01;
  static const uint16_t TF_MAX_DATA = 256;
  static const uint8_t TF_HEADER_SIZE = 8;

  // Message types — host to radar
  static const uint16_t TYPE_CONTROL = 0x0201;
  static const uint16_t TYPE_SET_AREA = 0x0202;
  static const uint16_t TYPE_SET_DELAY = 0x0203;
  static const uint16_t TYPE_SET_Z_RANGE = 0x0204;
  static const uint16_t TYPE_SET_SLEEP_TIME = 0x0205;
  static const uint16_t TYPE_SET_DWELL_LIFE = 0x0206;
  static const uint16_t TYPE_SET_OUTPUT_INTV = 0x0207;
  static const uint16_t TYPE_SET_BAUD = 0x0F0F;
  static const uint16_t TYPE_FIRMWARE = 0xFFFF;

  // Message types — radar to host
  static const uint16_t TYPE_COMBINED_REPORT = 0x0100;  // Periodic combined report
  static const uint16_t TYPE_TARGET_LOCATION = 0x0A04;
  static const uint16_t TYPE_POINT_CLOUD = 0x0A08;
  static const uint16_t TYPE_PRESENCE_STATE = 0x0A0A;
  static const uint16_t TYPE_INTERFERENCE = 0x0A0B;
  static const uint16_t TYPE_DETECTION_AREA = 0x0A0C;
  static const uint16_t TYPE_DELAY_TIME = 0x0A0D;
  static const uint16_t TYPE_SENSITIVITY = 0x0A0E;
  static const uint16_t TYPE_TRIGGER_SPEED = 0x0A0F;
  static const uint16_t TYPE_Z_RANGE = 0x0A10;
  static const uint16_t TYPE_INSTALL_METHOD = 0x0A11;
  static const uint16_t TYPE_OPERATING_MODE = 0x0A12;
  static const uint16_t TYPE_SLEEP_TIME = 0x0A13;
  static const uint16_t TYPE_WORK_STATUS = 0x0A14;
  static const uint16_t TYPE_GPIO_STATUS = 0x0A15;
  static const uint16_t TYPE_DWELL_AREA = 0x0A16;
  static const uint16_t TYPE_DWELL_LIFECYCLE = 0x0A17;
  static const uint16_t TYPE_OUTPUT_INTERVAL = 0x0A18;

  // Parser state machine
  enum ParserState : uint8_t {
    WAIT_SOF,
    READ_HEADER,
    READ_DATA,
    READ_DATA_CKSUM,
  };

  void handle_byte_(uint8_t byte);
  void process_frame_(uint16_t type, const uint8_t *data, uint16_t len);
  void publish_presence_();
  void publish_targets_();
  void send_config_query_(uint8_t idx);

  // Frame encoding
  void send_frame_(uint16_t type, const uint8_t *data, uint16_t len);
  static uint8_t calc_checksum_(const uint8_t *data, uint16_t len);
  static float read_float_(const uint8_t *p);
  static uint32_t read_u32_le_(const uint8_t *p);
  static int32_t read_i32_le_(const uint8_t *p);

  // Parser state
  ParserState parser_state_{WAIT_SOF};
  uint8_t header_buf_[8]{};
  uint8_t data_buf_[256]{};
  uint16_t header_idx_{0};
  uint16_t data_idx_{0};
  uint16_t data_len_{0};
  uint16_t frame_type_{0};
  uint16_t send_id_{0};

  // Streaming data
  uint32_t presence_[MAX_ZONES]{};
  Target targets_[MAX_TARGETS]{};
  int target_count_{0};

  // Cached config
  uint8_t sensitivity_{2};       // HIGH
  uint8_t trigger_speed_{1};     // MEDIUM
  uint8_t install_method_{0};    // TOP
  uint8_t operating_mode_{1};    // NORMAL
  uint32_t delay_time_{30};
  uint32_t sleep_time_{500};
  uint32_t output_interval_{1};
  uint32_t dwell_lifecycle_{300};
  float z_range_min_{0.0f};
  float z_range_max_{0.0f};
  uint8_t p20_status_{0};        // HIGH_PRESENT
  uint8_t fw_project_{0};
  uint8_t fw_major_{0};
  uint8_t fw_sub_{0};
  uint8_t fw_modified_{0};

  // Zone data
  Zone detection_zones_[MAX_ZONES]{};
  Zone interference_zones_[MAX_ZONES]{};
  Zone dwell_zones_[MAX_ZONES]{};

  // Stats
  static const uint8_t CONFIG_QUERY_COUNT = 12;
  uint32_t frames_received_{0};
  uint32_t checksum_errors_{0};
  uint32_t last_update_time_{0};
  uint32_t last_init_attempt_{0};
  uint32_t init_retries_{0};
  uint8_t config_query_idx_{255};  // 255 = not querying
  uint32_t last_config_query_time_{0};
  bool initial_config_requested_{false};
  bool data_frames_received_{false};
  bool combined_report_logged_{false};
  bool config_received_{false};

  // --- Platform entity pointers ---
#ifdef USE_BINARY_SENSOR
  binary_sensor::BinarySensor *presence_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *zone_presence_binary_sensors_[MAX_ZONES]{};
#endif
#ifdef USE_SENSOR
  sensor::Sensor *target_count_sensor_{nullptr};
  sensor::Sensor *target_x_sensors_[MAX_TARGETS]{};
  sensor::Sensor *target_y_sensors_[MAX_TARGETS]{};
  sensor::Sensor *target_z_sensors_[MAX_TARGETS]{};
  sensor::Sensor *target_doppler_sensors_[MAX_TARGETS]{};
  sensor::Sensor *target_id_sensors_[MAX_TARGETS]{};
#endif
#ifdef USE_TEXT_SENSOR
  text_sensor::TextSensor *firmware_text_sensor_{nullptr};
#endif
#ifdef USE_SELECT
  HLKLD6004Select *sensitivity_select_{nullptr};
  HLKLD6004Select *trigger_speed_select_{nullptr};
  HLKLD6004Select *install_method_select_{nullptr};
  HLKLD6004Select *operating_mode_select_{nullptr};
  HLKLD6004Select *p20_mode_select_{nullptr};
#endif
#ifdef USE_NUMBER
  HLKLD6004Number *delay_time_number_{nullptr};
  HLKLD6004Number *sleep_time_number_{nullptr};
  HLKLD6004Number *output_interval_number_{nullptr};
  HLKLD6004Number *dwell_lifecycle_number_{nullptr};
  HLKLD6004Number *z_range_min_number_{nullptr};
  HLKLD6004Number *z_range_max_number_{nullptr};
#endif
#ifdef USE_SWITCH
  HLKLD6004Switch *point_cloud_switch_{nullptr};
  HLKLD6004Switch *target_display_switch_{nullptr};
#endif
#ifdef USE_BUTTON
  HLKLD6004Button *reset_unmanned_button_{nullptr};
  HLKLD6004Button *auto_interference_button_{nullptr};
  HLKLD6004Button *clear_interference_button_{nullptr};
  HLKLD6004Button *reset_detection_button_{nullptr};
  HLKLD6004Button *clear_dwell_button_{nullptr};
  HLKLD6004Button *refresh_config_button_{nullptr};
#endif
};

// --- Platform helper classes ---

#ifdef USE_SELECT
class HLKLD6004Select : public select::Select, public Parented<HLKLD6004Component> {
 public:
  void set_select_type(const std::string &type) { this->select_type_ = type; }

 protected:
  void control(const std::string &value) override;
  std::string select_type_;
};
#endif

#ifdef USE_NUMBER
class HLKLD6004Number : public number::Number, public Parented<HLKLD6004Component> {
 public:
  void set_number_type(const std::string &type) { this->number_type_ = type; }

 protected:
  void control(float value) override;
  std::string number_type_;
};
#endif

#ifdef USE_SWITCH
class HLKLD6004Switch : public switch_::Switch, public Parented<HLKLD6004Component> {
 public:
  void set_switch_type(const std::string &type) { this->switch_type_ = type; }

 protected:
  void write_state(bool state) override;
  std::string switch_type_;
};
#endif

#ifdef USE_BUTTON
class HLKLD6004Button : public button::Button, public Parented<HLKLD6004Component> {
 public:
  void set_button_type(const std::string &type) { this->button_type_ = type; }

 protected:
  void press_action() override;
  std::string button_type_;
};
#endif

}  // namespace hlk_ld6004
}  // namespace esphome
