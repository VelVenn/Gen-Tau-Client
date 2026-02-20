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

class TRecv;

class TRecvPasskey
{
	friend class TRecv;
	TRecvPasskey() = default;
};

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

		/**
		 * @brief: Calculate the difference between two frame indices, considering wrap-around.
		 * @return: The signed difference between idx_a and idx_b. Positive if idx_a is after 
		 *          idx_b, negative if idx_a is before idx_b, zero if they are the same.
		 */
		static constexpr i16 diff(u16 idx_a, u16 idx_b) noexcept
		{
			return static_cast<i16>(idx_a - idx_b);
		}

		/**
		 * @brief: Check if idx_a is after idx_b, considering wrap-around.
		 */
		static constexpr bool isAfter(u16 idx_a, u16 idx_b) noexcept
		{
			return diff(idx_a, idx_b) > 0;
		}

		/**
		 * @brief: Check if idx_a is before idx_b, considering wrap-around.
		 */
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
	static constexpr u32 maxSecPerFrame  = 1536;  // 1536 = 64 * 24, 1536 * 1392 ~= 2.04 MiB
	static constexpr u32 bigFrameThres   = 5000;  // 5 KB
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
		std::bitset<maxSecPerFrame>          receivedSecs;  // bitmap is based on uint64_t array

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
	 * @brief: 获取上一次网络连接同步成功（即收到有效包）的时间点。若未曾成功同步过，返回 TimePoint::min()。
	 * @note: 多线程安全。
	 */
	TimePoint getLastSyncedTime() const noexcept { return lastSyncedTime.load(); }

	/**
	 * @brief: 获取上一次推送到渲染管线的帧索引。若从未推送过任何帧，返回 0。
	 * @note: 多线程安全。
	 */
	u16 getLastPushedIdx() const noexcept { return lastPushedIdx.load(); }

	/**
	 * @brief: 检查当前是否处于网络连接同步状态（即是否持续收到有效包）。
	 * @note: 多线程安全。
	 */
	bool isSynced() const noexcept { return synced.load(); }

  public:
	/**
	 * @brief: 处理接收到的原始数据包。
	 * @param packetData: 接收到的包含协议头部的原始数据包内容，除此自外不能包含任何额外的填充字节。
	 * @note: 该方法仅能在 TRecv 类内部被正常调用，其他地方调用此方法将导致编译错误。该方法当且仅当
	 *        存在单一调用者时才是线程安全的，请勿在多个线程中并发调用此方法。
	 */
	void onPacketRecv(std::span<u8> packetData, TRecvPasskey);

	/**
	 * @brief: 检查同步状态。扫描当前正在重组的帧，检查是否有重组超时的帧，并进行相应的处理。
	 * @note: 该方法仅能在 TRecv 类内部被正常调用，其他地方调用此方法将导致编译错误。该方法当且仅当
	 *        存在单一调用者时才是线程安全的，请勿在多个线程中并发调用此方法。
	 */
	void onRecvTimeoutScan(TRecvPasskey);

  private:
	ReassemblingFrame* findReAsmSlot(u16 frameIdx);

  public:
	/**
	 * @brief: constructor of TReassembly.
	 * @throw: std::invalid_argument if the provided TVidRender::SharedPtr is nullptr.
	 */
	explicit TReassembly(TVidRender::SharedPtr _renderer);

	/**
	 * @brief: create a shared pointer to TReassembly instance. 
	 * @throw: std::invalid_argument if the provided TVidRender::SharedPtr is nullptr.
	 */

	[[nodiscard("Should not ignored the created TReassembly::SharedPtr")]] static SharedPtr create(
		TVidRender::SharedPtr _renderer
	)
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