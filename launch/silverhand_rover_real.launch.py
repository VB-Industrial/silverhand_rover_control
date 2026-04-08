from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    can_iface = LaunchConfiguration("can_iface")
    node_id = LaunchConfiguration("node_id")
    queue_len = LaunchConfiguration("queue_len")
    use_imu_odometry = LaunchConfiguration("use_imu_odometry")
    use_power_board = LaunchConfiguration("use_power_board")
    power_board_client_node_id = LaunchConfiguration("power_board_client_node_id")
    power_board_config = PathJoinSubstitution(
        [FindPackageShare("silverhand_rover_control"), "config", "power_board.yaml"]
    )

    return LaunchDescription(
        [
            DeclareLaunchArgument("can_iface", default_value="vcan1"),
            DeclareLaunchArgument("node_id", default_value="110"),
            DeclareLaunchArgument("queue_len", default_value="1000"),
            DeclareLaunchArgument("use_imu_odometry", default_value="auto"),
            DeclareLaunchArgument("use_power_board", default_value="true"),
            DeclareLaunchArgument("power_board_client_node_id", default_value="111"),
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(
                    PathJoinSubstitution(
                        [FindPackageShare("silverhand_rover_control"), "launch", "silverhand_rover_bringup.launch.py"]
                    )
                ),
                launch_arguments={
                    "use_mock_hardware": "false",
                    "can_iface": can_iface,
                    "node_id": node_id,
                    "queue_len": queue_len,
                    "use_imu_odometry": use_imu_odometry,
                }.items(),
            ),
            Node(
                package="silverhand_rover_control",
                executable="power_board_node",
                output="screen",
                parameters=[
                    power_board_config,
                    {
                        "can_iface": can_iface,
                        "queue_len": queue_len,
                        "node_id": power_board_client_node_id,
                    },
                ],
                condition=IfCondition(use_power_board),
            ),
        ]
    )
