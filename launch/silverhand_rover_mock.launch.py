import os
import yaml

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def load_profile(profile_name):
    package_path = get_package_share_directory("silverhand_rover_control")
    profile_path = os.path.join(package_path, "config", "hardware_profiles.yaml")
    with open(profile_path, "r", encoding="utf-8") as file:
        profiles = yaml.safe_load(file)["profiles"]
    return profiles[profile_name]


def generate_launch_description():
    mock_profile = load_profile("mock")
    can_iface = LaunchConfiguration("can_iface")
    node_id = LaunchConfiguration("node_id")
    queue_len = LaunchConfiguration("queue_len")
    use_power_board = LaunchConfiguration("use_power_board")
    power_board_client_node_id = LaunchConfiguration("power_board_client_node_id")
    power_board_config = PathJoinSubstitution(
        [FindPackageShare("silverhand_rover_control"), "config", "power_board.yaml"]
    )

    return LaunchDescription(
        [
            DeclareLaunchArgument("can_iface", default_value=str(mock_profile["can_iface"])),
            DeclareLaunchArgument("node_id", default_value=str(mock_profile["node_id"])),
            DeclareLaunchArgument("queue_len", default_value=str(mock_profile["queue_len"])),
            DeclareLaunchArgument("use_power_board", default_value=str(mock_profile["use_power_board"]).lower()),
            DeclareLaunchArgument(
                "power_board_client_node_id",
                default_value=str(mock_profile["power_board_client_node_id"]),
            ),
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(
                    PathJoinSubstitution(
                        [FindPackageShare("silverhand_rover_control"), "launch", "silverhand_rover_bringup.launch.py"]
                    )
                ),
                launch_arguments={
                    "use_mock_hardware": "true",
                    "can_iface": can_iface,
                    "node_id": node_id,
                    "queue_len": queue_len,
                    "use_imu_odometry": str(mock_profile["use_imu_odometry"]),
                }.items(),
            ),
            Node(
                package="silverhand_rover_control",
                executable="power_board_node",
                output="screen",
                parameters=[
                    power_board_config,
                    {
                        "use_mock": True,
                        "can_iface": can_iface,
                        "queue_len": queue_len,
                        "node_id": power_board_client_node_id,
                    },
                ],
                condition=IfCondition(use_power_board),
            ),
        ]
    )
