#include "silverhand_rover_control/cyphal/motor_io.hpp"

#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cyphal/cyphal.h"
#include "cyphal/subscriptions/subscription.h"
#include "rclcpp/rclcpp.hpp"
#include "uavcan/si/unit/angular_velocity/Scalar_1_0.h"

TYPE_ALIAS(AngularVelocityMsg, uavcan_si_unit_angular_velocity_Scalar_1_0)

namespace silverhand_rover_control::cyphal
{

struct MotorIo::Impl
{
  struct ChannelState
  {
    MotorChannelConfig config;
    double latest_velocity{0.0};
    bool has_feedback{false};
    CanardTransferID transfer_id{0};
  };

  class VelocityFeedbackReader : public AbstractSubscription<AngularVelocityMsg>
  {
  public:
    VelocityFeedbackReader(
      const InterfacePtr & interface,
      const CanardPortID port_id,
      ChannelState * channel)
    : AbstractSubscription<AngularVelocityMsg>(interface, port_id),
      channel_(channel)
    {
    }

    void handler(const AngularVelocityMsg::Type & message, CanardRxTransfer * /*transfer*/) override
    {
      if (channel_ == nullptr) {
        return;
      }

      channel_->latest_velocity = static_cast<double>(message.radian_per_second);
      channel_->has_feedback = true;
    }

  private:
    ChannelState * channel_;
  };

  rclcpp::Logger logger{rclcpp::get_logger("silverhand_rover_control.motor_io")};
  std::shared_ptr<CyphalInterface> interface;
  std::vector<ChannelState> channels;
  std::vector<std::unique_ptr<VelocityFeedbackReader>> readers;
};

MotorIo::MotorIo()
: impl_(std::make_unique<Impl>())
{
}

MotorIo::~MotorIo() = default;

bool MotorIo::configure(const std::vector<MotorChannelConfig> & channels, const rclcpp::Logger logger)
{
  impl_->logger = logger.get_child("motor_io");
  impl_->channels.clear();
  impl_->readers.clear();

  if (channels.empty()) {
    RCLCPP_ERROR(impl_->logger, "MotorIo requires at least one configured motor channel");
    return false;
  }

  impl_->channels.reserve(channels.size());
  for (const auto & channel : channels) {
    if (channel.motor_id == 0U) {
      RCLCPP_ERROR(
        impl_->logger,
        "Joint '%s' has invalid motor_id=0",
        channel.joint_name.c_str());
      impl_->channels.clear();
      return false;
    }
    impl_->channels.push_back(Impl::ChannelState{channel});
  }

  return true;
}

bool MotorIo::attach_interface(
  const std::shared_ptr<CyphalInterface> & interface,
  const rclcpp::Logger logger)
{
  impl_->logger = logger.get_child("motor_io");
  impl_->interface = interface;
  impl_->readers.clear();

  if (!impl_->interface) {
    RCLCPP_ERROR(impl_->logger, "Cannot attach MotorIo without a Cyphal interface");
    return false;
  }

  impl_->readers.reserve(impl_->channels.size());
  for (auto & channel : impl_->channels) {
    channel.has_feedback = false;
    channel.latest_velocity = 0.0;
    channel.transfer_id = 0;
    impl_->readers.push_back(
      std::make_unique<Impl::VelocityFeedbackReader>(
        impl_->interface,
        channel.config.feedback_port_id,
        &channel));
  }

  return true;
}

void MotorIo::detach_interface()
{
  impl_->readers.clear();
  impl_->interface.reset();
}

void MotorIo::copy_states(
  std::vector<double> & wheel_position_state,
  std::vector<double> & wheel_velocity_state,
  const double period_seconds)
{
  const auto count = std::min(
    std::min(impl_->channels.size(), wheel_velocity_state.size()),
    wheel_position_state.size());

  for (std::size_t i = 0; i < count; ++i) {
    if (impl_->channels[i].has_feedback) {
      wheel_velocity_state[i] = impl_->channels[i].latest_velocity;
    }

    if (std::isfinite(wheel_velocity_state[i])) {
      wheel_position_state[i] += wheel_velocity_state[i] * period_seconds;
    }
  }
}

void MotorIo::publish_commands(const std::vector<double> & wheel_velocity_command)
{
  if (!impl_->interface) {
    return;
  }

  const auto count = std::min(impl_->channels.size(), wheel_velocity_command.size());
  for (std::size_t i = 0; i < count; ++i) {
    AngularVelocityMsg::Type message = {
      .radian_per_second = static_cast<float>(wheel_velocity_command[i])
    };
    impl_->interface->send_msg<AngularVelocityMsg>(
      &message,
      impl_->channels[i].config.command_port_id,
      &impl_->channels[i].transfer_id);
  }
}

}  // namespace silverhand_rover_control::cyphal
