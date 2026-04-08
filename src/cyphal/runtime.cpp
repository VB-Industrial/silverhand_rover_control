#include "silverhand_rover_control/cyphal/runtime.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>

#include "cyphal/allocators/o1/o1_allocator.h"
#include "cyphal/cyphal.h"
#include "cyphal/providers/LinuxCAN.h"
#include "uavcan/node/Heartbeat_1_0.h"

TYPE_ALIAS(HeartbeatMsg, uavcan_node_Heartbeat_1_0)

namespace
{

void error_handler()
{
  std::exit(EXIT_FAILURE);
}

std::uint64_t micros_64()
{
  using namespace std::chrono;
  return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
}

UtilityConfig g_utilities(micros_64, error_handler);

}  // namespace

namespace silverhand_rover_control::cyphal
{

Runtime::Runtime() = default;

Runtime::~Runtime()
{
  stop();
}

bool Runtime::start(const std::string & can_iface, const std::uint16_t node_id, const std::size_t queue_len)
{
  stop();

  interface_ = CyphalInterface::create_heap<LinuxCAN, O1Allocator>(
    node_id, can_iface.c_str(), queue_len, g_utilities);

  uptime_seconds_ = 0;
  heartbeat_transfer_id_ = 0;
  return interface_ && interface_->is_up();
}

void Runtime::stop()
{
  interface_.reset();
  uptime_seconds_ = 0;
  heartbeat_transfer_id_ = 0;
}

void Runtime::spin_once() const
{
  if (interface_) {
    interface_->loop();
  }
}

bool Runtime::is_started() const
{
  return interface_ && interface_->is_up();
}

void Runtime::publish_heartbeat()
{
  if (!interface_) {
    return;
  }

  HeartbeatMsg::Type heartbeat = {
    .uptime = uptime_seconds_,
    .health = {uavcan_node_Health_1_0_NOMINAL},
    .mode = {uavcan_node_Mode_1_0_OPERATIONAL}
  };

  interface_->send_msg<HeartbeatMsg>(
    &heartbeat,
    uavcan_node_Heartbeat_1_0_FIXED_PORT_ID_,
    &heartbeat_transfer_id_);
  ++uptime_seconds_;
}

std::shared_ptr<CyphalInterface> Runtime::interface() const
{
  return interface_;
}

}  // namespace silverhand_rover_control::cyphal
