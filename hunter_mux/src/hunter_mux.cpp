#include <chrono>
#include <geometry_msgs/msg/twist.hpp>
#include <hunter_mux_msgs/msg/cmd_mode.hpp>
#include <hunter_mux_msgs/msg/e_stop_status.hpp>
#include <hunter_mux_msgs/msg/mux_status.hpp>
#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_components/register_node_macro.hpp>
#include <std_msgs/msg/empty.hpp>
#include <utility>
#include <vector>

#include "mux.hpp"

namespace hunter_mux {

using geometry_msgs::msg::Twist;
using hunter_mux_msgs::msg::CmdMode;
using hunter_mux_msgs::msg::EStopStatus;
using hunter_mux_msgs::msg::MuxStatus;
using std_msgs::msg::Empty;

class MuxNode : public rclcpp::Node {
 public:
  explicit MuxNode(const rclcpp::NodeOptions& options)
      : Node("hunter_mux", options),
        mux_(std::chrono::duration<double>(declare_parameter<double>("cmd_vel_timeout")),
             std::chrono::duration<double>(declare_parameter<double>("teleop_timeout"))) {
    mux_.set_allow_remote(declare_parameter<bool>("remote_status"));
    const double rate = declare_parameter<double>("rate");

    param_cb_ =
        add_on_set_parameters_callback([this](const std::vector<rclcpp::Parameter>& params) {
          for (const auto& p : params) {
            if (p.get_name() == "remote_status") {
              mux_.set_allow_remote(p.as_bool());
              RCLCPP_INFO(get_logger(), "remote_status changed to %d", mux_.allow_remote());
            }
          }
          rcl_interfaces::msg::SetParametersResult result;
          result.successful = true;
          return result;
        });

    final_cmd_pub_ = create_publisher<Twist>("~/output/final_cmd", 1);
    mux_status_pub_ = create_publisher<MuxStatus>("~/output/mux_status", 10);
    estop_status_pub_ = create_publisher<EStopStatus>("~/output/estop_status", 10);

    cmd_vel_sub_ =
        create_subscription<Twist>("~/input/cmd_vel", 1, [this](Twist::ConstSharedPtr msg) {
          last_nav_msg_ = *msg;
          mux_.on_nav(Clock::now());
          publish_if_selected(source_nav);
        });
    teleop_sub_ =
        create_subscription<Twist>("~/input/teleop_cmd", 1, [this](Twist::ConstSharedPtr msg) {
          last_teleop_msg_ = *msg;
          mux_.on_teleop(Clock::now());
          publish_if_selected(source_teleop);
        });
    estop_sub_ = create_subscription<EStopStatus>("~/input/estop_status", 10,
                                                  [this](EStopStatus::ConstSharedPtr msg) {
                                                    bridge_estop_ = *msg;
                                                    mux_.set_bridge_estop(msg->is_estop);
                                                    publish_final(mux_.select(Clock::now()));
                                                  });
    estop_request_sub_ =
        create_subscription<Empty>("~/input/estop", 10, [this](Empty::ConstSharedPtr) {
          mux_.request_estop();
          RCLCPP_WARN(get_logger(), "e-stop requested");
          publish_final(mux_.select(Clock::now()));
        });
    estop_reset_sub_ =
        create_subscription<Empty>("~/input/estop_reset", 10, [this](Empty::ConstSharedPtr) {
          mux_.reset_estop(Clock::now());
          const Selection s = mux_.select(Clock::now());
          RCLCPP_INFO(get_logger(), "e-stop reset: %s",
                      s.is_estop ? "a cause is still active" : "released");
          publish_final(s);
        });
    cmd_mode_sub_ =
        create_subscription<CmdMode>("~/input/cmd_mode", 10, [this](CmdMode::ConstSharedPtr msg) {
          mux_.set_cmd_mode(msg->mode);
          publish_final(mux_.select(Clock::now()));
        });

    timer_ = create_wall_timer(std::chrono::duration<double>(1.0 / rate), [this]() { tick(); });
    RCLCPP_INFO(get_logger(), "hunter_mux started: remote_status=%d, rate=%.1fHz",
                mux_.allow_remote(), rate);
  }

 private:
  void publish_if_selected(int8_t source) {
    const Selection s = mux_.select(Clock::now());
    if (s.source == source) {
      publish_final(s);
    }
  }

  void publish_final(const Selection& s) {
    auto cmd = std::make_unique<Twist>();
    if (s.source == source_nav) {
      *cmd = last_nav_msg_;
    } else if (s.source == source_teleop) {
      *cmd = last_teleop_msg_;
    }
    final_cmd_pub_->publish(std::move(cmd));
    published_since_tick_ = true;
  }

  // The tick keeps final_cmd alive at `rate` and drops it to zero once inputs time out.
  void tick() {
    const Selection s = mux_.select(Clock::now());
    if (!published_since_tick_) {
      publish_final(s);
    }
    published_since_tick_ = false;

    auto mux_status = std::make_unique<MuxStatus>();
    mux_status->mode = mux_.cmd_mode();
    mux_status->cmd_source = s.source;
    mux_status->remote_status = mux_.allow_remote();
    mux_status->nav_active = s.nav_active;
    mux_status->teleop_active = s.teleop_active;
    mux_status->final_active = s.source != source_none;
    mux_status_pub_->publish(std::move(mux_status));

    auto estop_status = std::make_unique<EStopStatus>();
    estop_status->is_estop = s.is_estop;
    estop_status->bridge_flag = bridge_estop_.bridge_flag;
    estop_status->mux_flag = s.mux_flag;
    estop_status_pub_->publish(std::move(estop_status));
  }

  Mux mux_;
  Twist last_nav_msg_;
  Twist last_teleop_msg_;
  EStopStatus bridge_estop_;
  bool published_since_tick_ = false;

  OnSetParametersCallbackHandle::SharedPtr param_cb_;
  rclcpp::Publisher<Twist>::SharedPtr final_cmd_pub_;
  rclcpp::Publisher<MuxStatus>::SharedPtr mux_status_pub_;
  rclcpp::Publisher<EStopStatus>::SharedPtr estop_status_pub_;
  rclcpp::Subscription<Twist>::SharedPtr cmd_vel_sub_;
  rclcpp::Subscription<Twist>::SharedPtr teleop_sub_;
  rclcpp::Subscription<EStopStatus>::SharedPtr estop_sub_;
  rclcpp::Subscription<Empty>::SharedPtr estop_request_sub_;
  rclcpp::Subscription<Empty>::SharedPtr estop_reset_sub_;
  rclcpp::Subscription<CmdMode>::SharedPtr cmd_mode_sub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

}  // namespace hunter_mux

RCLCPP_COMPONENTS_REGISTER_NODE(hunter_mux::MuxNode)
