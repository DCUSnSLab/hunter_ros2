#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from rclpy.parameter import Parameter
from rcl_interfaces.msg import SetParametersResult
from geometry_msgs.msg import Twist
from teleop_rover_msgs.msg import EStopStatus, CmdMode, MuxStatus
import time
from typing import cast


class MuxNode(Node):
    MODE_IDLE = -1
    MODE_CTRL_ON = 0
    MODE_NAV = 1
    MODE_REMOTE = 2

    def __init__(self):
        super().__init__('mux_node')
        self.declare_parameter('remote_status', False)
        self.declare_parameter('cmd_vel_timeout', 0.5)
        self.declare_parameter('teleop_timeout', 0.5)
        self.declare_parameter('rate', 50.0)

        self.allow_remote = cast(bool, self.get_parameter('remote_status').value)
        self.cmd_vel_timeout = cast(float, self.get_parameter('cmd_vel_timeout').value)
        self.teleop_timeout = cast(float, self.get_parameter('teleop_timeout').value)

        rate = cast(float, self.get_parameter('rate').value)

        self.add_on_set_parameters_callback(self.on_param_change)

        # -1=idle, 0=local control(enabled controller), 1=nav control, 2=remote control
        self.cmd_mode = self.MODE_IDLE
        self.last_nav_time = 0.0
        self.last_teleop_time = 0.0

        self.last_nav_msg = Twist()
        self.last_teleop_msg = Twist()
        self.bridge_estop = EStopStatus()

        self.create_subscription(Twist, '/cmd_vel', self.cmd_vel_callback, 10)
        self.create_subscription(Twist, '/remote/teleop_cmd', self.teleop_callback, 10)
        self.create_subscription(EStopStatus, '/remote/estop_status', self.estop_callback, 10)
        self.create_subscription(CmdMode, '/remote/cmd_mode', self.cmd_mode_callback, 10)

        self.final_cmd_pub = self.create_publisher(Twist, '/final_cmd', 10)
        self.mux_status_pub = self.create_publisher(MuxStatus, '/vehicle/mux_status', 10)
        self.estop_status_pub = self.create_publisher(EStopStatus, '/vehicle/estop_status', 10)

        period = 1.0 / rate
        self.create_timer(period, self.tick)
        self.get_logger().info(f'MuxNode started: remote_status={self.allow_remote}, rate={rate}Hz')

    def on_param_change(self, params):
        for p in params:
            if p.name == 'remote_status' and p.type_ == Parameter.Type.BOOL:
                self.allow_remote = p.value
                self.get_logger().info(f'remote_status changed to {p.value}')
        return SetParametersResult(successful=True)

    def cmd_vel_callback(self, msg: Twist):
        self.last_nav_msg = msg
        self.last_nav_time = time.monotonic()

    def teleop_callback(self, msg: Twist):
        self.last_teleop_msg = msg
        self.last_teleop_time = time.monotonic()

    def estop_callback(self, msg: EStopStatus):
        self.bridge_estop = msg

    def cmd_mode_callback(self, msg: CmdMode):
        self.cmd_mode = msg.mode

    def tick(self):
        now = time.monotonic()
        
        #check recent cmd
        nav_active = (now - self.last_nav_time) < self.cmd_vel_timeout
        teleop_active = (now - self.last_teleop_time) < self.teleop_timeout

        remote_mode_requested = self.cmd_mode == self.MODE_REMOTE
        current_remote_mode = self.allow_remote and remote_mode_requested

        mux_flag = 0
        bridge_is_estop = self.bridge_estop.is_estop
        bridge_flag = self.bridge_estop.bridge_flag
        
        # if enabled remote control and activate navgation command
        if current_remote_mode and nav_active:
            # but teleop command is not activate
            if not teleop_active:
                mux_flag = 1

        is_estop = bridge_is_estop or (mux_flag != 0)
        final_msg = Twist()
        cmd_source = -1
        final_active = False

        if is_estop:
            pass
        elif not self.allow_remote:
            if nav_active:
                final_msg = self.last_nav_msg
                cmd_source = 0
                final_active = True
        else:
            if remote_mode_requested:
                if teleop_active:
                    final_msg = self.last_teleop_msg
                    cmd_source = 1
                    final_active = True
            elif self.cmd_mode == self.MODE_NAV:
                if nav_active:
                    final_msg = self.last_nav_msg
                    cmd_source = 0
                    final_active = True

        self.final_cmd_pub.publish(final_msg)
        mux_status = MuxStatus()
        mux_status.mode = int(self.cmd_mode)
        mux_status.cmd_source = int(cmd_source)
        mux_status.remote_status = self.allow_remote
        mux_status.nav_active = nav_active
        mux_status.teleop_active = teleop_active
        mux_status.final_active = final_active
        self.mux_status_pub.publish(mux_status)
        estop_status = EStopStatus()
        estop_status.is_estop = is_estop
        estop_status.bridge_flag = int(bridge_flag)
        estop_status.mux_flag = int(mux_flag)
        self.estop_status_pub.publish(estop_status)

def main(args=None):
    rclpy.init(args=args)
    node = MuxNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
