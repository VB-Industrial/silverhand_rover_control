#include "silverhand_rover_control/silverhand_rover_hardware.hpp"

#include <cstddef>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <exception>
#include <limits>
#include <string>
#include <vector>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "pluginlib/class_list_macros.hpp"
#include "rclcpp/rclcpp.hpp"
#include "silverhand_rover_control/cyphal/motor_io.hpp"
#include "silverhand_rover_control/cyphal/runtime.hpp"

#if __has_include(<hidapi/hidapi.h>)
#include <hidapi/hidapi.h>
#define SILVERHAND_ROVER_HAS_HIDAPI 1
#elif __has_include(<SDL2/SDL_hidapi.h>)
#include <SDL2/SDL_hidapi.h>
#define SILVERHAND_ROVER_HAS_HIDAPI 1
#else
#define SILVERHAND_ROVER_HAS_HIDAPI 0
#endif

namespace
{

std::string get_string_param(
  const hardware_interface::HardwareInfo & info,
  const std::string & key,
  const std::string & default_value)
{
  const auto it = info.hardware_parameters.find(key);
  if (it == info.hardware_parameters.end()) {
    return default_value;
  }
  return it->second;
}

std::uint16_t get_uint16_param(
  const hardware_interface::HardwareInfo & info,
  const std::string & key,
  const std::uint16_t default_value)
{
  const auto it = info.hardware_parameters.find(key);
  if (it == info.hardware_parameters.end()) {
    return default_value;
  }

  try {
    return static_cast<std::uint16_t>(std::stoi(it->second));
  } catch (const std::exception &) {
    RCLCPP_WARN(
      rclcpp::get_logger("silverhand_rover_control"),
      "Failed to parse hardware parameter '%s', using default value %u",
      key.c_str(), default_value);
    return default_value;
  }
}

std::size_t get_size_t_param(
  const hardware_interface::HardwareInfo & info,
  const std::string & key,
  const std::size_t default_value)
{
  const auto it = info.hardware_parameters.find(key);
  if (it == info.hardware_parameters.end()) {
    return default_value;
  }

  try {
    return static_cast<std::size_t>(std::stoul(it->second));
  } catch (const std::exception &) {
    RCLCPP_WARN(
      rclcpp::get_logger("silverhand_rover_control"),
      "Failed to parse hardware parameter '%s', using default value %zu",
      key.c_str(), default_value);
    return default_value;
  }
}

std::uint16_t get_joint_uint16_param(
  const hardware_interface::ComponentInfo & joint,
  const std::string & key,
  const std::uint16_t default_value)
{
  const auto it = joint.parameters.find(key);
  if (it == joint.parameters.end()) {
    return default_value;
  }

  try {
    return static_cast<std::uint16_t>(std::stoi(it->second));
  } catch (const std::exception &) {
    return default_value;
  }
}

void initialize_imu_state(std::vector<double> & imu_state)
{
  constexpr std::size_t imu_state_interface_count = 10U;
  imu_state.assign(imu_state_interface_count, std::numeric_limits<double>::quiet_NaN());

  if (imu_state.size() >= 4U) {
    imu_state[0] = 0.0;
    imu_state[1] = 0.0;
    imu_state[2] = 0.0;
    imu_state[3] = 1.0;
  }
}

#if SILVERHAND_ROVER_HAS_HIDAPI
hid_device * to_hid_device(void * handle)
{
  return static_cast<hid_device *>(handle);
}

int16_t read_int16_le(const uint8_t * data)
{
  int16_t value = 0;
  std::memcpy(&value, data, sizeof(value));
  return value;
}
#endif

}  // namespace

namespace silverhand_rover_control
{

SilverhandRoverSystem::~SilverhandRoverSystem() = default;

CallbackReturn SilverhandRoverSystem::on_init(
  const hardware_interface::HardwareComponentInterfaceParams & params)
{
  if (hardware_interface::SystemInterface::on_init(params) != CallbackReturn::SUCCESS) {
    return CallbackReturn::ERROR;
  }

  can_iface_ = get_string_param(info_, "can_iface", "vcan1");
  imu_name_ = get_string_param(info_, "imu_name", "imu_sensor");
  imu_frame_id_ = get_string_param(info_, "imu_frame_id", "imu_link");
  imu_device_path_ = get_string_param(info_, "imu_device_path", "");
  imu_vid_ = get_uint16_param(info_, "imu_vid", 0xcafe);
  imu_pid_ = get_uint16_param(info_, "imu_pid", 0x4004);
  imu_report_size_ = get_size_t_param(info_, "imu_report_size", 64U);
  motor_command_port_base_ = get_uint16_param(info_, "motor_command_port_base", 3000);
  motor_feedback_port_base_ = get_uint16_param(info_, "motor_feedback_port_base", 3000);
  node_id_ = get_uint16_param(info_, "node_id", 110);
  queue_len_ = get_size_t_param(info_, "queue_len", 1000);

  const auto wheel_count = info_.joints.size();
  if (wheel_count == 0U) {
    RCLCPP_ERROR(get_logger(), "Expected at least one wheel joint for rover hardware");
    return CallbackReturn::ERROR;
  }

  wheel_velocity_command_.assign(wheel_count, 0.0);
  wheel_position_state_.assign(wheel_count, 0.0);
  wheel_velocity_state_.assign(wheel_count, 0.0);
  initialize_imu_state(imu_state_);
  cyphal_runtime_ = std::make_unique<cyphal::Runtime>();
  motor_io_ = std::make_unique<cyphal::MotorIo>();

  std::vector<cyphal::MotorChannelConfig> motor_channels;
  motor_channels.reserve(wheel_count);
  for (const auto & joint : info_.joints) {
    const auto motor_id = get_joint_uint16_param(joint, "motor_id", 0U);
    if (motor_id == 0U) {
      RCLCPP_ERROR(
        get_logger(),
        "Joint '%s' is missing required parameter 'motor_id'",
        joint.name.c_str());
      return CallbackReturn::ERROR;
    }

    motor_channels.push_back(
      {
        joint.name,
        motor_id,
        static_cast<std::uint16_t>(motor_command_port_base_ + motor_id),
        static_cast<std::uint16_t>(motor_feedback_port_base_ - motor_id),
      });
  }

  if (!motor_io_->configure(motor_channels, get_logger())) {
    return CallbackReturn::ERROR;
  }

  is_configured_ = false;
  is_active_ = false;
  imu_device_present_ = false;
  write_cycles_since_heartbeat_ = 0U;

  RCLCPP_INFO(
    get_logger(),
    "Initialized rover hardware '%s' with can_iface=%s node_id=%u queue_len=%zu command_base=%u feedback_base=%u imu_name=%s",
    info_.name.c_str(),
    can_iface_.c_str(),
    node_id_,
    queue_len_,
    motor_command_port_base_,
    motor_feedback_port_base_,
    imu_name_.c_str());

  return CallbackReturn::SUCCESS;
}

CallbackReturn SilverhandRoverSystem::on_configure(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  if (!cyphal_runtime_ || !cyphal_runtime_->start(can_iface_, node_id_, queue_len_)) {
    RCLCPP_ERROR(
      get_logger(),
      "Failed to start Cyphal runtime on interface '%s' with node_id=%u",
      can_iface_.c_str(),
      node_id_);
    return CallbackReturn::ERROR;
  }

  if (!motor_io_ || !motor_io_->attach_interface(cyphal_runtime_->interface(), get_logger())) {
    RCLCPP_ERROR(get_logger(), "Failed to attach motor IO to the Cyphal runtime");
    cyphal_runtime_->stop();
    return CallbackReturn::ERROR;
  }

  imu_device_present_ = false;
#if SILVERHAND_ROVER_HAS_HIDAPI
  if (hid_init() == 0) {
    hid_device_info * imu_info = hid_enumerate(imu_vid_, imu_pid_);
    if (imu_info != nullptr) {
      const char * imu_path = imu_device_path_.empty() ? imu_info->path : imu_device_path_.c_str();
      imu_device_handle_ = hid_open_path(imu_path);
      imu_device_present_ = (imu_device_handle_ != nullptr);
      hid_free_enumeration(imu_info);
      if (!imu_device_present_) {
        RCLCPP_WARN(
          get_logger(),
          "IMU HID device was detected but could not be opened on path '%s'; wheel odometry fallback remains available",
          imu_path);
      }
    } else {
      RCLCPP_WARN(
        get_logger(),
        "IMU HID device vid=0x%04x pid=0x%04x was not found; wheel odometry fallback remains available",
        imu_vid_,
        imu_pid_);
    }
  } else {
    RCLCPP_WARN(get_logger(), "hid_init() failed; IMU will stay unavailable");
  }
#else
  RCLCPP_WARN(get_logger(), "HIDAPI headers are not available; IMU will stay unavailable");
#endif

  is_configured_ = true;
  RCLCPP_INFO(
    get_logger(),
    "Configured rover hardware '%s' on %s, IMU=%s",
    info_.name.c_str(),
    can_iface_.c_str(),
    imu_device_present_ ? "available" : "unavailable");
  return CallbackReturn::SUCCESS;
}

CallbackReturn SilverhandRoverSystem::on_activate(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  if (!is_configured_) {
    RCLCPP_ERROR(get_logger(), "Cannot activate rover hardware before configure");
    return CallbackReturn::ERROR;
  }

  is_active_ = true;
  write_cycles_since_heartbeat_ = 0U;
  RCLCPP_INFO(
    get_logger(),
    "Activated rover hardware '%s' on interface %s node_id=%u",
    info_.name.c_str(),
    can_iface_.c_str(),
    node_id_);
  return CallbackReturn::SUCCESS;
}

CallbackReturn SilverhandRoverSystem::on_deactivate(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  is_active_ = false;
  return CallbackReturn::SUCCESS;
}

CallbackReturn SilverhandRoverSystem::on_cleanup(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
#if SILVERHAND_ROVER_HAS_HIDAPI
  if (imu_device_handle_ != nullptr) {
    hid_close(to_hid_device(imu_device_handle_));
    imu_device_handle_ = nullptr;
  }
  hid_exit();
#endif

  imu_device_present_ = false;
  is_active_ = false;
  is_configured_ = false;
  if (motor_io_) {
    motor_io_->detach_interface();
  }
  if (cyphal_runtime_) {
    cyphal_runtime_->stop();
  }
  return CallbackReturn::SUCCESS;
}

std::vector<hardware_interface::StateInterface> SilverhandRoverSystem::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> state_interfaces;
  state_interfaces.reserve(info_.joints.size() * 2U + imu_state_.size());

  for (std::size_t i = 0; i < info_.joints.size(); ++i) {
    state_interfaces.emplace_back(
      info_.joints[i].name,
      hardware_interface::HW_IF_POSITION,
      &wheel_position_state_[i]);
    state_interfaces.emplace_back(
      info_.joints[i].name,
      hardware_interface::HW_IF_VELOCITY,
      &wheel_velocity_state_[i]);
  }

  if (!info_.sensors.empty()) {
    const auto & imu_sensor = info_.sensors.front();
    for (std::size_t i = 0; i < imu_sensor.state_interfaces.size() && i < imu_state_.size(); ++i) {
      state_interfaces.emplace_back(
        imu_sensor.name,
        imu_sensor.state_interfaces[i].name,
        &imu_state_[i]);
    }
  }

  return state_interfaces;
}

std::vector<hardware_interface::CommandInterface> SilverhandRoverSystem::export_command_interfaces()
{
  std::vector<hardware_interface::CommandInterface> command_interfaces;
  command_interfaces.reserve(info_.joints.size());

  for (std::size_t i = 0; i < info_.joints.size(); ++i) {
    command_interfaces.emplace_back(
      info_.joints[i].name,
      hardware_interface::HW_IF_VELOCITY,
      &wheel_velocity_command_[i]);
  }

  return command_interfaces;
}

hardware_interface::return_type SilverhandRoverSystem::read(
  const rclcpp::Time & /*time*/,
  const rclcpp::Duration & period)
{
  if (!is_configured_) {
    return hardware_interface::return_type::ERROR;
  }

  if (cyphal_runtime_) {
    cyphal_runtime_->spin_once();
  }

  if (motor_io_) {
    motor_io_->copy_states(
      wheel_position_state_,
      wheel_velocity_state_,
      period.seconds());
  }

#if SILVERHAND_ROVER_HAS_HIDAPI
  if (imu_device_present_ && imu_device_handle_ != nullptr) {
    uint8_t buffer[64] = {};
    const auto bytes_number = hid_read(
      to_hid_device(imu_device_handle_),
      buffer,
      static_cast<size_t>(imu_report_size_));

    if (bytes_number < 0) {
      imu_device_present_ = false;
      RCLCPP_ERROR_THROTTLE(
        get_logger(),
        *get_clock(),
        2000,
        "USB IMU read failed; continuing in wheel-only mode");
    } else if (bytes_number >= 28) {
      const auto acc_x = read_int16_le(&buffer[2]);
      const auto acc_y = read_int16_le(&buffer[4]);
      const auto acc_z = read_int16_le(&buffer[6]);
      imu_state_[7] = static_cast<double>(acc_x) * 9.81 / 4096.0;
      imu_state_[8] = static_cast<double>(acc_y) * 9.81 / 4096.0;
      imu_state_[9] = static_cast<double>(acc_z) * 9.81 / 4096.0;

      const auto gyr_x = read_int16_le(&buffer[8]);
      const auto gyr_y = read_int16_le(&buffer[10]);
      const auto gyr_z = read_int16_le(&buffer[12]);
      constexpr double gyro_scale = 6.28 * 2000.0 / 32768.0 / 360.0;
      imu_state_[4] = static_cast<double>(gyr_x) * gyro_scale;
      imu_state_[5] = static_cast<double>(gyr_y) * gyro_scale;
      imu_state_[6] = static_cast<double>(gyr_z) * gyro_scale;

      const auto quat_x = read_int16_le(&buffer[14]);
      const auto quat_y = read_int16_le(&buffer[16]);
      const auto quat_z = read_int16_le(&buffer[18]);
      const auto quat_w = read_int16_le(&buffer[20]);
      constexpr double quat_scale = 1.0 / 16384.0;
      imu_state_[0] = static_cast<double>(quat_x) * quat_scale;
      imu_state_[1] = static_cast<double>(quat_y) * quat_scale;
      imu_state_[2] = static_cast<double>(quat_z) * quat_scale;
      imu_state_[3] = static_cast<double>(quat_w) * quat_scale;
    }
  }
#endif

  return hardware_interface::return_type::OK;
}

hardware_interface::return_type SilverhandRoverSystem::write(
  const rclcpp::Time & /*time*/,
  const rclcpp::Duration & /*period*/)
{
  if (!is_active_) {
    return hardware_interface::return_type::OK;
  }

  if (motor_io_) {
    motor_io_->publish_commands(wheel_velocity_command_);
  }

  ++write_cycles_since_heartbeat_;
  if (cyphal_runtime_ && write_cycles_since_heartbeat_ >= 100U) {
    cyphal_runtime_->publish_heartbeat();
    write_cycles_since_heartbeat_ = 0U;
  }

  if (cyphal_runtime_) {
    cyphal_runtime_->spin_once();
  }

  return hardware_interface::return_type::OK;
}

}  // namespace silverhand_rover_control

PLUGINLIB_EXPORT_CLASS(
  silverhand_rover_control::SilverhandRoverSystem,
  hardware_interface::SystemInterface)
