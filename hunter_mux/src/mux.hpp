#ifndef MUX_HPP_
#define MUX_HPP_

#include <chrono>
#include <cstdint>

namespace hunter_mux {

using Clock = std::chrono::steady_clock;

constexpr int8_t mode_idle = -1;
constexpr int8_t mode_nav = 1;
constexpr int8_t mode_remote = 2;
constexpr int8_t source_none = -1;
constexpr int8_t source_nav = 0;
constexpr int8_t source_teleop = 1;
constexpr int8_t mux_flag_remote_without_teleop = 1;

struct Selection {
  int8_t source = source_none;
  bool nav_active = false;
  bool teleop_active = false;
  bool is_estop = false;
  int8_t mux_flag = 0;
};

class Mux {
 public:
  Mux(std::chrono::duration<double> cmd_vel_timeout, std::chrono::duration<double> teleop_timeout);

  void on_nav(Clock::time_point stamp) { last_nav_time_ = stamp; }
  void on_teleop(Clock::time_point stamp) { last_teleop_time_ = stamp; }
  void set_bridge_estop(bool is_estop) { bridge_estop_ = is_estop; }
  void set_cmd_mode(int8_t mode) { cmd_mode_ = mode; }
  void set_allow_remote(bool allow) { allow_remote_ = allow; }
  int8_t cmd_mode() const { return cmd_mode_; }
  bool allow_remote() const { return allow_remote_; }

  Selection select(Clock::time_point now) const;

 private:
  std::chrono::duration<double> cmd_vel_timeout_;
  std::chrono::duration<double> teleop_timeout_;
  bool allow_remote_ = false;
  bool bridge_estop_ = false;
  int8_t cmd_mode_ = mode_idle;
  Clock::time_point last_nav_time_{};
  Clock::time_point last_teleop_time_{};
};

}  // namespace hunter_mux

#endif  // MUX_HPP_
