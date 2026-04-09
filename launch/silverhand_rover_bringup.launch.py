import os
import re
from pathlib import Path
import yaml

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, LogInfo, OpaqueFunction
from launch.substitutions import Command, FindExecutable, LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare


def _is_truthy(value: str) -> bool:
    return value.lower() in ("true", "1", "yes", "on")


def _detect_imu_available(imu_device_path: str, imu_vid: str, imu_pid: str) -> bool:
    if imu_device_path:
      return Path(imu_device_path).exists()

    try:
        expected_vid = int(imu_vid)
        expected_pid = int(imu_pid)
    except ValueError:
        return False

    for hidraw_dir in Path("/sys/class/hidraw").glob("hidraw*"):
        uevent_file = hidraw_dir / "device" / "uevent"
        if not uevent_file.exists():
            continue

        try:
            contents = uevent_file.read_text()
        except OSError:
            continue

        hid_id_match = re.search(
            r"^HID_ID=[0-9A-Fa-f]+:([0-9A-Fa-f]+):([0-9A-Fa-f]+)$",
            contents,
            re.MULTILINE,
        )
        if not hid_id_match:
            continue

        try:
            detected_vid = int(hid_id_match.group(1), 16)
            detected_pid = int(hid_id_match.group(2), 16)
        except ValueError:
            continue

        if detected_vid == expected_vid and detected_pid == expected_pid:
            return True

    return False


def _load_profile(profile_name: str):
    package_path = get_package_share_directory("silverhand_rover_control")
    profile_path = os.path.join(package_path, "config", "hardware_profiles.yaml")
    with open(profile_path, "r", encoding="utf-8") as file:
        profiles = yaml.safe_load(file)["profiles"]
    return profiles[profile_name]


def _create_runtime_actions(context, robot_description, controllers_imu_file, controllers_wheel_file, ekf_config_file):
    use_mock_hardware = LaunchConfiguration("use_mock_hardware").perform(context)
    use_imu_odometry = LaunchConfiguration("use_imu_odometry").perform(context).lower()
    imu_device_path = LaunchConfiguration("imu_device_path").perform(context)
    imu_vid = LaunchConfiguration("imu_vid").perform(context)
    imu_pid = LaunchConfiguration("imu_pid").perform(context)

    if _is_truthy(use_mock_hardware):
        imu_enabled = False
        reason = "mock hardware requested"
    elif use_imu_odometry == "true":
        imu_enabled = True
        reason = "forced by use_imu_odometry:=true"
    elif use_imu_odometry == "false":
        imu_enabled = False
        reason = "forced by use_imu_odometry:=false"
    else:
        imu_enabled = _detect_imu_available(imu_device_path, imu_vid, imu_pid)
        reason = "IMU device detected" if imu_enabled else "IMU device not detected, using wheel odometry fallback"

    selected_controllers = controllers_imu_file if imu_enabled else controllers_wheel_file
    controller_manager_name = "/rover_controller_manager"

    actions = [
        LogInfo(msg=f"silverhand_rover_control: {'IMU + EKF' if imu_enabled else 'wheel odometry'} mode selected ({reason})"),
        Node(
            package="controller_manager",
            executable="ros2_control_node",
            name="rover_controller_manager",
            output="screen",
            parameters=[robot_description, selected_controllers],
            remappings=[("/controller_manager/robot_description", "/robot_description")],
        ),
        Node(
            package="controller_manager",
            executable="spawner",
            arguments=["joint_state_broadcaster", "--controller-manager", controller_manager_name],
            output="screen",
        ),
        Node(
            package="controller_manager",
            executable="spawner",
            arguments=["rover_base_controller", "--controller-manager", controller_manager_name],
            output="screen",
        ),
    ]

    if imu_enabled:
        actions.extend(
            [
                Node(
                    package="controller_manager",
                    executable="spawner",
                    arguments=["imu_sensor_broadcaster", "--controller-manager", controller_manager_name],
                    output="screen",
                ),
                Node(
                    package="robot_localization",
                    executable="ekf_node",
                    name="ekf_filter_node",
                    output="screen",
                    parameters=[ekf_config_file],
                ),
            ]
        )

    return actions


def generate_launch_description():
    ros_control_profile = _load_profile("ros_control")
    use_mock_hardware = LaunchConfiguration("use_mock_hardware")
    can_iface = LaunchConfiguration("can_iface")
    node_id = LaunchConfiguration("node_id")
    queue_len = LaunchConfiguration("queue_len")
    imu_name = LaunchConfiguration("imu_name")
    imu_frame_id = LaunchConfiguration("imu_frame_id")
    imu_device_path = LaunchConfiguration("imu_device_path")
    imu_vid = LaunchConfiguration("imu_vid")
    imu_pid = LaunchConfiguration("imu_pid")
    imu_report_size = LaunchConfiguration("imu_report_size")
    use_imu_odometry = LaunchConfiguration("use_imu_odometry")

    description_file = PathJoinSubstitution(
        [FindPackageShare("silverhand_rover_control"), "urdf", "silverhand_rover.urdf.xacro"]
    )
    controllers_imu_file = PathJoinSubstitution(
        [FindPackageShare("silverhand_rover_control"), "config", "controllers_imu.yaml"]
    )
    controllers_wheel_file = PathJoinSubstitution(
        [FindPackageShare("silverhand_rover_control"), "config", "controllers_wheel.yaml"]
    )
    ekf_config_file = PathJoinSubstitution(
        [FindPackageShare("silverhand_rover_control"), "config", "ekf_imu.yaml"]
    )

    robot_description_content = Command(
        [
            PathJoinSubstitution([FindExecutable(name="xacro")]),
            " ",
            description_file,
            " ",
            "use_mock_hardware:=",
            use_mock_hardware,
            " ",
            "can_iface:=",
            can_iface,
            " ",
            "node_id:=",
            node_id,
            " ",
            "queue_len:=",
            queue_len,
            " ",
            "imu_name:=",
            imu_name,
            " ",
            "imu_frame_id:=",
            imu_frame_id,
            " ",
            "imu_device_path:=",
            imu_device_path,
            " ",
            "imu_vid:=",
            imu_vid,
            " ",
            "imu_pid:=",
            imu_pid,
            " ",
            "imu_report_size:=",
            imu_report_size,
        ]
    )
    robot_description = {
        "robot_description": ParameterValue(robot_description_content, value_type=str)
    }

    robot_state_publisher = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        output="screen",
        parameters=[robot_description],
    )

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "use_mock_hardware",
                default_value="true",
                description="Use ros2_control mock hardware instead of the rover stub hardware.",
            ),
            DeclareLaunchArgument(
                "can_iface",
                default_value=str(ros_control_profile["can_iface"]),
                description="Linux CAN interface reserved for the future rover Cyphal transport.",
            ),
            DeclareLaunchArgument(
                "node_id",
                default_value=str(ros_control_profile["node_id"]),
                description="Cyphal node id reserved for the rover ros2_control hardware plugin.",
            ),
            DeclareLaunchArgument(
                "queue_len",
                default_value=str(ros_control_profile["queue_len"]),
                description="Future Cyphal queue length for the rover hardware plugin.",
            ),
            DeclareLaunchArgument(
                "imu_name",
                default_value=str(ros_control_profile["imu_name"]),
                description="ros2_control sensor name exported for the USB IMU.",
            ),
            DeclareLaunchArgument(
                "imu_frame_id",
                default_value=str(ros_control_profile["imu_frame_id"]),
                description="Frame id published by the IMU broadcaster.",
            ),
            DeclareLaunchArgument(
                "imu_device_path",
                default_value=str(ros_control_profile["imu_device_path"]),
                description="Optional /dev/hidrawX path for the IMU. Empty means auto-detect by VID/PID.",
            ),
            DeclareLaunchArgument(
                "imu_vid",
                default_value=str(ros_control_profile["imu_vid"]),
                description="USB vendor id for the HID IMU in decimal form.",
            ),
            DeclareLaunchArgument(
                "imu_pid",
                default_value=str(ros_control_profile["imu_pid"]),
                description="USB product id for the HID IMU in decimal form.",
            ),
            DeclareLaunchArgument(
                "imu_report_size",
                default_value=str(ros_control_profile["imu_report_size"]),
                description="Expected HID report size for IMU packets.",
            ),
            DeclareLaunchArgument(
                "use_imu_odometry",
                default_value=str(ros_control_profile["use_imu_odometry"]),
                description="IMU odometry mode: auto, true, or false.",
            ),
            robot_state_publisher,
            OpaqueFunction(
                function=lambda context: _create_runtime_actions(
                    context,
                    robot_description,
                    controllers_imu_file,
                    controllers_wheel_file,
                    ekf_config_file,
                )
            ),
        ]
    )
