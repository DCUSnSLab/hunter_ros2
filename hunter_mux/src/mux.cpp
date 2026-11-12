#include "mux.hpp"

namespace hunter_mux {

Mux::Mux(std::chrono::duration<double> cmd_vel_timeout,
         std::chrono::duration<double> teleop_timeout)
    : cmd_vel_timeout_(cmd_vel_timeout), teleop_timeout_(teleop_timeout) {}

void Mux::reset_estop(Clock::time_point now) {
  latched_ = false;
  select(now);
}

Selection Mux::select(Clock::time_point now) {
  Selection s;
  s.nav_active = now - last_nav_time_ < cmd_vel_timeout_;
  s.teleop_active = now - last_teleop_time_ < teleop_timeout_;

  const bool remote_mode_requested = cmd_mode_ == mode_remote;
  if (allow_remote_ && remote_mode_requested && s.nav_active && !s.teleop_active) {
    s.mux_flag = mux_flag_remote_without_teleop;
  }
  latched_ = latched_ || bridge_estop_ || s.mux_flag != 0;
  s.is_estop = latched_;

  if (s.is_estop) {
    return s;
  }
  if (!allow_remote_ || cmd_mode_ == mode_nav) {
    if (s.nav_active) {
      s.source = source_nav;
    }
  } else if (remote_mode_requested && s.teleop_active) {
    s.source = source_teleop;
  }
  return s;
}

}  // namespace hunter_mux
