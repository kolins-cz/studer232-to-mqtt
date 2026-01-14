#include "studer.h"
#include "studer_sensor.h"
#include "esphome/core/log.h"

extern "C" {
#include "scomlib/scom_data_link.h"
#include "scomlib/scom_property.h"
#include "scomlib/scomlib_extra.h"
}

namespace esphome {
namespace studer {

static const char *const TAG = "studer";

void StuderComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Studer component...");
}

void StuderComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Studer Component:");
  ESP_LOGCONFIG(TAG, "  Registered sensors: %d", this->sensors_.size());
  this->check_uart_settings(115200, 1, uart::UART_CONFIG_PARITY_EVEN, 8);
}

void StuderComponent::loop() {
  // Poll one sensor every 500ms
  uint32_t now = millis();
  if (now - this->last_poll_ < 500) {
    return;
  }
  
  if (this->sensors_.empty()) {
    return;
  }
  
  this->last_poll_ = now;
  this->poll_next_sensor_();
}

void StuderComponent::poll_next_sensor_() {
  if (this->current_sensor_index_ >= this->sensors_.size()) {
    this->current_sensor_index_ = 0;
  }
  
  StuderSensor *sensor = this->sensors_[this->current_sensor_index_];
  this->current_sensor_index_++;
  
  // Encode read command
  scomx_enc_result_t enc_result = scomx_encode_read_user_info_value(
      sensor->get_address(), 
      sensor->get_parameter()
  );
  
  if (enc_result.error != SCOM_ERROR_NO_ERROR) {
    ESP_LOGW(TAG, "Failed to encode command for sensor '%s': error %d", 
             sensor->get_name().c_str(), enc_result.error);
    return;
  }
  
  // Send command
  this->write_array(enc_result.data, enc_result.length);
  this->flush();
  
  ESP_LOGV(TAG, "Sent command for '%s' (addr=%d, param=%d)", 
           sensor->get_name().c_str(), sensor->get_address(), sensor->get_parameter());
  
  // Read response header
  uint8_t header_buf[SCOM_FRAME_HEADER_SIZE];
  if (!this->read_response_(header_buf, SCOM_FRAME_HEADER_SIZE, 2000)) {
    ESP_LOGW(TAG, "Timeout reading header for '%s'", sensor->get_name().c_str());
    return;
  }
  
  // Decode header
  scomx_header_dec_result_t header_result = scomx_decode_frame_header(
      (const char *)header_buf, 
      SCOM_FRAME_HEADER_SIZE
  );
  
  if (header_result.error != SCOM_ERROR_NO_ERROR) {
    ESP_LOGW(TAG, "Failed to decode header for '%s': error %d", 
             sensor->get_name().c_str(), header_result.error);
    return;
  }
  
  // Read response data
  uint8_t data_buf[128];
  if (header_result.length_to_read > sizeof(data_buf)) {
    ESP_LOGE(TAG, "Response too large for '%s': %d bytes", 
             sensor->get_name().c_str(), header_result.length_to_read);
    return;
  }
  
  if (!this->read_response_(data_buf, header_result.length_to_read, 2000)) {
    ESP_LOGW(TAG, "Timeout reading data for '%s'", sensor->get_name().c_str());
    return;
  }
  
  // Decode response
  scomx_dec_result_t dec_result = scomx_decode_frame(
      (const char *)data_buf, 
      header_result.length_to_read
  );
  
  if (dec_result.error != SCOM_ERROR_NO_ERROR) {
    ESP_LOGW(TAG, "Failed to decode response for '%s': error %d", 
             sensor->get_name().c_str(), dec_result.error);
    return;
  }
  
  // Extract float value and publish
  float value = scomx_result_float(dec_result);
  sensor->publish_value(value);
  
  ESP_LOGD(TAG, "'%s': %.3f", sensor->get_name().c_str(), value * sensor->get_multiply());
}

bool StuderComponent::read_response_(uint8_t *buffer, size_t expected_len, uint32_t timeout_ms) {
  uint32_t start = millis();
  size_t bytes_read = 0;
  
  while (bytes_read < expected_len) {
    if (millis() - start > timeout_ms) {
      return false;
    }
    
    if (this->available()) {
      buffer[bytes_read++] = this->read();
    } else {
      delay(1);
    }
  }
  
  return true;
}

}  // namespace studer
}  // namespace esphome
