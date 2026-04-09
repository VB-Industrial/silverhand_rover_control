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

Clone the control stack into a workspace that already contains `libcxxcanard`:

```bash
cd ~/silver_ws/src
git clone https://github.com/VB-Industrial/libcxxcanard.git
git clone <silverhand_rover_control_repo_url>
```

Clone the rover model next to it in the same workspace:

```bash
cd ~/silver_ws/src
git clone https://github.com/VB-Industrial/silverhand_rover_model.git
```

The real Cyphal backend is expected at:

```bash
/home/r/silver_ws/src/libcxxcanard
```

## Workspace Layout

Minimal shared workspace for bringup:

```bash
/home/r/silver_ws/src/silverhand_rover_model
/home/r/silver_ws/src/libcxxcanard
/home/r/silver_ws/src/silverhand_rover_control
```

Extended workspace:

```bash
/home/r/silver_ws/src/silverhand_ros2
/home/r/silver_ws/src/libcxxcanard
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

Этот запуск поднимает:
- `ros2_control` на `mock_components/GenericSystem`
- `power_board_node` в mock-режиме без CAN/железа
- mock `BatteryState` публикуется с периодом `50 ms` (`20 Hz`) по умолчанию

Stub real hardware:

```bash
ros2 launch silverhand_rover_control silverhand_rover_real.launch.py can_iface:=vcan1 node_id:=110
```

Real hardware with forced wheel odometry fallback:

```bash
ros2 launch silverhand_rover_control silverhand_rover_real.launch.py \
  use_imu_odometry:=false
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
- `use_imu_odometry`: `auto`, `true`, or `false` for IMU+EKF versus wheel-only odometry
- `power_board_client_node_id`: Cyphal node id used by `power_board_node`, default `111`
- `power_board_node.use_mock`: publish synthetic battery data and accept headlights commands without Cyphal/CAN access
- `power_board_node.mock_battery_*`: parameters for mock battery telemetry values and publish period

## Power Board Timing

- real `power_board_node`: Cyphal polling loop runs every `50 ms` (`20 Hz`)
- real `power_board_node`: Cyphal heartbeat is emitted at approximately `1 Hz`
- mock `power_board_node`: synthetic battery telemetry is published every `50 ms` (`20 Hz`) by default

## Notes

- `silverhand_rover_control` does not duplicate the rover model. It includes `silverhand_rover_model/urdf/silverhand_rover.urdf.xacro` and appends the `ros2_control` block.
- `libcxxcanard` is a separate workspace prerequisite and should be cloned into `~/silver_ws/src/libcxxcanard`.
- The real hardware plugin now expects wheel motor commands on subjects `3000 + motor_id` and wheel feedback on subjects `3000 - motor_id`.
- `power_board_node` is a separate Cyphal-facing ROS node for battery telemetry and headlights, keeping power/HMI concerns outside `ros2_control`.
- `diff_drive_controller` is used as the first integration step. A custom rover controller can replace it later without changing the package split.
