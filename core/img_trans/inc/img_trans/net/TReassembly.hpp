#pragma once

#include "img_trans/vid_render/TVidRender.hpp"

#include "utils/TTypeRedef.hpp"

#include <atomic>
#include <memory>
#include <span>

namespace gentau {
constexpr u64 MTU_LEN = 1400;  // 1400 B

class TReassembly : public std::enable_shared_from_this<TReassembly>
{
  public:
	using SharedPtr = std::shared_ptr<TReassembly>;

  public:
	/**
     * @brief: Header struct of the UDP packet according to the RM Comm. Protocol 
     *
     * @note: Little-endian, size is 8 bytes, no alignment.
     *        Ref: https://qingflow.com/appView/c5rf6rkkbs02/shareView/c5rf6slgbs02?applyId=693818572
     */
	struct [[gnu::packed]] Header
	{
		u16 frameIdx;
		u16 secIdx;
		u32 frameLen;

		static constexpr i16 diff(u16 idx_a, u16 idx_b) noexcept
		{
			return static_cast<i16>(idx_a - idx_b);
		}

		static constexpr bool isAfter(u16 idx_a, u16 idx_b) noexcept
		{
			return diff(idx_a, idx_b) > 0;
		}

		static constexpr bool isBefore(u16 idx_a, u16 idx_b) noexcept
		{
			return diff(idx_a, idx_b) < 0;
		}

		/**
         * @brief: Parse a raw buffer into a Header pointer.
         *
         * @param data: The raw buffer to parse.
         * @return: The parsed Header pointer, or nullptr if the buffer is too small.
         * @note: No memory allocation or ownership transfer is performed.
         */
		[[nodiscard("The parsed header pointer should not be ignored")]] static const Header* parse(
			std::span<const u8> data
		) noexcept
		{
			if (data.size() < sizeof(Header)) [[unlikely]] { return nullptr; }
			return reinterpret_cast<const Header*>(data.data());
		}
	};

  private:
	const TVidRender::SharedPtr renderer;

  private:
	std::atomic<bool> synced = false;

  public:
	explicit TReassembly(TVidRender::SharedPtr _renderer) : renderer(std::move(_renderer)) {};

	~TReassembly() = default;

	TReassembly(const TReassembly&)            = delete;  // Forbid copy or move
	TReassembly& operator=(const TReassembly&) = delete;
	TReassembly(TReassembly&&)                 = delete;
	TReassembly&& operator=(TReassembly&&)     = delete;
};
}  // namespace gentau