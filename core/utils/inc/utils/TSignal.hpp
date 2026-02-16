#pragma once

#include "sigslot/signal.hpp"

#include <concepts>
#include <type_traits>

namespace gentau {
template<typename T>
struct IsSigslotSignal : std::false_type
{};

template<typename... Args>
struct IsSigslotSignal<sigslot::signal<Args...>> : std::true_type
{};

template<typename T>
concept SigslotSignalType = IsSigslotSignal<T>::value;

/**
 * TSignal 是对 sigslot::signal 的一个简单封装，它限制了信号只能由指定的 OwnerType 发出，
 * 并且提供了更简洁的接口来连接和断开槽函数。
 *
 * 需要注意的是，在 connect 到 TSignal 的槽函数会在发出信号的线程中同步执行，因此调用者需要
 * 负责在适当的线程中调度。
 *
 * TSignal 的具体使用方法请参考 sigslot::signal 的文档和示例：
 * https://github.com/palacaze/sigslot
 */
template<typename OwnerType, typename... Args>
class TSignal
{
	friend OwnerType;

  private:
	sigslot::signal<Args...> sig;

	// Emit a signal
	template<typename... EmitArgs>
		requires(std::convertible_to<EmitArgs, Args> && ...)
	void emit(EmitArgs&&... args) const
	{
		sig(std::forward<EmitArgs>(args)...);
	}

	// Emit a signal
	template<typename... EmitArgs>
		requires(
			std::convertible_to<EmitArgs, Args> && ...
		)  // Ensure the length and types of emitted arguments match the signal's signature
	void operator()(EmitArgs&&... args) const
	{
		sig(std::forward<EmitArgs>(args)...);
	}

  public:
	/**
	 * Connect to the signal.
	 * Please note that the connected slots will be executed synchronously
	 * in the context of the emitter. Callers are responsible for dispatching
	 * to the appropriate thread if needed.
	 */
	auto connect(auto&&... args) { return sig.connect(std::forward<decltype(args)>(args)...); }

	// Disconnect from the signal
	auto disconnect(auto&&... args)
	{
		return sig.disconnect(std::forward<decltype(args)>(args)...);
	}

	/**
	 * Connect to the signal, if you want to pass multiple args, use connect() instead.
	 * Please note that the connected slots will be executed synchronously
	 * in the context of the emitter. Callers are responsible for dispatching
	 * to the appropriate thread if needed.
	 */
	auto operator+=(auto&& slot) { return sig.connect(std::forward<decltype(slot)>(slot)); }

	// Disconnect from the signal, if you want to pass multiple args, use disconnect() instead
	auto operator-=(auto&& slot) { return sig.disconnect(std::forward<decltype(slot)>(slot)); }

	TSignal() = default;
	~TSignal() { sig.disconnect_all(); }

	TSignal(const TSignal&)            = delete;  // Forbid copy or move
	TSignal& operator=(const TSignal&) = delete;
	TSignal(TSignal&&)                 = delete;
	TSignal& operator=(TSignal&&)      = delete;
};
}  // namespace gentau