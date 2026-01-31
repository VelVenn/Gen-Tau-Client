#pragma once

namespace gentau {
constexpr bool allTrue(const auto&... vals)
{
	return (... && vals);  // expande to (((val1 && val2) && val3) && valN)
						   // if is (vals && ...) will expand to (val1 && (val2 && (val3 && valN)))
						   // 参数顺序不变，仅改变了结合顺序
						   // auto&& 既能接受左值也能接受右值，可读写变量，即完美转发
						   // auto&& 不是移动语义
						   // const auto& 既能接受左值也能接受右值，不可修改变量
						   // auto& 只能接受左值，可修改变量
						   // auto 既能接受左值也能接受右值，可读写变量，但会推导为拷贝
}

constexpr bool allFalse(const auto&... vals)
{
	return (... && !vals);
}

constexpr bool anyTrue(const auto&... vals)
{
	return (... || vals);
}

constexpr bool anyFalse(const auto&... vals)
{
	return (... || !vals);
}

}  // namespace gentau