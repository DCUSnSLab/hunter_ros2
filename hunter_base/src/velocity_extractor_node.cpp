#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/float64.hpp>

#include "hunter_msgs/msg/hunter_status.hpp"

class VelocityExtractorNode : public rclcpp::Node {
 public:
  VelocityExtractorNode() : Node("velocity_extractor") {
    hunter_status_sub_ = this->create_subscription<hunter_msgs::msg::HunterStatus>(
        "/hunter_status", 10,
        std::bind(&VelocityExtractorNode::hunterStatusCallback, this, std::placeholders::_1));

    velocity_pub_ = this->create_publisher<std_msgs::msg::Float64>("/hunter/velocity", 10);

    RCLCPP_INFO(this->get_logger(), "Velocity extractor node started");
  }

 private:
  void hunterStatusCallback(const hunter_msgs::msg::HunterStatus::SharedPtr msg) {
    auto velocity_msg = std_msgs::msg::Float64();
    velocity_msg.data = msg->linear_velocity;

    velocity_pub_->publish(velocity_msg);
  }

  rclcpp::Subscription<hunter_msgs::msg::HunterStatus>::SharedPtr hunter_status_sub_;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr velocity_pub_;
};

int main(int argc, char* argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<VelocityExtractorNode>());
  rclcpp::shutdown();
  return 0;
}