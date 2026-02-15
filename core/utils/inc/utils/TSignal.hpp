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
	// Connect to the signal
	auto connect(auto&&... args) { return sig.connect(std::forward<decltype(args)>(args)...); }

	// Disconnect from the signal
	auto disconnect(auto&&... args)
	{
		return sig.disconnect(std::forward<decltype(args)>(args)...);
	}

	// Connect to the signal, if you want to pass multiple args, use connect() instead
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