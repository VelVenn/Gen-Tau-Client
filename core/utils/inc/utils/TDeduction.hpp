#pragma once

namespace gentau {
template<typename... VTypes, typename Cls, typename Ret>
constexpr auto overload(Ret (Cls::*ptr)(VTypes...))
{
	return ptr;
}

template<typename... VTypes, typename Cls, typename Ret>
constexpr auto overload(Ret (Cls::*ptr)(VTypes...) const)
{
	return ptr;
}

template<typename... VTypes, typename Ret>
constexpr auto overload(Ret (*ptr)(VTypes...))
{
	return ptr;
}

#define liftDefaultParams(func)                                                                    \
	[=](auto&&... args) noexcept(                                                                  \
		noexcept(func(std::forward<decltype(args)>(args)...))                                      \
	) -> decltype(auto) {                                                                          \
		return func(std::forward<decltype(args)>(args)...);                                        \
	}

}  // namespace gentau