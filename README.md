# silverhand_rover_control

ROS 2 Jazzy package for the Silverhand rover control layer.

Package:
- `silverhand_rover_control`

This repository intentionally contains only the lower and middle control layers:
- `ros2_control`
- hardware interface
- controller bringup

Robot geometry, meshes, and base rover visuals live in `silverhand_rover_model`.

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
/home/r/silver_ws/src/silverhand_system_bringup
```

## Build

```bash
cd ~/silver_ws
source /opt/ros/jazzy/setup.bash
colcon build --packages-up-to \
  silverhand_rover_model \
  silverhand_rover_control
source ~/silver_ws/install/setup.bash
```

## Packages Check

```bash
ros2 pkg list | rg silverhand_rover
```

Expected package from this repository:
- `silverhand_rover_control`

## Launch

Mock hardware:

```bash
ros2 launch silverhand_rover_control silverhand_rover_mock.launch.py
```

Stub real hardware:

```bash
ros2 launch silverhand_rover_control silverhand_rover_real.launch.py can_iface:=vcan1 node_id:=110
```

Generic bringup:

```bash
ros2 launch silverhand_rover_control silverhand_rover_bringup.launch.py \
  use_mock_hardware:=true
```

## Helper scripts

```bash
cd /home/r/silver_ws/src/silverhand_rover_control
./scripts/start_rover_mock.sh
./scripts/start_rover_real.sh
```

Поддерживаемые переменные окружения:

- `ROS_WS`
- `ROS_DISTRO`
- `SILVERHAND_ROVER_CAN_IFACE`
- `SILVERHAND_ROVER_NODE_ID`
- `SILVERHAND_ROVER_QUEUE_LEN`

## systemd

User-service template:

- `systemd/user/silverhand-rover-control@.service`

Установка:

```bash
mkdir -p ~/.config/systemd/user
cp /home/r/silver_ws/src/silverhand_rover_control/systemd/user/silverhand-rover-control@.service ~/.config/systemd/user/
systemctl --user daemon-reload
```

Запуск:

```bash
systemctl --user enable --now silverhand-rover-control@mock.service
systemctl --user enable --now silverhand-rover-control@real.service
```

Автозапуск без логина:

```bash
loginctl enable-linger "$USER"
```

Логи:

```bash
journalctl --user -u silverhand-rover-control@mock.service -f
```

## Parameters

- `use_mock_hardware`: use `mock_components/GenericSystem` for permanent debug bringup
- `can_iface`: CAN or VCAN interface for the future Cyphal transport, default `vcan1`
- `node_id`: Cyphal node id for the rover hardware plugin, default `110`
- `queue_len`: reserved queue length for the future Cyphal transport, default `1000`

## Notes

- `silverhand_rover_control` does not duplicate the rover model. It includes `silverhand_rover_model/urdf/silverhand_rover.urdf.xacro` and appends the `ros2_control` block.
- The Cyphal transport is vendored as the `third_party/libcxxcanard` git submodule.
- The real hardware plugin currently logs lifecycle, read, and write activity instead of talking to the rover electronics.
- `diff_drive_controller` is used as the first integration step. A custom rover controller can replace it later without changing the package split.
