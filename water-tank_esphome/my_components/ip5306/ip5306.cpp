#include "ip5306.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace ip5306 {

static const char *const TAG = "ip5306";

static const uint8_t IP5306_DEFAULT_SETUP_0 = 0x3A;  // set boost, charger, power on load enabled
static const uint8_t IP5306_DEFAULT_SETUP_1 = 0x05;  // set boost after vin, low bat shutdown enabled

static const uint8_t IP5306_REG_SYS_CTL0 = 0x00;  // initialize
static const uint8_t IP5306_REG_SYS_CTL1 = 0x01;
static const uint8_t IP5306_REG_READ0 = 0x70;  // charge en
static const uint8_t IP5306_REG_READ1 = 0x71;  // charge full
static const uint8_t IP5306_REG_LEVEL = 0x78;  // bat level


float IP5306::get_setup_priority() const { return setup_priority::IO; }

void IP5306::setup() {
  ESP_LOGD(TAG, "Setting up ip5306...");
  if (this->is_failed()) {
    return;
  }
  if (this->write_register(IP5306_REG_SYS_CTL0, (uint8_t *) &IP5306_DEFAULT_SETUP_0, 1) != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "setup of SYS_CTL0 failed");
    this->mark_failed();
    return;
  }
  if (this->write_register(IP5306_REG_SYS_CTL1, (uint8_t *) &IP5306_DEFAULT_SETUP_1, 1) != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "setup of SYS_CTL1 failed");
    this->mark_failed();
  }
}

void IP5306::dump_config() {
  ESP_LOGCONFIG(TAG, "IP5306:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with IP5306 failed!");
  }
  LOG_SENSOR("  ", "Battery Level", this->battery_level_);
  LOG_BINARY_SENSOR("  ", "Charger Connected", this->charger_connected_);
  LOG_BINARY_SENSOR("  ", "Charge Full", this->charge_full_);
}

void IP5306::loop() {
  if (this->is_failed()) {
    return;
  }
  uint8_t data[2];
  if (this->battery_level_ != nullptr) {
    if (this->read_register(IP5306_REG_LEVEL, data, 1) != i2c::ERROR_OK) {
      ESP_LOGE(TAG, "unable to read level");
      this->mark_failed();
      return;
    }
    float value = 0;
    switch (data[0] & 0xF0) {
      case 0xE0: value = 25; break;
      case 0xC0: value = 50; break;
      case 0x80: value = 75; break;
      case 0x00: value = 100; break;
    }
    if (!this->battery_level_->has_state() || (this->battery_level_->state != value))
      this->battery_level_->publish_state(value);
  }
  if (this->read_register(IP5306_REG_READ0, data, 2) != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "unable to read status");
    this->mark_failed();
    return;
  }
  if (this->charger_connected_ != nullptr)
    this->charger_connected_->publish_state(data[0] & 0x08);
  if (this->charge_full_ != nullptr)
    this->charge_full_->publish_state(data[1] & 0x08);
}

}  // namespace ip5306
}  // namespace esphome

