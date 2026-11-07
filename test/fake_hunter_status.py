#!/usr/bin/env python3
"""Publish random HunterStatus messages at 50 Hz for testing."""

import random

from hunter_msgs.msg import HunterActuatorState
from hunter_msgs.msg import HunterStatus
import rclpy
from rclpy.node import Node
from std_msgs.msg import Header


class FakeHunterStatus(Node):
    def __init__(self):
        super().__init__('fake_hunter_status')
        self.pub = self.create_publisher(HunterStatus, '/hunter_status', 10)
        self.create_timer(1.0 / 50.0, self.publish)
        self.get_logger().info('fake_hunter_status started at 50 Hz')

    def publish(self):
        msg = HunterStatus()
        msg.header = Header()
        msg.header.stamp = self.get_clock().now().to_msg()

        msg.linear_velocity = random.uniform(-1.5, 1.5)
        msg.steering_angle = random.uniform(-0.48, 0.48)
        msg.vehicle_state = 0  # 0 normal, 1 estop, 3exception
        msg.control_mode = 1  # 0 standby, 1 can, 2 uart, 3 rc
        msg.error_code = random.choice([0, 0, 0, 0, 1, 2, 4])
        msg.battery_voltage = random.uniform(24.0, 29.4)

        for i in range(3):
            a = HunterActuatorState()
            a.motor_id = i
            a.rpm = random.randint(-500, 500)
            a.current = random.uniform(0.0, 10.0)
            a.pulse_count = random.randint(-100000, 100000)
            a.driver_voltage = random.uniform(22.0, 30.0)
            a.driver_temperature = random.uniform(25.0, 70.0)
            a.motor_temperature = random.randint(20, 80)
            a.driver_state = random.randint(0, 255)
            msg.actuator_states[i] = a

        self.pub.publish(msg)


def main(args=None):
    rclpy.init(args=args)
    node = FakeHunterStatus()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
