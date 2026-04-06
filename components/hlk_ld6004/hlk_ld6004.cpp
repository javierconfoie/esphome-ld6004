#include "hlk_ld6004.h"
#include "esphome/core/log.h"

namespace esphome {
namespace hlk_ld6004 {

static const char *const TAG = "hlk_ld6004";

// =====================================================================
// Setup / Loop / Dump
// =====================================================================

void HLKLD6004Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up HLK-LD6004...");
  this->parser_state_ = WAIT_SOF;
  this->header_idx_ = 0;
  this->data_idx_ = 0;

  // Immediately activate the radar — it may be in sleep/disabled mode.
  // The GUI tool sends a "Start" command; we use Normal Mode (0x17).
  ESP_LOGI(TAG, "Sending initial activation: Normal Mode (0x17)");
  this->send_control(0x17);  // Normal detection mode
  this->send_control(0x08);  // Enable target display
  this->request_firmware_version();  // Query firmware (0xFFFF)
  this->last_init_attempt_ = millis();
}

void HLKLD6004Component::loop() {
  // Read all available bytes
  uint8_t buf[64];
  while (this->available() > 0) {
    size_t to_read = std::min((size_t) this->available(), sizeof(buf));
    this->read_array(buf, to_read);
    for (size_t i = 0; i < to_read; i++) {
      this->handle_byte_(buf[i]);
    }
  }

  // Retry activation every 5s until we receive a data frame (not just 0x0100)
  if (!this->data_frames_received_ && (millis() - this->last_init_attempt_ > 5000)) {
    this->last_init_attempt_ = millis();
    this->init_retries_++;
    ESP_LOGD(TAG, "No data frames yet, retrying activation (attempt %u)...", this->init_retries_);
    this->send_control(0x17);  // Normal detection mode
    this->send_control(0x08);  // Enable target display
    this->request_firmware_version();
  }

  // Request config once after first real data frame — staggered, one query per 200ms
  if (!this->initial_config_requested_ && this->data_frames_received_) {
    this->initial_config_requested_ = true;
    this->config_query_idx_ = 0;
    this->last_config_query_time_ = millis();
    ESP_LOGI(TAG, "Data frames flowing, querying config...");
  }

  // Staggered config queries — one every 1000ms
  if (this->config_query_idx_ < CONFIG_QUERY_COUNT &&
      (millis() - this->last_config_query_time_ >= 1000)) {
    this->send_config_query_(this->config_query_idx_);
    this->config_query_idx_++;
    this->last_config_query_time_ = millis();
  }

  // Retry all config queries if no config responses after 10s
  if (this->initial_config_requested_ && !this->config_received_ &&
      this->config_query_idx_ >= CONFIG_QUERY_COUNT &&
      (millis() - this->last_config_query_time_ > 10000)) {
    ESP_LOGI(TAG, "No config responses, retrying...");
    this->config_query_idx_ = 0;
    this->last_config_query_time_ = millis();
  }
}

void HLKLD6004Component::dump_config() {
  ESP_LOGCONFIG(TAG, "HLK-LD6004:");
  ESP_LOGCONFIG(TAG, "  Frames received: %u", this->frames_received_);
  ESP_LOGCONFIG(TAG, "  Checksum errors: %u", this->checksum_errors_);
  if (this->fw_major_ > 0 || this->fw_sub_ > 0) {
    ESP_LOGCONFIG(TAG, "  Firmware: %u.%u.%u (project %u)", this->fw_major_, this->fw_sub_,
                  this->fw_modified_, this->fw_project_);
  }
}

// =====================================================================
// TinyFrame parser
// =====================================================================

void HLKLD6004Component::handle_byte_(uint8_t byte) {
  switch (this->parser_state_) {
    case WAIT_SOF:
      if (byte == TF_SOF) {
        this->header_buf_[0] = byte;
        this->header_idx_ = 1;
        this->parser_state_ = READ_HEADER;
      }
      break;

    case READ_HEADER:
      this->header_buf_[this->header_idx_++] = byte;
      if (this->header_idx_ == TF_HEADER_SIZE) {
        uint8_t expected = calc_checksum_(this->header_buf_, 7);
        if (this->header_buf_[7] != expected) {
          this->checksum_errors_++;
          ESP_LOGW(TAG, "HDR CKSUM FAIL: got=0x%02X exp=0x%02X bytes=[%02X %02X %02X %02X %02X %02X %02X %02X]",
                   this->header_buf_[7], expected,
                   this->header_buf_[0], this->header_buf_[1], this->header_buf_[2],
                   this->header_buf_[3], this->header_buf_[4], this->header_buf_[5],
                   this->header_buf_[6], this->header_buf_[7]);
          this->parser_state_ = WAIT_SOF;
          break;
        }
        this->data_len_ = ((uint16_t) this->header_buf_[3] << 8) | this->header_buf_[4];
        this->frame_type_ = ((uint16_t) this->header_buf_[5] << 8) | this->header_buf_[6];

        if (this->data_len_ == 0) {
          // Zero-length frame (ACK) — radar does NOT send DATA_CKSUM for LEN=0
          this->process_frame_(this->frame_type_, this->data_buf_, 0);
          this->frames_received_++;
          this->last_update_time_ = millis();
          this->parser_state_ = WAIT_SOF;
        } else if (this->data_len_ > TF_MAX_DATA) {
          ESP_LOGD(TAG, "Oversized frame: type=0x%04X len=%u (max=%u)", this->frame_type_, this->data_len_, TF_MAX_DATA);
          this->parser_state_ = WAIT_SOF;
        } else {
          this->data_idx_ = 0;
          this->parser_state_ = READ_DATA;
        }
      }
      break;

    case READ_DATA:
      this->data_buf_[this->data_idx_++] = byte;
      if (this->data_idx_ == this->data_len_) {
        this->parser_state_ = READ_DATA_CKSUM;
      }
      break;

    case READ_DATA_CKSUM: {
      uint8_t expected;
      if (this->data_len_ == 0) {
        expected = 0xFF;
      } else {
        expected = calc_checksum_(this->data_buf_, this->data_len_);
      }
      if (byte == expected) {
        this->process_frame_(this->frame_type_, this->data_buf_, this->data_len_);
        this->frames_received_++;
        this->last_update_time_ = millis();
      } else {
        this->checksum_errors_++;
        ESP_LOGW(TAG, "DATA CKSUM FAIL: type=0x%04X len=%u got=0x%02X exp=0x%02X",
                 this->frame_type_, this->data_len_, byte, expected);
      }
      this->parser_state_ = WAIT_SOF;
      break;
    }
  }
}

// =====================================================================
// Frame processing
// =====================================================================

void HLKLD6004Component::process_frame_(uint16_t type, const uint8_t *data, uint16_t len) {
  ESP_LOGD(TAG, "RX: type=0x%04X len=%u", type, len);

  switch (type) {
    case TYPE_COMBINED_REPORT:
      // Product ID string — not real sensor data, ignore
      if (!this->combined_report_logged_) {
        this->combined_report_logged_ = true;
        // Log as ASCII for identification
        char id_str[64]{};
        uint16_t copy_len = len < 63 ? len : 63;
        memcpy(id_str, data, copy_len);
        // Strip trailing CR/LF
        for (int i = copy_len - 1; i >= 0 && (id_str[i] == '\r' || id_str[i] == '\n'); i--)
          id_str[i] = '\0';
        ESP_LOGI(TAG, "Radar identified: %s", id_str);
      }
      break;

    case TYPE_PRESENCE_STATE:
      this->data_frames_received_ = true;
      if (len >= 16) {
        for (int i = 0; i < MAX_ZONES; i++) {
          this->presence_[i] = read_u32_le_(&data[i * 4]);
        }
        this->publish_presence_();
      }
      break;

    case TYPE_TARGET_LOCATION:
    case TYPE_POINT_CLOUD:
      this->data_frames_received_ = true;
      if (len >= 4) {
        int count = (int) read_u32_le_(data);
        if (count < 0) count = 0;
        if (count > MAX_TARGETS) count = MAX_TARGETS;
        if (len < (uint16_t)(4 + count * 20)) break;

        for (int i = 0; i < count; i++) {
          const uint8_t *rec = &data[4 + i * 20];
          this->targets_[i].x = read_float_(&rec[0]);
          this->targets_[i].y = read_float_(&rec[4]);
          this->targets_[i].z = read_float_(&rec[8]);
          this->targets_[i].doppler = read_i32_le_(&rec[12]);
          this->targets_[i].id = read_i32_le_(&rec[16]);
        }
        // Clear unused targets
        for (int i = count; i < MAX_TARGETS; i++) {
          this->targets_[i] = {};
        }
        this->target_count_ = count;
        this->publish_targets_();
      }
      break;

    case TYPE_INTERFERENCE:
      if (len >= 96) {
        for (int i = 0; i < MAX_ZONES; i++) {
          const uint8_t *p = &data[i * 24];
          this->interference_zones_[i] = {read_float_(&p[0]), read_float_(&p[4]),
                                          read_float_(&p[8]), read_float_(&p[12]),
                                          read_float_(&p[16]), read_float_(&p[20])};
        }
      }
      break;

    case TYPE_DETECTION_AREA:
      if (len >= 96) {
        for (int i = 0; i < MAX_ZONES; i++) {
          const uint8_t *p = &data[i * 24];
          this->detection_zones_[i] = {read_float_(&p[0]), read_float_(&p[4]),
                                       read_float_(&p[8]), read_float_(&p[12]),
                                       read_float_(&p[16]), read_float_(&p[20])};
        }
      }
      break;

    case TYPE_DWELL_AREA:
      if (len >= 96) {
        for (int i = 0; i < MAX_ZONES; i++) {
          const uint8_t *p = &data[i * 24];
          this->dwell_zones_[i] = {read_float_(&p[0]), read_float_(&p[4]),
                                   read_float_(&p[8]), read_float_(&p[12]),
                                   read_float_(&p[16]), read_float_(&p[20])};
        }
      }
      break;

    case TYPE_DELAY_TIME:
      this->config_received_ = true;
      if (len >= 4) {
        this->delay_time_ = read_u32_le_(data);
#ifdef USE_NUMBER
        if (this->delay_time_number_ != nullptr)
          this->delay_time_number_->publish_state(this->delay_time_);
#endif
      }
      break;

    case TYPE_SENSITIVITY:
      if (len >= 1) {
        this->sensitivity_ = data[0];
#ifdef USE_SELECT
        if (this->sensitivity_select_ != nullptr) {
          static const char *const vals[] = {"Low", "Medium", "High"};
          if (data[0] < 3)
            this->sensitivity_select_->publish_state(vals[data[0]]);
        }
#endif
      }
      break;

    case TYPE_TRIGGER_SPEED:
      if (len >= 1) {
        this->trigger_speed_ = data[0];
#ifdef USE_SELECT
        if (this->trigger_speed_select_ != nullptr) {
          static const char *const vals[] = {"Slow", "Medium", "Fast"};
          if (data[0] < 3)
            this->trigger_speed_select_->publish_state(vals[data[0]]);
        }
#endif
      }
      break;

    case TYPE_Z_RANGE:
      if (len >= 8) {
        this->z_range_min_ = read_float_(&data[0]);
        this->z_range_max_ = read_float_(&data[4]);
#ifdef USE_NUMBER
        if (this->z_range_min_number_ != nullptr)
          this->z_range_min_number_->publish_state(this->z_range_min_);
        if (this->z_range_max_number_ != nullptr)
          this->z_range_max_number_->publish_state(this->z_range_max_);
#endif
      }
      break;

    case TYPE_INSTALL_METHOD:
      if (len >= 1) {
        this->install_method_ = data[0];
#ifdef USE_SELECT
        if (this->install_method_select_ != nullptr) {
          static const char *const vals[] = {"Top", "Side"};
          if (data[0] < 2)
            this->install_method_select_->publish_state(vals[data[0]]);
        }
#endif
      }
      break;

    case TYPE_OPERATING_MODE:
      if (len >= 1) {
        this->operating_mode_ = data[0];
#ifdef USE_SELECT
        if (this->operating_mode_select_ != nullptr) {
          static const char *const vals[] = {"Low Power", "Normal", "Off P20 High", "Off P20 Low", "High Reflectivity"};
          if (data[0] < 5)
            this->operating_mode_select_->publish_state(vals[data[0]]);
        }
#endif
      }
      break;

    case TYPE_SLEEP_TIME:
      if (len >= 4) {
        this->sleep_time_ = read_u32_le_(data);
#ifdef USE_NUMBER
        if (this->sleep_time_number_ != nullptr)
          this->sleep_time_number_->publish_state(this->sleep_time_);
#endif
      }
      break;

    case TYPE_GPIO_STATUS:
      if (len >= 1) {
        this->p20_status_ = data[0];
#ifdef USE_SELECT
        if (this->p20_mode_select_ != nullptr) {
          static const char *const vals[] = {"High Present", "Low Present", "Always Low",
                                             "Always High", "Pulse Low", "Pulse High"};
          if (data[0] < 6)
            this->p20_mode_select_->publish_state(vals[data[0]]);
        }
#endif
      }
      break;

    case TYPE_DWELL_LIFECYCLE:
      if (len >= 4) {
        this->dwell_lifecycle_ = read_u32_le_(data);
#ifdef USE_NUMBER
        if (this->dwell_lifecycle_number_ != nullptr)
          this->dwell_lifecycle_number_->publish_state(this->dwell_lifecycle_);
#endif
      }
      break;

    case TYPE_OUTPUT_INTERVAL:
      if (len >= 4) {
        this->output_interval_ = read_u32_le_(data);
#ifdef USE_NUMBER
        if (this->output_interval_number_ != nullptr)
          this->output_interval_number_->publish_state(this->output_interval_);
#endif
      }
      break;

    case TYPE_FIRMWARE:
      this->data_frames_received_ = true;
      if (len >= 4) {
        this->fw_project_ = data[0];
        this->fw_major_ = data[1];
        this->fw_sub_ = data[2];
        this->fw_modified_ = data[3];
#ifdef USE_TEXT_SENSOR
        if (this->firmware_text_sensor_ != nullptr) {
          char buf[32];
          snprintf(buf, sizeof(buf), "%u.%u.%u", this->fw_major_, this->fw_sub_, this->fw_modified_);
          this->firmware_text_sensor_->publish_state(buf);
        }
#endif
        ESP_LOGI(TAG, "Firmware: %u.%u.%u (project %u)", this->fw_major_, this->fw_sub_,
                 this->fw_modified_, this->fw_project_);
      }
      break;

    default:
      ESP_LOGD(TAG, "Unhandled frame type=0x%04X len=%u", type, len);
      break;
  }
}

// =====================================================================
// Publishing
// =====================================================================

void HLKLD6004Component::publish_presence_() {
#ifdef USE_BINARY_SENSOR
  bool any_presence = false;
  for (int i = 0; i < MAX_ZONES; i++) {
    bool occupied = this->presence_[i] != 0;
    if (occupied) any_presence = true;
    if (this->zone_presence_binary_sensors_[i] != nullptr)
      this->zone_presence_binary_sensors_[i]->publish_state(occupied);
  }
  if (this->presence_binary_sensor_ != nullptr)
    this->presence_binary_sensor_->publish_state(any_presence);
#endif
}

void HLKLD6004Component::publish_targets_() {
#ifdef USE_SENSOR
  if (this->target_count_sensor_ != nullptr)
    this->target_count_sensor_->publish_state(this->target_count_);

  for (int i = 0; i < MAX_TARGETS; i++) {
    if (i < this->target_count_) {
      if (this->target_x_sensors_[i] != nullptr)
        this->target_x_sensors_[i]->publish_state(this->targets_[i].x);
      if (this->target_y_sensors_[i] != nullptr)
        this->target_y_sensors_[i]->publish_state(this->targets_[i].y);
      if (this->target_z_sensors_[i] != nullptr)
        this->target_z_sensors_[i]->publish_state(this->targets_[i].z);
      if (this->target_doppler_sensors_[i] != nullptr)
        this->target_doppler_sensors_[i]->publish_state(this->targets_[i].doppler);
      if (this->target_id_sensors_[i] != nullptr)
        this->target_id_sensors_[i]->publish_state(this->targets_[i].id);
    } else {
      // Clear sensors for absent targets
      if (this->target_x_sensors_[i] != nullptr)
        this->target_x_sensors_[i]->publish_state(NAN);
      if (this->target_y_sensors_[i] != nullptr)
        this->target_y_sensors_[i]->publish_state(NAN);
      if (this->target_z_sensors_[i] != nullptr)
        this->target_z_sensors_[i]->publish_state(NAN);
      if (this->target_doppler_sensors_[i] != nullptr)
        this->target_doppler_sensors_[i]->publish_state(NAN);
      if (this->target_id_sensors_[i] != nullptr)
        this->target_id_sensors_[i]->publish_state(NAN);
    }
  }
#endif
}

// =====================================================================
// Config queries
// =====================================================================

void HLKLD6004Component::query_all_config_() {
  ESP_LOGI(TAG, "Requesting sensor configuration...");
  this->config_query_idx_ = 0;
  this->last_config_query_time_ = millis();
}

void HLKLD6004Component::send_config_query_(uint8_t idx) {
  static const uint16_t queries[] = {
    0xFFFF,  // firmware (special — uses request_firmware_version)
    0x0D,    // sensitivity
    0x02,    // zones (this one works!)
    0x11,    // trigger speed
    0x15,    // install method
    0x18,    // operating mode
    0x05,    // delay time
    0x19,    // sleep time
    0x12,    // z range
    0x1F,    // p20 status
    0x26,    // dwell lifecycle
    0x27,    // output interval
  };
  if (idx >= CONFIG_QUERY_COUNT) return;
  ESP_LOGD(TAG, "Config query %u/%u: cmd=0x%04X", idx + 1, (uint8_t)CONFIG_QUERY_COUNT, queries[idx]);
  if (queries[idx] == 0xFFFF) {
    this->request_firmware_version();
  } else {
    this->send_control(queries[idx]);
  }
}

// =====================================================================
// Command sending
// =====================================================================

void HLKLD6004Component::send_control(uint32_t subcmd) {
  uint8_t data[4];
  data[0] = subcmd & 0xFF;
  data[1] = (subcmd >> 8) & 0xFF;
  data[2] = (subcmd >> 16) & 0xFF;
  data[3] = (subcmd >> 24) & 0xFF;
  this->send_frame_(TYPE_CONTROL, data, 4);
}

void HLKLD6004Component::send_set_delay(uint32_t seconds) {
  uint8_t data[4];
  data[0] = seconds & 0xFF;
  data[1] = (seconds >> 8) & 0xFF;
  data[2] = (seconds >> 16) & 0xFF;
  data[3] = (seconds >> 24) & 0xFF;
  this->send_frame_(TYPE_SET_DELAY, data, 4);
}

void HLKLD6004Component::send_set_sleep_time(uint32_t ms) {
  uint8_t data[4];
  data[0] = ms & 0xFF;
  data[1] = (ms >> 8) & 0xFF;
  data[2] = (ms >> 16) & 0xFF;
  data[3] = (ms >> 24) & 0xFF;
  this->send_frame_(TYPE_SET_SLEEP_TIME, data, 4);
}

void HLKLD6004Component::send_set_output_interval(uint32_t interval) {
  uint8_t data[4];
  data[0] = interval & 0xFF;
  data[1] = (interval >> 8) & 0xFF;
  data[2] = (interval >> 16) & 0xFF;
  data[3] = (interval >> 24) & 0xFF;
  this->send_frame_(TYPE_SET_OUTPUT_INTV, data, 4);
}

void HLKLD6004Component::send_set_dwell_lifecycle(uint32_t lifecycle) {
  uint8_t data[4];
  data[0] = lifecycle & 0xFF;
  data[1] = (lifecycle >> 8) & 0xFF;
  data[2] = (lifecycle >> 16) & 0xFF;
  data[3] = (lifecycle >> 24) & 0xFF;
  this->send_frame_(TYPE_SET_DWELL_LIFE, data, 4);
}

void HLKLD6004Component::send_set_z_range(float z_min, float z_max) {
  uint8_t data[8];
  uint32_t v_min, v_max;
  memcpy(&v_min, &z_min, 4);
  memcpy(&v_max, &z_max, 4);
  data[0] = v_min & 0xFF;       data[1] = (v_min >> 8) & 0xFF;
  data[2] = (v_min >> 16) & 0xFF; data[3] = (v_min >> 24) & 0xFF;
  data[4] = v_max & 0xFF;       data[5] = (v_max >> 8) & 0xFF;
  data[6] = (v_max >> 16) & 0xFF; data[7] = (v_max >> 24) & 0xFF;
  this->send_frame_(TYPE_SET_Z_RANGE, data, 8);
}

void HLKLD6004Component::send_set_zone(uint8_t area_id, float x_min, float x_max,
                                       float y_min, float y_max, float z_min, float z_max) {
  uint8_t data[28];
  data[0] = area_id; data[1] = 0; data[2] = 0; data[3] = 0;
  float vals[6] = {x_min, x_max, y_min, y_max, z_min, z_max};
  for (int i = 0; i < 6; i++) {
    uint32_t v;
    memcpy(&v, &vals[i], 4);
    data[4 + i * 4] = v & 0xFF;
    data[4 + i * 4 + 1] = (v >> 8) & 0xFF;
    data[4 + i * 4 + 2] = (v >> 16) & 0xFF;
    data[4 + i * 4 + 3] = (v >> 24) & 0xFF;
  }
  this->send_frame_(TYPE_SET_AREA, data, 28);
}

void HLKLD6004Component::send_set_baud_rate(uint8_t rate) {
  this->send_frame_(TYPE_SET_BAUD, &rate, 1);
}

void HLKLD6004Component::request_firmware_version() {
  this->send_frame_(TYPE_FIRMWARE, nullptr, 0);
}

// =====================================================================
// Frame encoding
// =====================================================================

void HLKLD6004Component::send_frame_(uint16_t type, const uint8_t *data, uint16_t len) {
  this->send_id_++;
  uint8_t hdr[8];
  hdr[0] = TF_SOF;
  hdr[1] = (this->send_id_ >> 8) & 0xFF;
  hdr[2] = this->send_id_ & 0xFF;
  hdr[3] = (len >> 8) & 0xFF;
  hdr[4] = len & 0xFF;
  hdr[5] = (type >> 8) & 0xFF;
  hdr[6] = type & 0xFF;
  hdr[7] = calc_checksum_(hdr, 7);

  this->write_array(hdr, 8);

  if (len > 0 && data != nullptr) {
    this->write_array(data, len);
    uint8_t dcksum = calc_checksum_(data, len);
    this->write_byte(dcksum);
  }
  // No DATA_CKSUM for zero-length frames — radar expects only 8-byte header

  this->flush();

  // Log TX with hex dump of full frame for verification
  {
    uint8_t frame[64];
    uint16_t flen = 8 + (len > 0 ? len + 1 : 0);
    if (flen > sizeof(frame)) flen = sizeof(frame);
    memcpy(frame, hdr, 8);
    if (len > 0 && data != nullptr) {
      uint16_t copy = (len + 1 <= sizeof(frame) - 8) ? len + 1 : sizeof(frame) - 8;
      memcpy(&frame[8], data, copy > 0 ? copy - 1 : 0);
      frame[8 + copy - 1] = calc_checksum_(data, len);
    }
    char hex[200];
    uint16_t dump = flen < 30 ? flen : 30;
    for (uint16_t i = 0; i < dump; i++) snprintf(&hex[i*3], 4, "%02X ", frame[i]);
    hex[dump*3] = '\0';
    ESP_LOGD(TAG, "TX: type=0x%04X len=%u id=%u bytes=[%s]", type, len, this->send_id_, hex);
  }
}

// =====================================================================
// Helpers
// =====================================================================

uint8_t HLKLD6004Component::calc_checksum_(const uint8_t *data, uint16_t len) {
  uint8_t result = 0;
  for (uint16_t i = 0; i < len; i++) {
    result ^= data[i];
  }
  return ~result;
}

float HLKLD6004Component::read_float_(const uint8_t *p) {
  uint32_t v = read_u32_le_(p);
  float f;
  memcpy(&f, &v, 4);
  return f;
}

uint32_t HLKLD6004Component::read_u32_le_(const uint8_t *p) {
  return (uint32_t) p[0] | ((uint32_t) p[1] << 8) |
         ((uint32_t) p[2] << 16) | ((uint32_t) p[3] << 24);
}

int32_t HLKLD6004Component::read_i32_le_(const uint8_t *p) {
  return (int32_t) read_u32_le_(p);
}

// =====================================================================
// Platform control implementations
// =====================================================================

#ifdef USE_SELECT
void HLKLD6004Select::control(const std::string &value) {
  auto *hub = this->parent_;
  if (this->select_type_ == "sensitivity") {
    if (value == "Low") hub->send_control(0x0A);
    else if (value == "Medium") hub->send_control(0x0B);
    else if (value == "High") hub->send_control(0x0C);
  } else if (this->select_type_ == "trigger_speed") {
    if (value == "Slow") hub->send_control(0x0E);
    else if (value == "Medium") hub->send_control(0x0F);
    else if (value == "Fast") hub->send_control(0x10);
  } else if (this->select_type_ == "install_method") {
    if (value == "Top") hub->send_control(0x13);
    else if (value == "Side") hub->send_control(0x14);
  } else if (this->select_type_ == "operating_mode") {
    if (value == "Low Power") hub->send_control(0x16);
    else if (value == "Normal") hub->send_control(0x17);
    else if (value == "Off P20 High") hub->send_control(0x1B);
    else if (value == "Off P20 Low") hub->send_control(0x1C);
    else if (value == "High Reflectivity") hub->send_control(0x24);
  } else if (this->select_type_ == "p20_mode") {
    if (value == "High Present") hub->send_control(0x1D);
    else if (value == "Low Present") hub->send_control(0x1E);
    else if (value == "Always Low") hub->send_control(0x20);
    else if (value == "Always High") hub->send_control(0x21);
    else if (value == "Pulse Low") hub->send_control(0x22);
    else if (value == "Pulse High") hub->send_control(0x23);
  }
  this->publish_state(value);
}
#endif

#ifdef USE_NUMBER
void HLKLD6004Number::control(float value) {
  auto *hub = this->parent_;
  if (this->number_type_ == "delay_time") {
    hub->send_set_delay((uint32_t) value);
  } else if (this->number_type_ == "sleep_time") {
    hub->send_set_sleep_time((uint32_t) value);
  } else if (this->number_type_ == "output_interval") {
    hub->send_set_output_interval((uint32_t) value);
  } else if (this->number_type_ == "dwell_lifecycle") {
    hub->send_set_dwell_lifecycle((uint32_t) value);
  } else if (this->number_type_ == "z_range_min") {
    hub->send_set_z_range(value, hub->get_z_range_max());
  } else if (this->number_type_ == "z_range_max") {
    hub->send_set_z_range(hub->get_z_range_min(), value);
  }
  this->publish_state(value);
}
#endif

#ifdef USE_SWITCH
void HLKLD6004Switch::write_state(bool state) {
  auto *hub = this->parent_;
  if (this->switch_type_ == "point_cloud") {
    hub->send_control(state ? 0x06 : 0x07);
  } else if (this->switch_type_ == "target_display") {
    hub->send_control(state ? 0x08 : 0x09);
  }
  this->publish_state(state);
}
#endif

#ifdef USE_BUTTON
void HLKLD6004Button::press_action() {
  auto *hub = this->parent_;
  if (this->button_type_ == "reset_unmanned") hub->send_control(0x1A);
  else if (this->button_type_ == "auto_interference") hub->send_control(0x01);
  else if (this->button_type_ == "clear_interference") hub->send_control(0x03);
  else if (this->button_type_ == "reset_detection") hub->send_control(0x04);
  else if (this->button_type_ == "clear_dwell") hub->send_control(0x25);
  else if (this->button_type_ == "refresh_config") hub->query_all_config_();
}
#endif

}  // namespace hlk_ld6004
}  // namespace esphome
