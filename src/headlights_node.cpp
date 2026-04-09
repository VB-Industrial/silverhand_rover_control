#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>

#include "cyphal/cyphal.h"
#include "rclcpp/rclcpp.hpp"
#include "silverhand_rover_control/cyphal/runtime.hpp"
#include "std_srvs/srv/set_bool.hpp"
#include "uavcan/si/unit/angular_velocity/Scalar_1_0.h"

TYPE_ALIAS(AngularVelocityMsg, uavcan_si_unit_angular_velocity_Scalar_1_0)

namespace
{

constexpr auto kSpinPeriod = std::chrono::milliseconds(50);
constexpr std::size_t kHeartbeatLoopCount = 20U;

class HeadlightsNode : public rclcpp::Node
{
public:
  HeadlightsNode()
  : Node("headlights_node")
  {
    declare_parameter<bool>("use_mock", false);
    declare_parameter<std::string>("can_iface", "vcan1");
    declare_parameter<int>("node_id", 111);
    declare_parameter<int>("queue_len", 1000);
    declare_parameter<int>("headlights_port_id", 1000);

    use_mock_ = get_parameter("use_mock").as_bool();
    headlights_port_id_ = static_cast<std::uint16_t>(get_parameter("headlights_port_id").as_int());

    headlights_service_ = create_service<std_srvs::srv::SetBool>(
      "/power_board/set_headlights",
      std::bind(&HeadlightsNode::handle_headlights, this, std::placeholders::_1, std::placeholders::_2));

    if (use_mock_) {
      RCLCPP_INFO(get_logger(), "Starting headlights_node in mock mode; hardware access is disabled");
      return;
    }

    const auto can_iface = get_parameter("can_iface").as_string();
    const auto node_id = static_cast<std::uint16_t>(get_parameter("node_id").as_int());
    const auto queue_len = static_cast<std::size_t>(get_parameter("queue_len").as_int());

    runtime_ = std::make_unique<silverhand_rover_control::cyphal::Runtime>();
    if (!runtime_->start(can_iface, node_id, queue_len)) {
      throw std::runtime_error("Failed to start Cyphal runtime for headlights_node");
    }

    timer_ = create_wall_timer(kSpinPeriod, std::bind(&HeadlightsNode::spin_once, this));
  }

  ~HeadlightsNode() override
  {
    shutdown_runtime();
  }

private:
  void shutdown_runtime()
  {
    if (timer_) {
      timer_->cancel();
      timer_.reset();
    }
    headlights_service_.reset();
    if (runtime_) {
      runtime_->stop();
      runtime_.reset();
    }
  }

  void spin_once()
  {
    if (!runtime_) {
      return;
    }

    runtime_->spin_once();
    ++loop_counter_;
    if (loop_counter_ >= kHeartbeatLoopCount) {
      runtime_->publish_heartbeat();
      loop_counter_ = 0U;
    }
  }

  void handle_headlights(
    const std::shared_ptr<std_srvs::srv::SetBool::Request> request,
    std::shared_ptr<std_srvs::srv::SetBool::Response> response)
  {
    if (use_mock_) {
      response->success = true;
      response->message = request->data ? "Mock headlights enabled" : "Mock headlights disabled";
      return;
    }

    if (!runtime_ || !runtime_->is_started()) {
      response->success = false;
      response->message = "Headlights Cyphal runtime is not started";
      return;
    }

    AngularVelocityMsg::Type message = {.radian_per_second = request->data ? 1.0F : 0.0F};
    runtime_->interface()->send_msg<AngularVelocityMsg>(&message, headlights_port_id_, &headlights_transfer_id_);
    response->success = true;
    response->message = request->data ? "Headlights enabled" : "Headlights disabled";
  }

  std::unique_ptr<silverhand_rover_control::cyphal::Runtime> runtime_;
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Service<std_srvs::srv::SetBool>::SharedPtr headlights_service_;
  std::uint16_t headlights_port_id_{1000};
  bool use_mock_{false};
  std::size_t loop_counter_{0};
  CanardTransferID headlights_transfer_id_{0};
};

}  // namespace

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<HeadlightsNode>();
  rclcpp::spin(node);
  node.reset();
  rclcpp::shutdown();
  return 0;
}
