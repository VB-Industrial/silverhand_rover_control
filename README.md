# silverhand_rover_control

ROS 2 Jazzy workspace for the Silverhand rover lower and middle layers.

Packages:
- `silverhand_rover_description`
- `silverhand_rover_hardware`
- `silverhand_rover_bringup`

## Prerequisites

```bash
sudo apt-get update
sudo apt-get install -y \
  ros-jazzy-ros2-control \
  ros-jazzy-ros2-controllers \
  ros-jazzy-controller-manager \
  ros-jazzy-diff-drive-controller \
  ros-jazzy-joint-state-broadcaster \
  ros-jazzy-robot-state-publisher \
  ros-jazzy-xacro
```

## Clone

Clone with submodules:

```bash
git clone --recurse-submodules <repo-url>
```

If the repository is already cloned:

```bash
git submodule update --init --recursive
```

The real Cyphal backend is expected at:

```bash
/home/r/silver_ws/src/silverhand_rover_control/third_party/libcxxcanard
```

## Workspace Layout

Standalone:

```bash
/home/r/silver_ws/src/silverhand_rover_control
```

Shared workspace:

```bash
/home/r/silver_ws/src/silverhand_ros2
/home/r/silver_ws/src/silverhand_rover_control
/home/r/silver_ws/src/silverhand_moveit2
```

## Build

```bash
cd /home/r/silver_ws
source /opt/ros/jazzy/setup.bash
colcon build --packages-up-to \
  silverhand_rover_description \
  silverhand_rover_hardware \
  silverhand_rover_bringup
source /home/r/silver_ws/install/setup.bash
```

## Packages Check

```bash
ros2 pkg list | rg silverhand_rover
```

## Launch

Mock hardware:

```bash
ros2 launch silverhand_rover_bringup silverhand_rover_mock.launch.py
```

Stub real hardware:

```bash
ros2 launch silverhand_rover_bringup silverhand_rover_real.launch.py can_iface:=vcan1 node_id:=110
```

Generic bringup:

```bash
ros2 launch silverhand_rover_bringup silverhand_rover_bringup.launch.py \
  use_mock_hardware:=true
```

## Parameters

- `use_mock_hardware`: use `mock_components/GenericSystem` for permanent debug bringup
- `can_iface`: CAN or VCAN interface for the future Cyphal transport, default `vcan1`
- `node_id`: Cyphal node id for the rover hardware plugin, default `110`
- `queue_len`: reserved queue length for the future Cyphal transport, default `1000`

## Notes

- The current rover URDF is intentionally temporary and uses a simple box base with six wheel joints.
- The Cyphal transport is vendored as the `third_party/libcxxcanard` git submodule.
- The real hardware plugin currently logs lifecycle, read, and write activity instead of talking to the rover electronics.
- `diff_drive_controller` is used as the first integration step. A custom rover controller can replace it later without changing the package split.
