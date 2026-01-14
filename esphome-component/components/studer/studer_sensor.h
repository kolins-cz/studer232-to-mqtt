#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace studer {

// Forward declaration
class StuderComponent;

class StuderSensor : public sensor::Sensor, public Component {
 public:
  void set_parent(StuderComponent *parent) { this->parent_ = parent; }
  void set_address(uint16_t address) { this->address_ = address; }
  void set_parameter(uint16_t parameter) { this->parameter_ = parameter; }
  void set_multiply(float multiply) { this->multiply_ = multiply; }
  
  uint16_t get_address() const { return this->address_; }
  uint16_t get_parameter() const { return this->parameter_; }
  float get_multiply() const { return this->multiply_; }
  
  void publish_value(float raw_value) {
    float value = raw_value * this->multiply_;
    this->publish_state(value);
  }

 protected:
  StuderComponent *parent_{nullptr};
  uint16_t address_{0};
  uint16_t parameter_{0};
  float multiply_{1.0};
};

}  // namespace studer
}  // namespace esphome
