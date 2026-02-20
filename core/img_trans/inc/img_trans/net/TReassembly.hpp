#pragma once

#include "img_trans/vid_render/TFramePool.hpp"
#include "img_trans/vid_render/TVidRender.hpp"

#include "utils/TTypeRedef.hpp"

#include <array>
#include <atomic>
#include <bitset>
#include <chrono>
#include <memory>
#include <optional>
#include <span>

namespace gentau {
constexpr u64 MTU_LEN = 1400;  // 1400 B

class TReassembly : public std::enable_shared_from_this<TReassembly>
{
  public:
	using SharedPtr = std::shared_ptr<TReassembly>;
	using TimePoint = std::chrono::steady_clock::time_point;

  public:
	/**
     * @brief: Header struct of the UDP packet according to the RM Comm. Protocol 
     *
     * @note: Little-endian, size is 8 bytes, no alignment, might fail to construct on some
	 *        specific CPU archs.
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
			if (data.size() < sizeof(Header)) { return nullptr; }
			return reinterpret_cast<const Header*>(data.data());
		}
	};
	static_assert(sizeof(Header) == 8, "Header size must be 8 bytes");

  public:
	static constexpr u32 maxReAsmSlots   = 5;
	static constexpr u32 maxPayloadSize  = MTU_LEN - sizeof(Header);
	static constexpr u32 maxSecPerFrame  = 1536;
	static constexpr u32 bigFrameThres   = 5000;
	static constexpr i16 minFrameIdxDiff = -180;  // About 3 seconds, assuming 60 FPS

	static constexpr std::chrono::milliseconds reassembleTimeout{ 70 };
	static constexpr std::chrono::milliseconds syncTimeout{ 1000 };

  private:
	struct ReassemblingFrame
	{
		std::optional<TFramePool::FrameData> frameSlot    = std::nullopt;
		u16                                  frameIdx     = 0;
		u32                                  curLen       = 0;
		TimePoint                            asmStartTime = TimePoint::min();
		std::bitset<maxSecPerFrame>          receivedSecs;

		void clear() noexcept
		{
			if (frameSlot.has_value()) { frameSlot.reset(); }
			frameIdx     = 0;
			curLen       = 0;
			asmStartTime = TimePoint::min();
			receivedSecs.reset();
		}

		TFramePool::FrameData steal()
		{
			if (!frameSlot.has_value()) {
				return TFramePool::FrameData(nullptr, nullptr, UINT32_MAX);
			}

			auto data = std::move(frameSlot).value();
			frameSlot.reset();

			return data;
		}

		bool isOccupied() const noexcept
		{
			if (!frameSlot.has_value()) { return false; }

			return frameSlot.value().isValid();
		}

		bool isComplete() const noexcept
		{
			if (!frameSlot.has_value()) { return false; }

			if (!frameSlot.value().isValid()) { return false; }

			return curLen == frameSlot.value().getDataLen();
		}

		bool fill(std::span<u8> packet, const Header* header);
	};

  private:
	const TVidRender::SharedPtr                  renderer;
	std::array<ReassemblingFrame, maxReAsmSlots> rFrames;

  private:
	std::atomic<TimePoint> lastSyncedTime = TimePoint::min();
	std::atomic<u16>       lastPushedIdx  = 0;
	std::atomic<bool>      synced         = false;

  public:
	/**
	 * @brief: Handle a received packet.
	 * 
	 * @param packetData: The raw packet data to be processed. The span must only contain 
	 *                    the valid data received, and should not include any extra padding.
	 * @note: NOT MT-SAFE! Should be called from a single thread synchronously.
	 */
	void onPacketRecv(std::span<u8> packetData);
	void onRecvTimeoutScan();

  private:
	ReassemblingFrame* findReAsmSlot(u16 frameIdx);

  public:
	explicit TReassembly(TVidRender::SharedPtr _renderer);

	SharedPtr create(TVidRender::SharedPtr _renderer)
	{
		return std::make_shared<TReassembly>(std::move(_renderer));
	}

	~TReassembly() = default;

	TReassembly()                              = delete;  // Forbid default construction
	TReassembly(const TReassembly&)            = delete;  // Forbid copy or move
	TReassembly& operator=(const TReassembly&) = delete;
	TReassembly(TReassembly&&)                 = delete;
	TReassembly&& operator=(TReassembly&&)     = delete;
};
}  // namespace gentau