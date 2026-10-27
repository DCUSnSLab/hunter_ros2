import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    pkg_mux = get_package_share_directory('hunter_mux')
    mux_params = os.path.join(pkg_mux, 'config', 'mux_params.yaml')

    use_sim_arg = DeclareLaunchArgument(
        'use_sim', default_value='true',
        description='Use mock hunter base for simulation')

    mux_node = Node(
        package='hunter_mux',
        executable='mux_node.py',
        name='mux_node',
        output='screen',
        parameters=[mux_params],
    )

    return LaunchDescription([
        use_sim_arg,
        mux_node,
    ])
