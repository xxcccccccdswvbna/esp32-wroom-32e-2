#pragma once
#include "esphome/core/component.h"
#include <deque>
#include <string>
#include <vector>
#include <cstdint>

namespace esphome { namespace ble_tx {
class BLETx : public Component {
 public:
  void setup() override;
  void loop() override;
  void send_hex(const std::string &hex);
 private:
  enum class State : uint8_t { IDLE, CONFIGURING, ADVERTISING, STOPPING, GAP };
  struct Job { std::string command; std::string end; };
  State state_{State::IDLE};
  std::deque<Job> queue_;
  Job current_{};
  std::vector<uint8_t> data_;
  uint32_t deadline_{0};
  uint8_t phase_{0};
  uint8_t retries_{0};
  bool end_pending_{false};
  void pump_();
  void configure_();
  void start_();
  void stop_();
  void next_phase_();
  void fail_(const char *why);
  static std::vector<uint8_t> decode_(const std::string &hex);
};
} }
