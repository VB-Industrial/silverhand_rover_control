#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>

#include "cyphal/cyphal.h"
#include "cyphal/subscriptions/subscription.h"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/battery_state.hpp"
#include "silverhand_rover_control/cyphal/runtime.hpp"
#include "std_srvs/srv/set_bool.hpp"
#include "uavcan/si/unit/angular_velocity/Scalar_1_0.h"
#include "voltbro/battery/state_1_0.h"

TYPE_ALIAS(AngularVelocityMsg, uavcan_si_unit_angular_velocity_Scalar_1_0)
TYPE_ALIAS(BatteryStateMsg, voltbro_battery_state_1_0)

namespace
{

constexpr double kCoulombToAmpereHour = 1.0 / 3600.0;

class PowerBoardNode : public rclcpp::Node
{
public:
  PowerBoardNode()
  : Node("power_board_node"),
    runtime_(std::make_unique<silverhand_rover_control::cyphal::Runtime>())
  {
    declare_parameter<std::string>("can_iface", "vcan1");
    declare_parameter<int>("node_id", 111);
    declare_parameter<int>("queue_len", 1000);
    declare_parameter<int>("power_board_node_id", 9);
    declare_parameter<int>("battery_state_port_id", 7993);
    declare_parameter<int>("headlights_port_id", 1000);
    declare_parameter<std::string>("battery_topic", "/battery_state");

    const auto can_iface = get_parameter("can_iface").as_string();
    const auto node_id = static_cast<std::uint16_t>(get_parameter("node_id").as_int());
    const auto queue_len = static_cast<std::size_t>(get_parameter("queue_len").as_int());
    power_board_node_id_ = static_cast<std::uint16_t>(get_parameter("power_board_node_id").as_int());
    battery_state_port_id_ = static_cast<std::uint16_t>(get_parameter("battery_state_port_id").as_int());
    headlights_port_id_ = static_cast<std::uint16_t>(get_parameter("headlights_port_id").as_int());

    battery_publisher_ = create_publisher<sensor_msgs::msg::BatteryState>(
      get_parameter("battery_topic").as_string(),
      10);

    headlights_service_ = create_service<std_srvs::srv::SetBool>(
      "/power_board/set_headlights",
      std::bind(&PowerBoardNode::handle_headlights, this, std::placeholders::_1, std::placeholders::_2));

    if (!runtime_->start(can_iface, node_id, queue_len)) {
      throw std::runtime_error("Failed to start Cyphal runtime for power_board_node");
    }

    battery_subscription_ = std::make_unique<BatteryReader>(
      runtime_->interface(),
      battery_state_port_id_,
      this);

    timer_ = create_wall_timer(
      std::chrono::milliseconds(10),
      std::bind(&PowerBoardNode::spin_once, this));
  }

private:
  class BatteryReader : public AbstractSubscription<BatteryStateMsg>
  {
  public:
    BatteryReader(
      const InterfacePtr & interface,
      const CanardPortID port_id,
      PowerBoardNode * node)
    : AbstractSubscription<BatteryStateMsg>(interface, port_id),
      node_(node)
    {
    }

    void handler(const BatteryStateMsg::Type & message, CanardRxTransfer * /*transfer*/) override
    {
      if (node_ == nullptr) {
        return;
      }
      node_->publish_battery_state(message);
    }

  private:
    PowerBoardNode * node_;
  };

  void spin_once()
  {
    runtime_->spin_once();
    ++loop_counter_;
    if (loop_counter_ >= 100U) {
      runtime_->publish_heartbeat();
      loop_counter_ = 0U;
    }
  }

  void publish_battery_state(const BatteryStateMsg::Type & message)
  {
    sensor_msgs::msg::BatteryState ros_msg;
    ros_msg.header.stamp = now();
    ros_msg.voltage = message.voltage.volt;
    ros_msg.current = message.current.ampere;
    ros_msg.charge = message.charge.coulomb * kCoulombToAmpereHour;
    ros_msg.capacity = message.capacity.coulomb * kCoulombToAmpereHour;
    ros_msg.design_capacity = message.design_capacity.coulomb * kCoulombToAmpereHour;
    ros_msg.percentage =
      ros_msg.capacity > 0.0F ? static_cast<float>(ros_msg.charge / ros_msg.capacity) : 0.0F;
    ros_msg.present = message.is_present.value != 0;
    ros_msg.power_supply_status = sensor_msgs::msg::BatteryState::POWER_SUPPLY_STATUS_UNKNOWN;
    ros_msg.power_supply_health = sensor_msgs::msg::BatteryState::POWER_SUPPLY_HEALTH_UNKNOWN;
    ros_msg.power_supply_technology =
      sensor_msgs::msg::BatteryState::POWER_SUPPLY_TECHNOLOGY_UNKNOWN;
    battery_publisher_->publish(ros_msg);
  }

  void handle_headlights(
    const std::shared_ptr<std_srvs::srv::SetBool::Request> request,
    std::shared_ptr<std_srvs::srv::SetBool::Response> response)
  {
    if (!runtime_->is_started()) {
      response->success = false;
      response->message = "Cyphal runtime is not started";
      return;
    }

    AngularVelocityMsg::Type message = {
      .radian_per_second = request->data ? 1.0F : 0.0F
    };
    runtime_->interface()->send_msg<AngularVelocityMsg>(
      &message,
      headlights_port_id_,
      &headlights_transfer_id_);
    response->success = true;
    response->message = request->data ? "Headlights enabled" : "Headlights disabled";
  }

  std::unique_ptr<silverhand_rover_control::cyphal::Runtime> runtime_;
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<sensor_msgs::msg::BatteryState>::SharedPtr battery_publisher_;
  rclcpp::Service<std_srvs::srv::SetBool>::SharedPtr headlights_service_;
  std::unique_ptr<BatteryReader> battery_subscription_;
  std::uint16_t power_board_node_id_{9};
  std::uint16_t battery_state_port_id_{7993};
  std::uint16_t headlights_port_id_{1000};
  CanardTransferID headlights_transfer_id_{0};
  std::size_t loop_counter_{0};
};

}  // namespace

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<PowerBoardNode>());
  rclcpp::shutdown();
  return 0;
}
