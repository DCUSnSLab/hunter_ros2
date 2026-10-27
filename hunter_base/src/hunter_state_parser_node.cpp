#include <cmath>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/float64.hpp>

#include "hunter_msgs/msg/hunter_status.hpp"

class HunterStateParserNode : public rclcpp::Node {
 public:
  HunterStateParserNode() : Node("hunter_state_parser") {
    const std::string status_topic =
        this->declare_parameter<std::string>("status_topic", "/hunter_status");
    const std::string speed_topic =
        this->declare_parameter<std::string>("speed_topic", "current_speed");
    const std::string steer_topic =
        this->declare_parameter<std::string>("steer_topic", "current_steer_angle");

    speed_pub_ = this->create_publisher<std_msgs::msg::Float64>(speed_topic, 10);
    steer_pub_ = this->create_publisher<std_msgs::msg::Float64>(steer_topic, 10);

    status_sub_ = this->create_subscription<hunter_msgs::msg::HunterStatus>(
        status_topic, 10,
        std::bind(&HunterStateParserNode::statusCallback, this, std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(), "hunter_state_parser started: '%s' -> '%s' (m/s), '%s' (deg)",
                status_topic.c_str(), speed_topic.c_str(), steer_topic.c_str());
  }

 private:
  void statusCallback(const hunter_msgs::msg::HunterStatus::SharedPtr msg) {
    std_msgs::msg::Float64 speed_msg;
    speed_msg.data = msg->linear_velocity;
    speed_pub_->publish(speed_msg);

    std_msgs::msg::Float64 steer_msg;
    steer_msg.data = msg->steering_angle * 180.0 / M_PI;
    steer_pub_->publish(steer_msg);
  }

  rclcpp::Subscription<hunter_msgs::msg::HunterStatus>::SharedPtr status_sub_;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr speed_pub_;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr steer_pub_;
};

int main(int argc, char* argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<HunterStateParserNode>());
  rclcpp::shutdown();
  return 0;
}
