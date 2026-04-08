#ifndef SILVERHAND_ROVER_CONTROL__SILVERHAND_ROVER_HARDWARE_HPP_
#define SILVERHAND_ROVER_CONTROL__SILVERHAND_ROVER_HARDWARE_HPP_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "hardware_interface/hardware_component_interface.hpp"
#include "hardware_interface/system_interface.hpp"
#include "hardware_interface/types/hardware_interface_return_values.hpp"
#include "rclcpp_lifecycle/node_interfaces/lifecycle_node_interface.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "silverhand_rover_control/visibility_control.h"

namespace silverhand_rover_control
{

using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

namespace cyphal
{
class MotorIo;
class Runtime;
}  // namespace cyphal

class SILVERHAND_ROVER_CONTROL_PUBLIC SilverhandRoverSystem
  : public hardware_interface::SystemInterface
{
public:
  ~SilverhandRoverSystem() override;

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
  std::vector<double> imu_state_;
  std::string can_iface_{"vcan1"};
  std::string imu_name_{"imu_sensor"};
  std::string imu_frame_id_{"imu_link"};
  std::string imu_device_path_{};
  std::uint16_t imu_vid_{0xcafe};
  std::uint16_t imu_pid_{0x4004};
  std::size_t imu_report_size_{64};
  std::uint16_t motor_command_port_base_{3000};
  std::uint16_t motor_feedback_port_base_{3000};
  void * imu_device_handle_{nullptr};
  std::unique_ptr<cyphal::Runtime> cyphal_runtime_;
  std::unique_ptr<cyphal::MotorIo> motor_io_;
  std::uint16_t node_id_{110};
  std::size_t queue_len_{1000};
  bool is_configured_{false};
  bool is_active_{false};
  bool imu_device_present_{false};
  std::size_t write_cycles_since_heartbeat_{0};
};

}  // namespace silverhand_rover_control

#endif  // SILVERHAND_ROVER_CONTROL__SILVERHAND_ROVER_HARDWARE_HPP_
