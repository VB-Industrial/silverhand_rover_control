# silverhand_rover_control

Пакет ROS 2 Jazzy для слоя управления rover SilverHand.

Пакет:
- `silverhand_rover_control`

В этом репозитории намеренно оставлены только нижний и средний слои управления:
- `ros2_control`
- hardware interface
- controller bringup

Геометрия робота, меши и базовая визуализация rover живут в `silverhand_rover_model`.

## Требования

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

## Клонирование

Клонируйте control-стек в workspace, где уже есть `libcxxcanard`:

```bash
cd ~/silver_ws/src
git clone https://github.com/VB-Industrial/libcxxcanard.git
git clone <silverhand_rover_control_repo_url>
```

Рядом в том же workspace клонируйте модель rover:

```bash
cd ~/silver_ws/src
git clone https://github.com/VB-Industrial/silverhand_rover_model.git
```

Ожидаемый путь к реальной Cyphal-серверной части:

```bash
~/silver_ws/src/libcxxcanard
```

## Структура workspace

Минимальный общий workspace для bringup:

```bash
~/silver_ws/src/silverhand_rover_model
~/silver_ws/src/libcxxcanard
~/silver_ws/src/silverhand_rover_control
```

Расширенный workspace:

```bash
~/silver_ws/src/silverhand_rover_model
~/silver_ws/src/libcxxcanard
~/silver_ws/src/silverhand_rover_control
~/silver_ws/src/silverhand_system_bringup
~/silver_ws/src/silverhand_system_description
```

## Сборка

```bash
cd ~/silver_ws
source /opt/ros/jazzy/setup.bash
colcon build --packages-up-to \
  silverhand_rover_model \
  silverhand_rover_control
source ~/silver_ws/install/setup.bash
```

## Проверка пакетов

```bash
ros2 pkg list | rg silverhand_rover
```

Ожидаемый пакет из этого репозитория:
- `silverhand_rover_control`

## Запуск

Mock-режим:

```bash
ros2 launch silverhand_rover_control silverhand_rover_mock.launch.py
```

Этот запуск поднимает:
- `ros2_control` на `mock_components/GenericSystem`
- `power_board_node` в mock-режиме без CAN/железа
- mock `BatteryState` публикуется с периодом `50 ms` (`20 Hz`) по умолчанию

Заглушка для реального железа:

```bash
ros2 launch silverhand_rover_control silverhand_rover_real.launch.py can_iface:=vcan1 node_id:=110
```

Реальное железо с принудительным fallback на wheel odometry:

```bash
ros2 launch silverhand_rover_control silverhand_rover_real.launch.py \
  use_imu_odometry:=false
```

Универсальный bringup:

```bash
ros2 launch silverhand_rover_control silverhand_rover_bringup.launch.py \
  use_mock_hardware:=true
```

## Вспомогательные скрипты

```bash
cd ~/silver_ws/src/silverhand_rover_control
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

Шаблон systemd-сервиса:

- `systemd/system/silverhand-rover-control@.service`

Установка:

```bash
sudo install -Dm644 systemd/system/silverhand-rover-control@.service /etc/systemd/system/silverhand-rover-control@.service
sudo systemctl daemon-reload
```

Запуск:

```bash
sudo systemctl enable --now silverhand-rover-control@mock.service
sudo systemctl enable --now silverhand-rover-control@real.service
```

Автозапуск без логина не нужен: system-сервис стартует без пользовательской сессии.

Логи:

```bash
journalctl -u silverhand-rover-control@mock.service -f
```

## Параметры

- `use_mock_hardware`: use `mock_components/GenericSystem` for permanent debug bringup
- `can_iface`: CAN or VCAN interface for the future Cyphal transport, default `vcan1`
- `node_id`: Cyphal node id for the rover hardware plugin, default `110`
- `queue_len`: reserved queue length for the future Cyphal transport, default `1000`
- `use_imu_odometry`: `auto`, `true`, or `false` for IMU+EKF versus wheel-only odometry
- `power_board_client_node_id`: Cyphal node id used by `power_board_node`, default `111`
- `power_board_node.use_mock`: publish synthetic battery data and accept headlights commands without Cyphal/CAN access
- `power_board_node.mock_battery_*`: parameters for mock battery telemetry values and publish period

## Тайминги power board

- real `power_board_node`: Cyphal polling loop runs every `50 ms` (`20 Hz`)
- real `power_board_node`: Cyphal heartbeat is emitted at approximately `1 Hz`
- mock `power_board_node`: synthetic battery telemetry is published every `50 ms` (`20 Hz`) by default

## Примечания

- `silverhand_rover_control` does not duplicate the rover model. It includes `silverhand_rover_model/urdf/silverhand_rover.urdf.xacro` and appends the `ros2_control` block.
- `libcxxcanard` - отдельная зависимость workspace, его следует клонировать в `~/silver_ws/src/libcxxcanard`.
- Реальный аппаратный плагин теперь ожидает команды на моторы колес на subject'ах `3000 + motor_id`, а обратную связь - на `3000 - motor_id`.
- `power_board_node` is a separate Cyphal-facing ROS node for battery telemetry and headlights, keeping power/HMI concerns outside `ros2_control`.
- `diff_drive_controller` is used as the first integration step. A custom rover controller can replace it later without changing the package split.
