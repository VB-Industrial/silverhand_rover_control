#ifndef SILVERHAND_ROVER_CONTROL__CYPHAL__RUNTIME_HPP_
#define SILVERHAND_ROVER_CONTROL__CYPHAL__RUNTIME_HPP_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

class CyphalInterface;

namespace silverhand_rover_control::cyphal
{

class Runtime
{
public:
  Runtime();
  ~Runtime();

  bool start(const std::string & can_iface, std::uint16_t node_id, std::size_t queue_len);

  void stop();

  void spin_once() const;

  bool is_started() const;

  void publish_heartbeat();

  std::shared_ptr<CyphalInterface> interface() const;

private:
  std::shared_ptr<CyphalInterface> interface_;
  std::uint32_t uptime_seconds_{0};
  std::uint8_t heartbeat_transfer_id_{0};
};

}  // namespace silverhand_rover_control::cyphal

#endif  // SILVERHAND_ROVER_CONTROL__CYPHAL__RUNTIME_HPP_
