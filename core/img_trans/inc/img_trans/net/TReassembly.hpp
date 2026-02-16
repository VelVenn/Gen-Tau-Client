#include "utils/TTypeRedef.hpp"

#include "readerwritercircularbuffer.h"

namespace gentau {
constexpr u64 MTU_LEN         = 1400;
constexpr u64 NET_CACHE_DEPTH = 128;

class TReassembly
{
  public:
	using SharedPtr = std::shared_ptr<TReassembly>;
};
}  // namespace gentau