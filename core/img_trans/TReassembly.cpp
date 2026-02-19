#include "img_trans/net/TReassembly.hpp"

#include "img_trans/vid_render/TFramePool.hpp"
#include "utils/TLog.hpp"

#include <chrono>
#include <cstring>
#include <string_view>

#define T_LOG_TAG_IMG "[Reassembler] "

using namespace std;
using namespace std::literals;

namespace gentau {
TReassembly::TReassembly(TVidRender::SharedPtr _renderer) : renderer(std::move(_renderer))
{
	if (renderer == nullptr) {
		constexpr auto errMsg = "Renderer cannot be nullptr"sv;
		tImgTransLogError("{}", errMsg);
		throw std::invalid_argument(errMsg.data());
	}
}

bool TReassembly::ReassemblingFrame::fill(std::span<u8> packet, u32 packetLen, const Header* header)
{
	if (!isOccupied() || isComplete()) { return false; }

	if (packet.empty() || header == nullptr) { return false; }

	u16 secIdx      = header->secIdx;
	u32 offset      = secIdx * maxPayloadSize;
	u32 payloadSize = packetLen < sizeof(Header) ? 0 : packetLen - sizeof(Header);
	u32 frameLen    = frameSlot->getDataLen();

	// tImgTransLogDebug("secIdx {} | offset {} | payloadSize {} | frameLen {}", secIdx, offset, payloadSize, frameLen);

	if (secIdx >= receivedSecs.size() || payloadSize == 0) { return false; }

	if (receivedSecs.test(secIdx)) { return false; }

	if (offset + payloadSize > frameLen) { return false; }

	memcpy(frameSlot->data() + offset, packet.data() + sizeof(Header), payloadSize);
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

void TReassembly::onPacketRecv(std::span<u8> packetData, u32 packetLen)
{
	if (packetData.empty() || packetLen == 0) { return; }

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
			"Received abnormally old frame: {} (sec {}), last pushed frame: {}, considering it as new "
			"section",
			header->frameIdx,
			header->secIdx,
			lastPushedIdx.load()
		);
		synced.store(false);
	}

	if (synced.load() && frameIdxDiff <= 0) {
		return;
	}  // Drop likely normal duplicate or out-of-order packet

	if (!synced.load()) { synced.store(true); }
	lastSyncedTime.store(chrono::steady_clock::now());

	auto rSlot = findReAsmSlot(header->frameIdx);
	if (!rSlot) [[unlikely]] {
		tImgTransLogWarn(
			"No available reassembly slot for frame {}, dropping packet.", header->frameIdx
		);
		return;
	}

	if (!rSlot->isOccupied()) {
		auto frameDataOpt = renderer->acquireFrameSlot();
		if (!frameDataOpt.has_value()) { return; }

		rSlot->frameSlot = std::move(frameDataOpt).value();
		rSlot->frameSlot->setDataLen(header->frameLen);

		rSlot->frameIdx     = header->frameIdx;
		rSlot->asmStartTime = chrono::steady_clock::now();
	}

	if (rSlot->fill(packetData, packetLen, header)) {
		if (rSlot->isComplete()) {
			renderer->tryPushFrame(rSlot->steal());
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

void TReassembly::onRecvTimeoutScan()
{
	auto now = chrono::steady_clock::now();

	if (synced.load() && now - lastSyncedTime.load() > syncTimeout) { synced.store(false); }

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