# ROS2 Packages for Hunter Mobile Base

## Packages

* hunter_base: a ROS wrapper around Hunter SDK to monitor and control the robot
* hunter_msgs: hunter related message definitions
* hunter_mux: command multiplexer that routes navigation or remote teleop commands to the vehicle with safety E-stop logic
* hunter_mux_msgs: mode, e-stop and status messages of hunter_mux


## hunter_mux

hunter_mux publishes `/final_cmd` from navigation (`~/input/cmd_vel`) or remote teleop
(`~/input/teleop_cmd`), chosen by `~/input/cmd_mode` (1 navigation, 2 remote; remote only when
`remote_status` is true, set with `remote_status:=true` on `hunter_base.launch.py`).

An e-stop stays latched until a reset, whatever caused it: a request on `~/input/estop`, the bridge
flag on `~/input/estop_status`, or remote mode with navigation running and no teleop command. An
empty message on `~/input/estop_reset` releases it only once no cause is still active. The state is
on `~/output/estop_status`.

## Communication interface setup

Please refer to the [README](https://github.com/westonrobot/ugv_sdk) of "ugv_sdk" package for setup of communication interfaces.

#### Note on CAN interface on Nvidia Jetson Platforms

Nvidia Jeston TX2/Xavier/XavierNX have CAN controller(s) integrated in the main SOC. If you're using a dev kit, you need to add a CAN transceiver for proper CAN communication. 

## Basic usage of the ROS package


1. Clone the packages into your catkin workspace and compile

    (the following instructions assume your catkin workspace is at: ~/ros2_ws/src)

    ```
    $ cd ~/ros2_ws/src
    $ git clone https://github.com/agilexrobotics/ugv_sdk.git
    $ git clone https://github.com/agilexrobotics/hunter_ros2.git
    $ cd ..
    $ colcon build
    ```
    
2. Setup CAN-To-USB adapter

* Enable gs_usb kernel module(If you have already added this module, you do not need to add it)
    ```
    $ sudo modprobe gs_usb
    ```
    
* first time use hunter-ros2 package
   ```
   $ cd ~/your_ws/src/ugv_sdk/scripts/
   $ bash setup_can2usb.bash
   ```
   
* if not the first time use hunter-ros2 package(Run this command every time you turn off the power) 
   ```
   $ cd ~/catkin_ws/src/ugv_sdk/scripts/
   $ bash bringup_can2usb_500k.bash
   ```
   
* Testing command
    ```
    # receiving data from can0
    $ candump can0
    ```

3. Launch ROS nodes

* Start the base node for the real robot

    ```
    $ ros2 launch hunter_base hunter_base.launch.py
    ```
* Then you can send command to the robot
    ```
    $ ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "linear:
    x: 0.0
    y: 0.0
    z: 0.0
    angular:
    x: 0.0
    y: 0.0
    z: 0.0" 

    ```