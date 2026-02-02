#pragma once

#include <string>
#include <type_traits>

namespace gentau {
template<typename T>
struct TTraits
{
	using Type = T;                                                            // Optional
	static constexpr bool toBool(T const& v) { return static_cast<bool>(v); }  // compulsory
};

template<>
struct TTraits<char const*>
{
	using Type = char const*;
	static constexpr bool toBool(char const* v) { return v && (*v != '\0'); }
};

template<>
struct TTraits<char*>
{
	using Type = char*;
	static constexpr bool toBool(char* v) { return v && (*v != '\0'); }
};

template<>
struct TTraits<std::string>
{
	using Type = std::string;
	static constexpr bool toBool(std::string const& v) { return !v.empty(); }
};

#define T_TRAITS_DECAY(v) TTraits<std::decay_t<decltype(v)>>::toBool(v)

constexpr bool allTrue(const auto&... vals)
{
	return (... && T_TRAITS_DECAY(vals));

	// expande to (((val1 && val2) && val3) && valN)
	// if is (vals && ...) will expand to (val1 && (val2 && (val3 && valN)))
	// 参数顺序不变，仅改变了结合顺序
	// auto&& 既能接受左值也能接受右值，可读写变量
	// auto&& 不是移动语义
	// const auto& 既能接受左值也能接受右值，不可修改变量
	// auto& 只能接受左值，可修改变量
	// auto 既能接受左值也能接受右值，可读写变量，但会推导为拷贝
}

constexpr bool allFalse(const auto&... vals)
{
	return (... && !T_TRAITS_DECAY(vals));
}

constexpr bool anyTrue(const auto&... vals)
{
	return (... || T_TRAITS_DECAY(vals));
}

constexpr bool anyFalse(const auto&... vals)
{
	return (... || !T_TRAITS_DECAY(vals));
}

#undef T_TRAITS_DECAY

}  // namespace gentau