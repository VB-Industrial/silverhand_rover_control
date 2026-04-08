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

exec ros2 launch silverhand_rover_control silverhand_rover_mock.launch.py \
  can_iface:="${SILVERHAND_ROVER_CAN_IFACE:-vcan1}" \
  node_id:="${SILVERHAND_ROVER_NODE_ID:-110}" \
  queue_len:="${SILVERHAND_ROVER_QUEUE_LEN:-1000}"
