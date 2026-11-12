#include <gtest/gtest.h>

#include "mux.hpp"

using hunter_mux::Clock;
using hunter_mux::Mux;
using hunter_mux::Selection;
using std::chrono::milliseconds;

namespace {

const Clock::time_point t0 = Clock::time_point{} + std::chrono::hours(1);

Mux make_mux(bool allow_remote, int8_t mode) {
  Mux mux(milliseconds(500), milliseconds(500));
  mux.set_allow_remote(allow_remote);
  mux.set_cmd_mode(mode);
  return mux;
}

}  // namespace

TEST(Mux, NothingIsSelectedWithoutInput) {
  const Selection s = make_mux(false, hunter_mux::mode_idle).select(t0);
  EXPECT_EQ(s.source, hunter_mux::source_none);
  EXPECT_FALSE(s.nav_active);
  EXPECT_FALSE(s.teleop_active);
  EXPECT_FALSE(s.is_estop);
}

TEST(Mux, NavIsSelectedWhenRemoteIsNotAllowed) {
  Mux mux = make_mux(false, hunter_mux::mode_remote);
  mux.on_nav(t0);
  mux.on_teleop(t0);
  EXPECT_EQ(mux.select(t0 + milliseconds(100)).source, hunter_mux::source_nav);
}

TEST(Mux, NavTimesOut) {
  Mux mux = make_mux(false, hunter_mux::mode_idle);
  mux.on_nav(t0);
  EXPECT_TRUE(mux.select(t0 + milliseconds(499)).nav_active);
  const Selection s = mux.select(t0 + milliseconds(500));
  EXPECT_FALSE(s.nav_active);
  EXPECT_EQ(s.source, hunter_mux::source_none);
}

TEST(Mux, TeleopIsSelectedInRemoteMode) {
  Mux mux = make_mux(true, hunter_mux::mode_remote);
  mux.on_nav(t0);
  mux.on_teleop(t0);
  const Selection s = mux.select(t0);
  EXPECT_EQ(s.source, hunter_mux::source_teleop);
  EXPECT_FALSE(s.is_estop);
}

TEST(Mux, NavInNavModeWhenRemoteIsAllowed) {
  Mux mux = make_mux(true, hunter_mux::mode_nav);
  mux.on_nav(t0);
  mux.on_teleop(t0);
  EXPECT_EQ(mux.select(t0).source, hunter_mux::source_nav);
}

TEST(Mux, IdleModeSelectsNothingWhenRemoteIsAllowed) {
  Mux mux = make_mux(true, hunter_mux::mode_idle);
  mux.on_nav(t0);
  mux.on_teleop(t0);
  EXPECT_EQ(mux.select(t0).source, hunter_mux::source_none);
}

TEST(Mux, RemoteModeWithNavButNoTeleopStops) {
  Mux mux = make_mux(true, hunter_mux::mode_remote);
  mux.on_nav(t0);
  const Selection s = mux.select(t0);
  EXPECT_EQ(s.mux_flag, hunter_mux::mux_flag_remote_without_teleop);
  EXPECT_TRUE(s.is_estop);
  EXPECT_EQ(s.source, hunter_mux::source_none);
}

TEST(Mux, BridgeEstopBlocksEverySource) {
  Mux mux = make_mux(false, hunter_mux::mode_nav);
  mux.on_nav(t0);
  mux.set_bridge_estop(true);
  const Selection s = mux.select(t0);
  EXPECT_TRUE(s.is_estop);
  EXPECT_EQ(s.mux_flag, 0);
  EXPECT_EQ(s.source, hunter_mux::source_none);
}

TEST(Mux, EstopStaysLatchedAfterItsCauseClears) {
  Mux mux = make_mux(false, hunter_mux::mode_nav);
  mux.on_nav(t0);
  mux.set_bridge_estop(true);
  EXPECT_TRUE(mux.select(t0).is_estop);
  mux.set_bridge_estop(false);
  const Selection s = mux.select(t0);
  EXPECT_TRUE(s.is_estop);
  EXPECT_EQ(s.source, hunter_mux::source_none);
}

TEST(Mux, ResetReleasesTheLatchOnceTheCauseIsGone) {
  Mux mux = make_mux(false, hunter_mux::mode_nav);
  mux.on_nav(t0);
  mux.set_bridge_estop(true);
  mux.select(t0);
  mux.reset_estop(t0);
  EXPECT_TRUE(mux.select(t0).is_estop);
  mux.set_bridge_estop(false);
  mux.reset_estop(t0);
  const Selection s = mux.select(t0);
  EXPECT_FALSE(s.is_estop);
  EXPECT_EQ(s.source, hunter_mux::source_nav);
}

TEST(Mux, RequestedEstopLatchesUntilReset) {
  Mux mux = make_mux(true, hunter_mux::mode_remote);
  mux.on_teleop(t0);
  mux.request_estop();
  EXPECT_TRUE(mux.select(t0).is_estop);
  mux.reset_estop(t0);
  EXPECT_EQ(mux.select(t0).source, hunter_mux::source_teleop);
}

TEST(Mux, RemoteWithoutTeleopStaysLatchedWhenTeleopResumes) {
  Mux mux = make_mux(true, hunter_mux::mode_remote);
  mux.on_nav(t0);
  EXPECT_TRUE(mux.select(t0).is_estop);
  mux.on_teleop(t0);
  EXPECT_TRUE(mux.select(t0).is_estop);
}
