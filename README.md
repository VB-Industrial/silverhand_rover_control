# silverhand_rover_control

ROS 2 Jazzy stack for the Silverhand rover control layer.

Packages:
- `silverhand_rover_description` - thin wrapper over `silverhand_rover_model` that adds `ros2_control`
- `silverhand_rover_hardware` - hardware plugin for mock and future real rover backend
- `silverhand_rover_bringup` - launch files and controller configuration

This repository does not own the visual/kinematic rover model itself.
The base model lives in the separate `silverhand_rover_model` repository and is included from there.

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

Clone the control stack with submodules:

```bash
cd ~/silver_ws/src
git clone --recurse-submodules <silverhand_rover_control_repo_url>
```

If the repository is already cloned:

```bash
git submodule update --init --recursive
```

The real Cyphal backend is expected at:

```bash
/home/r/silver_ws/src/silverhand_rover_control/third_party/libcxxcanard
```

Clone the rover model next to it in the same workspace:

```bash
cd ~/silver_ws/src
git clone https://github.com/VB-Industrial/silverhand_rover_model.git
```

## Workspace Layout

Minimal shared workspace for bringup:

```bash
/home/r/silver_ws/src/silverhand_rover_model
/home/r/silver_ws/src/silverhand_rover_control
```

Extended workspace:

```bash
/home/r/silver_ws/src/silverhand_ros2
/home/r/silver_ws/src/silverhand_rover_control
/home/r/silver_ws/src/silverhand_moveit2
```

## Build

```bash
cd ~/silver_ws
source /opt/ros/jazzy/setup.bash
colcon build --packages-up-to \
  silverhand_rover_model \
  silverhand_rover_description \
  silverhand_rover_hardware \
  silverhand_rover_bringup
source ~/silver_ws/install/setup.bash
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

- `silverhand_rover_description` does not duplicate the rover model. It includes `silverhand_rover_model/urdf/silverhand_rover.urdf.xacro` and appends the `ros2_control` block.
- The Cyphal transport is vendored as the `third_party/libcxxcanard` git submodule.
- The real hardware plugin currently logs lifecycle, read, and write activity instead of talking to the rover electronics.
- `diff_drive_controller` is used as the first integration step. A custom rover controller can replace it later without changing the package split.
