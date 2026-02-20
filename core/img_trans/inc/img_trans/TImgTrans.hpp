#pragma once

#include "img_trans/net/TReassembly.hpp"
#include "img_trans/net/TRecv.hpp"
#include "img_trans/vid_render/TVidRender.hpp"

#include "utils/TTypeRedef.hpp"

namespace gentau {
/**
 * TImgTrans 是对网络接收、数据重组和视频渲染模块的轻度包装，它保证了底层组件的初始化顺序正确与生命周期的统一。
 * 如果没有特殊需求，建议总是通过 TImgTrans 来使用 TVidRender、TReassembly 和 TRecv，而不是手动创建和管
 * 理它们的实例。
 *
 * 在使用时，应当确保 TImgTrans 的构造永远发生在调用 TImgTrans::initContext() 与 QQGuiApplication 的实例
 * 化之后，TVidRender::play() 必须在 Qt 的渲染同步发生前调用，否则可能会导致未定义行为，严重时可能会导致段错误。
 * 通常情况下，在 GUI 的 view-model 层或 controller 层创建与管理 TImgTrans 实例是一个不错的选择。
 *
 * 请参考 tests/render/rend-net.cpp 中的示例用法，但需要注意该例程不适合直接用于生产环境中。
 *
 * 通过 TImgTrans 使用 TVidRender 时，请永远不要直接调用 TVidRender::tryPushFrame(TVidRender::FramePtr) 
 * 方法，该方法仅用于测试目的，否则可能会在 Debug 构建下导致非常严重的安全性问题（因为它允许任意帧数据被推送到管道中），
 * 在非 Debug 构建下该方法无任何实际效果与副作用，但仍然不建议调用以免造成误解。让底层组件自行处理数据的流转才是 TImgTrans 
 * 安全和正确的使用方式。
 *
 * @note: TImgTrans 的成员均有文档注释，强烈建议在使用前仔细阅读这些注释以避免误用。
 */
class TImgTrans
{
  public:
	using SharedPtr = std::shared_ptr<TImgTrans>;

  public:
	const TVidRender::SharedPtr  renderer;
	const TReassembly::SharedPtr reassembler;
	const TRecv::UniPtr          receiver;

  public:
	static void initContext(int* argc, char** argv[]) { TVidRender::initContext(argc, argv); }

  public:
	explicit TImgTrans(
		u64 _maxBufferBytes = 262'144, u16 recvPort = 3334, const char* recvIp = "127.0.0.1"
	) :
		renderer(TVidRender::create(_maxBufferBytes)),
		reassembler(TReassembly::create(renderer)),
		receiver(TRecv::createUni(reassembler, recvPort, recvIp)) {};

	/**
     * 创建一个 TImgTrans 实例。
     * @param maxBufferBytes 最大缓冲区大小（字节）
     * @param recvPort 接收端口
     * @param recvIp 接收 IP 地址
     * @return TImgTrans 的共享指针
     * @throws std::runtime_error 如果管道初始化失败。
     */
	[[nodiscard("Should not ignored the created TImgTrans::SharedPtr")]] static SharedPtr create(
		u64 maxBufferBytes = 262'144, u16 recvPort = 3334, const char* recvIp = "127.0.0.1"
	)
	{
		return std::make_shared<TImgTrans>(maxBufferBytes, recvPort, recvIp);
	}

	~TImgTrans() = default;
};
}  // namespace gentau