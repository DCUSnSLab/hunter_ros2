import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import ComposableNodeContainer, Node
from launch_ros.descriptions import ComposableNode


def generate_launch_description():
    use_sim_time_arg = DeclareLaunchArgument('use_sim_time', default_value='false',
                                             description='Use simulation clock if true')

    port_name_arg = DeclareLaunchArgument('port_name', default_value='can0',
                                          description='CAN bus name, e.g. can0')
    odom_frame_arg = DeclareLaunchArgument('odom_frame', default_value='odom',
                                           description='Odometry frame id')
    base_link_frame_arg = DeclareLaunchArgument('base_frame', default_value='base_link',
                                                description='Base link frame id')
    odom_topic_arg = DeclareLaunchArgument('odom_topic_name', default_value='odometry/wheel',
                                           description='Odometry topic name')
    publish_tf_arg = DeclareLaunchArgument('publish_tf', default_value='false',
                                           description='Whether to publish TF (odom->base_link)')

    simulated_robot_arg = DeclareLaunchArgument('simulated_robot', default_value='false',
                                                description='Whether running with simulator')
    sim_control_rate_arg = DeclareLaunchArgument('control_rate', default_value='50',
                                                 description='Simulation control loop update rate')

    mux_params = os.path.join(
        get_package_share_directory('hunter_mux'), 'config', 'mux_params.yaml')
    use_sim_time = LaunchConfiguration('use_sim_time')
    intra = [{'use_intra_process_comms': True}]

    # hunter_base and hunter_mux share one process so /final_cmd stays intra-process.
    vehicle_container = ComposableNodeContainer(
        name='vehicle_container',
        namespace='',
        package='rclcpp_components',
        executable='component_container',
        output='screen',
        emulate_tty=True,
        composable_node_descriptions=[
            ComposableNode(
                package='hunter_base',
                plugin='westonrobot::HunterBaseRos',
                name='hunter',
                parameters=[{
                    'use_sim_time': use_sim_time,
                    'port_name': LaunchConfiguration('port_name'),
                    'odom_frame': LaunchConfiguration('odom_frame'),
                    'base_frame': LaunchConfiguration('base_frame'),
                    'odom_topic_name': LaunchConfiguration('odom_topic_name'),
                    'publish_tf': LaunchConfiguration('publish_tf'),
                    'simulated_robot': LaunchConfiguration('simulated_robot'),
                    'control_rate': LaunchConfiguration('control_rate'),
                }],
                extra_arguments=intra,
            ),
            ComposableNode(
                package='hunter_mux',
                plugin='hunter_mux::MuxNode',
                name='mux_node',
                parameters=[mux_params, {'use_sim_time': use_sim_time}],
                extra_arguments=intra,
            ),
        ],
    )

    velocity_extractor_node = Node(
        package='hunter_base',
        executable='velocity_extractor_node',
        output='screen',
        emulate_tty=True)

    hunter_state_parser_node = Node(
        package='hunter_base',
        executable='hunter_state_parser_node',
        output='screen',
        emulate_tty=True)

    return LaunchDescription([
        use_sim_time_arg,
        port_name_arg,
        odom_frame_arg,
        base_link_frame_arg,
        odom_topic_arg,
        publish_tf_arg,
        simulated_robot_arg,
        sim_control_rate_arg,
        vehicle_container,
        velocity_extractor_node,
        hunter_state_parser_node
    ])
