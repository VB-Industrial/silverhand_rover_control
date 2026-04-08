#ifndef SILVERHAND_ROVER_CONTROL__CYPHAL__MOTOR_IO_HPP_
#define SILVERHAND_ROVER_CONTROL__CYPHAL__MOTOR_IO_HPP_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rclcpp/logger.hpp"

class CyphalInterface;

namespace silverhand_rover_control::cyphal
{

struct MotorChannelConfig
{
  std::string joint_name;
  std::uint16_t motor_id{0};
  std::uint16_t command_port_id{0};
  std::uint16_t feedback_port_id{0};
};

class MotorIo
{
public:
  MotorIo();
  ~MotorIo();

  bool configure(const std::vector<MotorChannelConfig> & channels, rclcpp::Logger logger);

  bool attach_interface(const std::shared_ptr<CyphalInterface> & interface, rclcpp::Logger logger);

  void detach_interface();

  void copy_states(
    std::vector<double> & wheel_position_state,
    std::vector<double> & wheel_velocity_state,
    double period_seconds);

  void publish_commands(const std::vector<double> & wheel_velocity_command);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace silverhand_rover_control::cyphal

#endif  // SILVERHAND_ROVER_CONTROL__CYPHAL__MOTOR_IO_HPP_
