#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
ROS_WS="${ROS_WS:-$(cd "${REPO_DIR}/../.." && pwd)}"
ROS_DISTRO="${ROS_DISTRO:-jazzy}"

set +u
source "/opt/ros/${ROS_DISTRO}/setup.bash"
source "${ROS_WS}/install/setup.bash"
set -u

exec ros2 launch silverhand_rover_control silverhand_rover_real.launch.py \
  can_iface:="${SILVERHAND_ROVER_CAN_IFACE:-can0}" \
  power_board_can_iface:="${SILVERHAND_ROVER_POWER_BOARD_CAN_IFACE:-${SILVERHAND_ROVER_CAN_IFACE:-can0}}" \
  headlights_can_iface:="${SILVERHAND_ROVER_HEADLIGHTS_CAN_IFACE:-${SILVERHAND_ROVER_POWER_BOARD_CAN_IFACE:-${SILVERHAND_ROVER_CAN_IFACE:-can0}}}" \
  node_id:="${SILVERHAND_ROVER_NODE_ID:-110}" \
  queue_len:="${SILVERHAND_ROVER_QUEUE_LEN:-1000}" \
  use_imu_odometry:="${SILVERHAND_ROVER_USE_IMU_ODOMETRY:-auto}" \
  use_power_board:="${SILVERHAND_ROVER_USE_POWER_BOARD:-true}" \
  power_board_client_node_id:="${SILVERHAND_ROVER_POWER_BOARD_CLIENT_NODE_ID:-111}"
