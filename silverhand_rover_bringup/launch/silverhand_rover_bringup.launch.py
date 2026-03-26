from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import Command, FindExecutable, LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    use_mock_hardware = LaunchConfiguration("use_mock_hardware")
    can_iface = LaunchConfiguration("can_iface")
    node_id = LaunchConfiguration("node_id")
    queue_len = LaunchConfiguration("queue_len")

    description_file = PathJoinSubstitution(
        [FindPackageShare("silverhand_rover_description"), "urdf", "silverhand_rover.urdf.xacro"]
    )
    controllers_file = PathJoinSubstitution(
        [FindPackageShare("silverhand_rover_bringup"), "config", "controllers.yaml"]
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
        ]
    )
    robot_description = {"robot_description": robot_description_content}

    robot_state_publisher = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        output="screen",
        parameters=[robot_description],
    )

    ros2_control_node = Node(
        package="controller_manager",
        executable="ros2_control_node",
        output="screen",
        parameters=[robot_description, controllers_file],
        remappings=[("/controller_manager/robot_description", "/robot_description")],
    )

    joint_state_broadcaster_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["joint_state_broadcaster", "--controller-manager", "/controller_manager"],
        output="screen",
    )

    rover_base_controller_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["rover_base_controller", "--controller-manager", "/controller_manager"],
        output="screen",
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
                default_value="vcan1",
                description="Linux CAN interface reserved for the future rover Cyphal transport.",
            ),
            DeclareLaunchArgument(
                "node_id",
                default_value="110",
                description="Cyphal node id reserved for the rover ros2_control hardware plugin.",
            ),
            DeclareLaunchArgument(
                "queue_len",
                default_value="1000",
                description="Future Cyphal queue length for the rover hardware plugin.",
            ),
            robot_state_publisher,
            ros2_control_node,
            joint_state_broadcaster_spawner,
            rover_base_controller_spawner,
        ]
    )

