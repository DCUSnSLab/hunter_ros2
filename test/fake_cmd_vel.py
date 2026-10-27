#!/usr/bin/env python3
"""
Dummy /cmd_vel publisher for testing the mux_node pipeline.

Usage:
  ros2 run hunter_ros2 fake_cmd_vel.py
  ros2 run hunter_ros2 fake_cmd_vel.py --ros-args -p pattern:=sine -p rate:=10.0

Parameters:
  pattern   : 'sine' | 'square' | 'zero'  (default: sine)
  rate      : Hz  (default: 10.0)
  max_speed : m/s (default: 0.3)
  max_steer : deg (default: 20.0)
"""
import math
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist


class FakeCmdVel(Node):
    def __init__(self):
        super().__init__('fake_cmd_vel')
        self.declare_parameter('pattern',   'sine')
        self.declare_parameter('rate',      10.0)
        self.declare_parameter('max_speed', 0.3)
        self.declare_parameter('max_steer', 20.0)   # degrees

        self._pattern   = self.get_parameter('pattern').value
        rate            = self.get_parameter('rate').value
        self._max_speed = self.get_parameter('max_speed').value
        self._max_steer = math.radians(self.get_parameter('max_steer').value)

        self._pub = self.create_publisher(Twist, '/cmd_vel', 10)
        self._t   = 0.0
        self._dt  = 1.0 / rate
        self.create_timer(self._dt, self._tick)
        self.get_logger().info(
            f'fake_cmd_vel started — pattern={self._pattern} rate={rate}Hz '
            f'max_speed={self._max_speed}m/s max_steer={math.degrees(self._max_steer):.1f}°'
        )

    def _tick(self):
        msg = Twist()

        if self._pattern == 'sine':
            msg.linear.x  = self._max_speed * math.sin(self._t)
            msg.angular.z = self._max_steer * math.sin(self._t * 0.5)

        elif self._pattern == 'square':
            sign = 1.0 if int(self._t) % 2 == 0 else -1.0
            msg.linear.x  = self._max_speed * sign
            msg.angular.z = 0.0

        # pattern == 'zero' leaves Twist() at (0, 0)

        self._pub.publish(msg)
        self.get_logger().debug(
            f'lx={msg.linear.x:.3f}  az={msg.angular.z:.3f}'
        )
        self._t += self._dt


def main(args=None):
    rclpy.init(args=args)
    node = FakeCmdVel()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
