#include "utils/TSignal.hpp"
#include "utils/TTypeRedef.hpp"

#include "mqtt/async_client.h"
#include "mqtt/callback.h"

#include <memory>

namespace gentau {
class TMqttClient
{
  public:
    using sharedPtr = std::shared_ptr<TMqttClient>;

  public:
    TSignal<TMqttClient> onConnected;
    TSignal<TMqttClient> onConnectionLost;
};

}  // namespace gentau
