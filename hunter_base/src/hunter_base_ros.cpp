/*
 * hunter_base_ros.cpp
 *
 * Created on: Oct 15, 2021 14:35
 * Description:
 *
 * Copyright (c) 2021 Weston Robot Pte. Ltd.
 */

#include "hunter_base/hunter_base_ros.hpp"

#include <rclcpp_components/register_node_macro.hpp>
#include <stdexcept>

#include "ugv_sdk/utilities/protocol_detector.hpp"

namespace westonrobot {
HunterBaseRos::HunterBaseRos(const rclcpp::NodeOptions &options) : rclcpp::Node("hunter", options) {
  this->declare_parameter("port_name", rclcpp::ParameterValue("can0"));  // 声明参数

  this->declare_parameter("odom_frame", rclcpp::ParameterValue("odom"));
  this->declare_parameter("base_frame", rclcpp::ParameterValue("base_link"));
  this->declare_parameter("odom_topic_name", rclcpp::ParameterValue("odom"));
  this->declare_parameter("publish_tf", rclcpp::ParameterValue(true));

  this->declare_parameter("is_hunter_mini", rclcpp::ParameterValue(false));
  this->declare_parameter("is_omni_wheel", rclcpp::ParameterValue(false));

  this->declare_parameter("simulated_robot", rclcpp::ParameterValue(false));
  this->declare_parameter("control_rate", rclcpp::ParameterValue(50));

  LoadParameters();
  if (!Initialize()) throw std::runtime_error("hunter protocol detection failed");
  SetupMessenger();
}

void HunterBaseRos::LoadParameters() {
  this->get_parameter_or<std::string>("port_name", port_name_, "can0");  // 获取参数

  this->get_parameter_or<std::string>("odom_frame", odom_frame_, "odom");
  this->get_parameter_or<std::string>("base_frame", base_frame_, "base_link");
  this->get_parameter_or<std::string>("odom_topic_name", odom_topic_name_, "odom");
  this->get_parameter_or<bool>("publish_tf", publish_tf_, true);

  this->get_parameter_or<bool>("simulated_robot", simulated_robot_, false);
  this->get_parameter_or<int>("control_rate", sim_control_rate_, 50);

  std::cout << "Loading parameters: " << std::endl;
  std::cout << "- port name: " << port_name_ << std::endl;
  std::cout << "- odom frame name: " << odom_frame_ << std::endl;
  std::cout << "- base frame name: " << base_frame_ << std::endl;
  std::cout << "- odom topic name: " << odom_topic_name_ << std::endl;
  std::cout << "- publish tf: " << std::boolalpha << publish_tf_ << std::endl;

  std::cout << "- simulated robot: " << std::boolalpha << simulated_robot_ << std::endl;
  std::cout << "- sim control rate: " << sim_control_rate_ << std::endl;
  std::cout << "----------------------------" << std::endl;
}

bool HunterBaseRos::Initialize() {
  std::cout << "Robot base: Hunter" << std::endl;
  ProtocolDetector detector;
  if (detector.Connect(port_name_)) {
    auto proto = detector.DetectProtocolVersion(5);
    if (proto == ProtocolVersion::AGX_V1) {
      std::cout << "Detected protocol: AGX_V1" << std::endl;
      robot_ = std::unique_ptr<HunterRobot>(new HunterRobot(ProtocolVersion::AGX_V1));
      version = 1;
    } else if (proto == ProtocolVersion::AGX_V2) {
      std::cout << "Detected protocol: AGX_V2" << std::endl;
      robot_ = std::unique_ptr<HunterRobot>(new HunterRobot(ProtocolVersion::AGX_V2));
      version = 2;
    } else {
      std::cout << "Detected protocol: UNKONWN" << std::endl;
      return false;
    }
  } else {
    return false;
  }

  return true;
}

void HunterBaseRos::SetupMessenger() {
  messenger_ =
      std::unique_ptr<HunterMessenger<HunterRobot>>(new HunterMessenger<HunterRobot>(robot_, this));

  if (version == 1) {
    messenger_->SetTrack(HunterV1Params::track);
    messenger_->SetWeelbase(HunterV1Params::wheelbase);
    messenger_->SetMaxSteerAngleCentral(HunterV1Params::max_steer_angle_central);
    messenger_->SetMaxSteerAngle(HunterV1Params::max_steer_angle);

  } else {
    messenger_->SetTrack(HunterV2Params::track);
    messenger_->SetWeelbase(HunterV2Params::wheelbase);
    messenger_->SetMaxSteerAngleCentral(HunterV2Params::max_steer_angle_central);
    messenger_->SetMaxSteerAngle(HunterV2Params::max_steer_angle);
  }

  messenger_->SetOdometryFrame(odom_frame_);
  messenger_->SetBaseFrame(base_frame_);
  messenger_->SetOdometryTopicName(odom_topic_name_);
  messenger_->SetPublishTf(publish_tf_);
  if (simulated_robot_) messenger_->SetSimulationMode(sim_control_rate_);

  // connect to robot and setup ROS subscription
  if (port_name_.find("can") == std::string::npos) {
    throw std::runtime_error("port_name is not a CAN port: " + port_name_);
  }
  if (!robot_->Connect(port_name_)) {
    throw std::runtime_error("failed to connect to CAN bus " + port_name_);
  }
  robot_->EnableCommandedMode();
  std::cout << "Using CAN bus to talk with the robot" << std::endl;

  // publish robot state at 50Hz while listening to twist commands
  messenger_->SetupSubscription();
  timer_ = create_wall_timer(std::chrono::milliseconds(20),
                             [this]() { messenger_->PublishStateToROS(); });
}
}  // namespace westonrobot

RCLCPP_COMPONENTS_REGISTER_NODE(westonrobot::HunterBaseRos)
