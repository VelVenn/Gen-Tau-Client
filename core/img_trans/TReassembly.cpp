#include "img_trans/net/TReassembly.hpp"

#include "img_trans/vid_render/TFramePool.hpp"

#include "utils/TLog.hpp"

#include "conf/version.hpp"

#include <chrono>
#include <cstring>
#include <string_view>

#define T_LOG_TAG_IMG "[Reassembler] "

using namespace std;
using namespace std::literals;

namespace gentau {
TReassembly::TReassembly(TVidRender::SharedPtr _renderer) : renderer(std::move(_renderer))
{
	if constexpr (!conf::TDebugMode) {
		if (renderer == nullptr) {
			constexpr auto errMsg = "Renderer cannot be nullptr"sv;
			tImgTransLogError("{}", errMsg);
			throw std::invalid_argument(errMsg.data());
		}
	} else {
		if (!renderer) {
			tImgTransLogWarn(
				"Renderer is nullptr, this is allowed in Debug build for testing purposes, but may "
				"cause some features to not work properly. Use with caution."
			);
		}
	}
}

bool TReassembly::ReassemblingFrame::fill(std::span<u8> packet, const Header* header)
{
	if (!isOccupied() || isComplete()) { return false; }

	if (packet.empty() || header == nullptr) { return false; }

	auto packetLen = static_cast<u32>(packet.size());

	u16 secIdx      = header->secIdx;
	u32 offset      = secIdx * maxPayloadSize;
	u32 payloadSize = packetLen < sizeof(Header) ? 0 : packetLen - sizeof(Header);
	u32 frameLen    = frameSlot->getDataLen();
	u8* destPtr     = frameSlot->data();

	// tImgTransLogDebug("secIdx {} | offset {} | payloadSize {} | frameLen {}", secIdx, offset, payloadSize, frameLen);

	if (secIdx >= receivedSecs.size() || payloadSize == 0) { return false; }

	if (receivedSecs.test(secIdx)) { return false; }

	if (offset + payloadSize > frameLen) { return false; }

	if (!destPtr) { return false; }

	memcpy(destPtr + offset, packet.data() + sizeof(Header), payloadSize);
	receivedSecs.set(secIdx);
	curLen += payloadSize;

	// tImgTransLogDebug("curLen {}", curLen);

	return true;
}

auto TReassembly::findReAsmSlot(u16 idx) -> ReassemblingFrame*
{
	ReassemblingFrame* firstEmpty  = nullptr;
	ReassemblingFrame* oldestSmall = nullptr;
	ReassemblingFrame* oldestLarge = nullptr;

	auto minTimeSmall = TimePoint::max();
	auto minTimeLarge = TimePoint::max();

	for (auto& frame : rFrames) {
		if (frame.isOccupied() && frame.frameIdx == idx) { return &frame; }

		if (!frame.isOccupied()) {
			if (!firstEmpty) { firstEmpty = &frame; }
		} else {
			bool isLarge = frame.frameSlot->getDataLen() >= bigFrameThres;

			if (isLarge) {
				if (frame.asmStartTime < minTimeLarge) {
					minTimeLarge = frame.asmStartTime;
					oldestLarge  = &frame;
				}
			} else {
				if (frame.asmStartTime < minTimeSmall) {
					minTimeSmall = frame.asmStartTime;
					oldestSmall  = &frame;
				}
			}
		}
	}

	if (firstEmpty) { return firstEmpty; }

	// 如果网络状况极差，可能需要加入额外的 Header::isAfter 判断，但这种判断可能会导致僵尸帧无法被
	// 正常清理，这里暂时保持抢占式清理策略即可
	if (oldestSmall) {
		tImgTransLogWarn("Dropping oldest SMALL frame: {}", oldestSmall->frameIdx);
		oldestSmall->clear();
		return oldestSmall;
	}

	if (oldestLarge) {
		tImgTransLogWarn("Dropping oldest LARGE frame: {}", oldestLarge->frameIdx);
		oldestLarge->clear();
		return oldestLarge;
	}

	return nullptr;
}

void TReassembly::onPacketRecv(std::span<u8> packetData, TRecvPasskey)
{
	auto now = chrono::steady_clock::now();
	if (synced.load() && now - lastSyncedTime.load() > syncTimeout) {
		tImgTransLogWarn("Sync timeout detected on recieving packet.");
		synced.store(false);
	}

	if (packetData.empty() || packetData.size() < sizeof(Header)) {
		tImgTransLogWarn("Received packet too small to contain valid header, ignoring.");
		return;
	}

	auto header = Header::parse(packetData);
	if (!header) {
		tImgTransLogWarn("Received packet with invalid header, ignoring.");
		return;
	}

	if (header->frameLen > TFramePool::slotLen) {
		tImgTransLogWarn(
			"Received packet with frame length {} exceeding slot capacity, ignoring.",
			header->frameLen
		);
		return;
	}

	auto frameIdxDiff = Header::diff(header->frameIdx, lastPushedIdx.load());

	if (synced.load() && frameIdxDiff < minFrameIdxDiff) {
		tImgTransLogWarn(
			"Received abnormally old frame: {} (sec {}), last pushed frame: {}, considering it as "
			"new session start.",
			header->frameIdx,
			header->secIdx,
			lastPushedIdx.load()
		);
		synced.store(false);
	}

	if (synced.load() && frameIdxDiff <= 0) {
		return;
	}  // Drop likely normal duplicate or out-of-order packet

	if (!synced.load()) {
		synced.store(true);

		// set to one before current. It's ok to overflow.
		lastPushedIdx.store(header->frameIdx - 1);

		tImgTransLogDebug("Session synced at frame {}, sec {}.", header->frameIdx, header->secIdx);
	}
	lastSyncedTime.store(now);

	auto rSlot = findReAsmSlot(header->frameIdx);
	if (!rSlot) [[unlikely]] {
		tImgTransLogWarn(
			"No available reassembly slot for frame {}, dropping packet.", header->frameIdx
		);
		return;
	}

	if (!rSlot->isOccupied()) {
		auto frameDataOpt = renderer->acquireFrameSlot({});
		if (!frameDataOpt.has_value()) { return; }

		rSlot->frameSlot = std::move(frameDataOpt).value();
		rSlot->frameSlot->setDataLen(header->frameLen);

		rSlot->frameIdx     = header->frameIdx;
		rSlot->asmStartTime = now;
	}

	if (rSlot->fill(packetData, header)) {
		if (rSlot->isComplete()) {
			renderer->tryPushFrame(rSlot->steal(), {});
			lastPushedIdx.store(header->frameIdx);

			for (auto& frame : rFrames) {
				if ((frame.isOccupied() && Header::isBefore(frame.frameIdx, header->frameIdx)) ||
					frame.frameIdx == header->frameIdx) {
					frame.clear();
				}
			}
		}
	}
};

void TReassembly::onRecvTimeoutScan(TRecvPasskey)
{
	auto now = chrono::steady_clock::now();

	if (synced.load() && now - lastSyncedTime.load() > syncTimeout) {
		tImgTransLogWarn("Sync timeout detected on recieving packet timeout.");
		synced.store(false);
	}

	for (auto& frame : rFrames) {
		if (frame.isOccupied() && now - frame.asmStartTime > reassembleTimeout) {
			frame.clear();
			continue;
		}

		// if (frame.isComplete() && Header::isAfter(frame.frameIdx, lastPushedIdx.load())) {
		// 	renderer->tryPushFrame(frame.steal());
		// 	lastPushedIdx.store(frame.frameIdx);
		// }
	}
}
}  // namespace gentau