#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
ROS_WS="${ROS_WS:-$(cd "${REPO_DIR}/../.." && pwd)}"
ROS_DISTRO="${ROS_DISTRO:-jazzy}"

source "/opt/ros/${ROS_DISTRO}/setup.bash"
source "${ROS_WS}/install/setup.bash"

exec ros2 launch silverhand_rover_control silverhand_rover_bringup.launch.py \
  use_mock_hardware:=false \
  can_iface:="${SILVERHAND_ROVER_CAN_IFACE:-can0}" \
  node_id:="${SILVERHAND_ROVER_NODE_ID:-110}" \
  queue_len:="${SILVERHAND_ROVER_QUEUE_LEN:-1000}"
