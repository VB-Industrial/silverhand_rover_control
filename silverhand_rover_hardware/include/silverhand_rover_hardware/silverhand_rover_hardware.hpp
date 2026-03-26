#ifndef SILVERHAND_ROVER_HARDWARE__SILVERHAND_ROVER_HARDWARE_HPP_
#define SILVERHAND_ROVER_HARDWARE__SILVERHAND_ROVER_HARDWARE_HPP_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "hardware_interface/hardware_component_interface.hpp"
#include "hardware_interface/system_interface.hpp"
#include "hardware_interface/types/hardware_interface_return_values.hpp"
#include "rclcpp_lifecycle/node_interfaces/lifecycle_node_interface.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "silverhand_rover_hardware/visibility_control.h"

namespace silverhand_rover_hardware
{

using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

class SILVERHAND_ROVER_HARDWARE_PUBLIC SilverhandRoverSystem
  : public hardware_interface::SystemInterface
{
public:
  CallbackReturn on_init(
    const hardware_interface::HardwareComponentInterfaceParams & params) override;

  CallbackReturn on_configure(const rclcpp_lifecycle::State & previous_state) override;

  CallbackReturn on_activate(const rclcpp_lifecycle::State & previous_state) override;

  CallbackReturn on_deactivate(const rclcpp_lifecycle::State & previous_state) override;

  CallbackReturn on_cleanup(const rclcpp_lifecycle::State & previous_state) override;

  std::vector<hardware_interface::StateInterface> export_state_interfaces() override;

  std::vector<hardware_interface::CommandInterface> export_command_interfaces() override;

  hardware_interface::return_type read(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

  hardware_interface::return_type write(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

private:
  std::vector<double> wheel_velocity_command_;
  std::vector<double> wheel_position_state_;
  std::vector<double> wheel_velocity_state_;
  std::string can_iface_{"vcan1"};
  std::uint16_t node_id_{110};
  std::size_t queue_len_{1000};
  bool is_configured_{false};
  bool is_active_{false};
};

}  // namespace silverhand_rover_hardware

#endif  // SILVERHAND_ROVER_HARDWARE__SILVERHAND_ROVER_HARDWARE_HPP_
