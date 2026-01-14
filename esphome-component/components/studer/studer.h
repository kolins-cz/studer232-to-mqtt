#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include <vector>

namespace esphome {
namespace studer {

// Forward declaration
class StuderSensor;

class StuderComponent : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void register_sensor(StuderSensor *sensor) { this->sensors_.push_back(sensor); }

 protected:
  std::vector<StuderSensor *> sensors_;
  uint32_t last_poll_{0};
  size_t current_sensor_index_{0};
  
  bool read_response_(uint8_t *buffer, size_t expected_len, uint32_t timeout_ms);
  void poll_next_sensor_();
};

}  // namespace studer
}  // namespace esphome
