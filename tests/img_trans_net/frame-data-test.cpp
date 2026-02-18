#include "img_trans/vid_render/TFramePool.hpp"
#include "utils/TLog.hpp"

#include <chrono>
#include <string>

#define T_LOG_TAG "[FramePool Test] "

using namespace gentau;
using namespace std;
using namespace std::chrono_literals;

void transFrameData(TFramePool::FrameData&& frameData, TFramePool::SharedPtr framePool)
{
	tLogInfo("Processing frame slot with index: {}", frameData.index());
	tLogInfo("Recieved frame's mem addr: {}", static_cast<const void*>(frameData.data()));

	string dataStr(frameData.data(), frameData.data() + frameData.getDataLen());
	tLogInfo("Frame data content: {}", dataStr);

	// Simulate some processing work
	std::this_thread::sleep_for(100ms);

	tLogInfo("Restored frame slot with index: {}", frameData.index());
}

int main()
{
	auto framePool = TFramePool::create();

	while (true) {
		auto frameDataOpt = framePool->acquire();
		if (frameDataOpt.has_value()) {
			auto& frameData = frameDataOpt.value();
			tLogInfo("Acquired frame slot with index: {}", frameData.index());
			tLogInfo("Frame's mem addr: {}", static_cast<const void*>(frameData.data()));

			// Simulate some work with the frame
			std::this_thread::sleep_for(100ms);

			string testData = "Test Frame Data " + to_string(frameData.index());
			memcpy(frameData.data(), testData.c_str(), testData.length());
			frameData.setDataLen(testData.length());

			transFrameData(std::move(frameData), framePool);
		} else {
			tLogWarn("No available frame slots, waiting...");
			std::this_thread::sleep_for(100ms);
		}
	}
}