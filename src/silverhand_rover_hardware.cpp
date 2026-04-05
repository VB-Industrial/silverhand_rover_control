#include "silverhand_rover_control/silverhand_rover_hardware.hpp"

#include <cstddef>
#include <exception>
#include <string>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "pluginlib/class_list_macros.hpp"
#include "rclcpp/rclcpp.hpp"

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

}  // namespace

namespace silverhand_rover_control
{

CallbackReturn SilverhandRoverSystem::on_init(
  const hardware_interface::HardwareComponentInterfaceParams & params)
{
  if (hardware_interface::SystemInterface::on_init(params) != CallbackReturn::SUCCESS) {
    return CallbackReturn::ERROR;
  }

  can_iface_ = get_string_param(info_, "can_iface", "vcan1");
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
  is_configured_ = false;
  is_active_ = false;

  RCLCPP_INFO(
    get_logger(),
    "Initialized rover hardware '%s' with can_iface=%s node_id=%u queue_len=%zu",
    info_.name.c_str(), can_iface_.c_str(), node_id_, queue_len_);

  return CallbackReturn::SUCCESS;
}

CallbackReturn SilverhandRoverSystem::on_configure(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  is_configured_ = true;
  RCLCPP_INFO(
    get_logger(),
    "Configured rover hardware '%s' in stub mode on interface %s",
    info_.name.c_str(), can_iface_.c_str());
#ifdef SILVERHAND_ROVER_HAS_LIBCXXCANARD
  RCLCPP_INFO(get_logger(), "libcxxcanard sources detected and linked");
#else
  RCLCPP_WARN(get_logger(), "libcxxcanard sources not present yet, running pure stub backend");
#endif
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
  RCLCPP_INFO(
    get_logger(),
    "Activated rover hardware '%s' in stub mode, target interface %s node_id=%u",
    info_.name.c_str(), can_iface_.c_str(), node_id_);
  return CallbackReturn::SUCCESS;
}

CallbackReturn SilverhandRoverSystem::on_deactivate(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  is_active_ = false;
  RCLCPP_INFO(get_logger(), "Deactivated rover hardware '%s'", info_.name.c_str());
  return CallbackReturn::SUCCESS;
}

CallbackReturn SilverhandRoverSystem::on_cleanup(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  is_active_ = false;
  is_configured_ = false;
  RCLCPP_INFO(get_logger(), "Cleaned up rover hardware '%s'", info_.name.c_str());
  return CallbackReturn::SUCCESS;
}

std::vector<hardware_interface::StateInterface> SilverhandRoverSystem::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> state_interfaces;
  state_interfaces.reserve(info_.joints.size() * 2U);

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
  const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
  if (!is_configured_) {
    return hardware_interface::return_type::ERROR;
  }

  return hardware_interface::return_type::OK;
}

hardware_interface::return_type SilverhandRoverSystem::write(
  const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
  if (!is_active_) {
    return hardware_interface::return_type::OK;
  }

  std::string command_summary;
  for (std::size_t i = 0; i < wheel_velocity_command_.size(); ++i) {
    if (!command_summary.empty()) {
      command_summary += ", ";
    }
    command_summary += info_.joints[i].name + "=" +
      std::to_string(wheel_velocity_command_[i]);
  }

  RCLCPP_INFO_THROTTLE(
    get_logger(),
    *get_clock(),
    2000,
    "Stub rover write on %s node_id=%u: %s",
    can_iface_.c_str(),
    node_id_,
    command_summary.c_str());

  return hardware_interface::return_type::OK;
}

}  // namespace silverhand_rover_control

PLUGINLIB_EXPORT_CLASS(
  silverhand_rover_control::SilverhandRoverSystem,
  hardware_interface::SystemInterface)
