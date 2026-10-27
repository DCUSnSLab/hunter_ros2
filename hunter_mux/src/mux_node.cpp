#include <chrono>
#include <geometry_msgs/msg/twist.hpp>
#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_components/register_node_macro.hpp>
#include <teleop_rover_msgs/msg/cmd_mode.hpp>
#include <teleop_rover_msgs/msg/e_stop_status.hpp>
#include <teleop_rover_msgs/msg/mux_status.hpp>

namespace hunter_mux {

using geometry_msgs::msg::Twist;
using teleop_rover_msgs::msg::CmdMode;
using teleop_rover_msgs::msg::EStopStatus;
using teleop_rover_msgs::msg::MuxStatus;
using Clock = std::chrono::steady_clock;

class MuxNode : public rclcpp::Node {
 public:
  static constexpr int8_t MODE_IDLE = -1;
  static constexpr int8_t MODE_NAV = 1;
  static constexpr int8_t MODE_REMOTE = 2;
  static constexpr int8_t SOURCE_NONE = -1;
  static constexpr int8_t SOURCE_NAV = 0;
  static constexpr int8_t SOURCE_TELEOP = 1;

  explicit MuxNode(const rclcpp::NodeOptions& options) : Node("mux_node", options) {
    allow_remote_ = declare_parameter<bool>("remote_status", false);
    cmd_vel_timeout_ =
        std::chrono::duration<double>(declare_parameter<double>("cmd_vel_timeout", 0.5));
    teleop_timeout_ =
        std::chrono::duration<double>(declare_parameter<double>("teleop_timeout", 0.5));
    const double rate = declare_parameter<double>("rate", 50.0);

    param_cb_ =
        add_on_set_parameters_callback([this](const std::vector<rclcpp::Parameter>& params) {
          for (const auto& p : params) {
            if (p.get_name() == "remote_status" &&
                p.get_type() == rclcpp::ParameterType::PARAMETER_BOOL) {
              allow_remote_ = p.as_bool();
              RCLCPP_INFO(get_logger(), "remote_status changed to %d", allow_remote_);
            }
          }
          rcl_interfaces::msg::SetParametersResult result;
          result.successful = true;
          return result;
        });

    final_cmd_pub_ = create_publisher<Twist>("/final_cmd", 1);
    mux_status_pub_ = create_publisher<MuxStatus>("/vehicle/mux_status", 10);
    estop_status_pub_ = create_publisher<EStopStatus>("/vehicle/estop_status", 10);

    cmd_vel_sub_ = create_subscription<Twist>("/cmd_vel", 1, [this](Twist::ConstSharedPtr msg) {
      last_nav_msg_ = *msg;
      last_nav_time_ = Clock::now();
      PublishIfSelected(SOURCE_NAV);
    });
    teleop_sub_ =
        create_subscription<Twist>("/remote/teleop_cmd", 1, [this](Twist::ConstSharedPtr msg) {
          last_teleop_msg_ = *msg;
          last_teleop_time_ = Clock::now();
          PublishIfSelected(SOURCE_TELEOP);
        });
    estop_sub_ = create_subscription<EStopStatus>("/remote/estop_status", 10,
                                                  [this](EStopStatus::ConstSharedPtr msg) {
                                                    bridge_estop_ = *msg;
                                                    PublishFinal(Select());
                                                  });
    cmd_mode_sub_ =
        create_subscription<CmdMode>("/remote/cmd_mode", 10, [this](CmdMode::ConstSharedPtr msg) {
          cmd_mode_ = msg->mode;
          PublishFinal(Select());
        });

    timer_ = create_wall_timer(std::chrono::duration<double>(1.0 / rate), [this]() { Tick(); });
    RCLCPP_INFO(get_logger(), "MuxNode started: remote_status=%d, rate=%.1fHz", allow_remote_,
                rate);
  }

 private:
  struct Selection {
    Twist cmd;
    int8_t source = SOURCE_NONE;
    bool nav_active = false;
    bool teleop_active = false;
    bool is_estop = false;
    int8_t mux_flag = 0;
  };

  Selection Select() const {
    const auto now = Clock::now();
    Selection s;
    s.nav_active = now - last_nav_time_ < cmd_vel_timeout_;
    s.teleop_active = now - last_teleop_time_ < teleop_timeout_;

    const bool remote_mode_requested = cmd_mode_ == MODE_REMOTE;
    if (allow_remote_ && remote_mode_requested && s.nav_active && !s.teleop_active) {
      s.mux_flag = 1;
    }
    s.is_estop = bridge_estop_.is_estop || s.mux_flag != 0;

    if (s.is_estop) {
      return s;
    }
    if (!allow_remote_ || cmd_mode_ == MODE_NAV) {
      if (s.nav_active) {
        s.cmd = last_nav_msg_;
        s.source = SOURCE_NAV;
      }
    } else if (remote_mode_requested && s.teleop_active) {
      s.cmd = last_teleop_msg_;
      s.source = SOURCE_TELEOP;
    }
    return s;
  }

  void PublishIfSelected(int8_t source) {
    const Selection s = Select();
    if (s.source == source) {
      PublishFinal(s);
    }
  }

  void PublishFinal(const Selection& s) {
    final_cmd_pub_->publish(std::make_unique<Twist>(s.cmd));
    published_since_tick_ = true;
  }

  // The tick keeps /final_cmd alive at `rate` and drops it to zero once inputs time out.
  void Tick() {
    const Selection s = Select();
    if (!published_since_tick_) {
      PublishFinal(s);
    }
    published_since_tick_ = false;

    auto mux_status = std::make_unique<MuxStatus>();
    mux_status->mode = cmd_mode_;
    mux_status->cmd_source = s.source;
    mux_status->remote_status = allow_remote_;
    mux_status->nav_active = s.nav_active;
    mux_status->teleop_active = s.teleop_active;
    mux_status->final_active = s.source != SOURCE_NONE;
    mux_status_pub_->publish(std::move(mux_status));

    auto estop_status = std::make_unique<EStopStatus>();
    estop_status->is_estop = s.is_estop;
    estop_status->bridge_flag = bridge_estop_.bridge_flag;
    estop_status->mux_flag = s.mux_flag;
    estop_status_pub_->publish(std::move(estop_status));
  }

  bool allow_remote_;
  std::chrono::duration<double> cmd_vel_timeout_;
  std::chrono::duration<double> teleop_timeout_;
  int8_t cmd_mode_ = MODE_IDLE;
  Twist last_nav_msg_;
  Twist last_teleop_msg_;
  Clock::time_point last_nav_time_{};
  Clock::time_point last_teleop_time_{};
  EStopStatus bridge_estop_;
  bool published_since_tick_ = false;

  OnSetParametersCallbackHandle::SharedPtr param_cb_;
  rclcpp::Publisher<Twist>::SharedPtr final_cmd_pub_;
  rclcpp::Publisher<MuxStatus>::SharedPtr mux_status_pub_;
  rclcpp::Publisher<EStopStatus>::SharedPtr estop_status_pub_;
  rclcpp::Subscription<Twist>::SharedPtr cmd_vel_sub_;
  rclcpp::Subscription<Twist>::SharedPtr teleop_sub_;
  rclcpp::Subscription<EStopStatus>::SharedPtr estop_sub_;
  rclcpp::Subscription<CmdMode>::SharedPtr cmd_mode_sub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

}  // namespace hunter_mux

RCLCPP_COMPONENTS_REGISTER_NODE(hunter_mux::MuxNode)
