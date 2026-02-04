#include "utils/TDeduction.hpp"
#include "utils/TLog.hpp"
#include "utils/TLogical.hpp"

#include <chrono>
#include <functional>
#include <iostream>
#include <string>

using namespace std;

#define T_LOG_TAG_IMG   "[LOG-TEST] "
#define T_LOG_TAG_PROTO "[LOG-TEST] "

struct MyType
{
	int a;
	int b;

	bool notZero() const { return a != 0 || b != 0; }
};

template<>
struct gentau::TTraits<MyType>
{
	using Type = MyType;
	static constexpr bool toBool(MyType const& v) { return v.notZero(); }
};

std::string heavy_func()
{
	unsigned long long sum = 0;
	for (int i = 0; i < 10; i++) {
		sum += i * i;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return std::to_string(sum);
}

void func_with_default(int a, bool b = false)
{
	if (b)
		std::cout << "a: " << a << " is true" << std::endl;
	else
		std::cout << "a: " << a << " is false" << std::endl;
}

void recv_func(function<void(int)> functor)
{
	functor(1919);
}

int main()
{
	tImgTransLogTrace("This is a TRACE log message.");
	tImgTransLogDebug("This is a DEBUG log message.");
	tImgTransLogInfo("This is an INFO log message.");
	tImgTransLogWarn("This is a WARN log message.");
	tImgTransLogError("This is an ERROR log message.");
	tImgTransLogCritical("This is a CRITICAL log message.");

	tProtoLogInfo("Doing very heavy func {}", heavy_func());

	std::chrono::system_clock::time_point now_t     = std::chrono::system_clock::now();
	auto                                  now_t_lit = std::chrono::system_clock::to_time_t(now_t);

	std::cout << "We should see this very soon:" << std::ctime(&now_t_lit) << std::endl;

	auto lift_to_str            = liftDefaultParams(std::to_string);
	auto lift_func_with_default = liftDefaultParams(func_with_default);

	std::cout << lift_to_str(114514) << std::endl;
	std::cout << lift_to_str(3.14159) << std::endl;

	lift_func_with_default(114514);
	lift_func_with_default(114514, true);

	recv_func(lift_func_with_default);

	if (gentau::allTrue(1, 2, "a string", 3.14, MyType{ 1, 2 })) { cout << "All True!" << endl; }

	string no_content = "";
	if (gentau::allFalse(0, no_content, "", 0.0, MyType{ 0, 0 })) { cout << "All False!" << endl; }

	if (gentau::anyTrue(0, "Vilva", 0.0, MyType{ 0, 0 })) { cout << "Any True!" << endl; }

	if (gentau::anyFalse(1, 2, "a string", 9.99, MyType{ 0, 0 })) { cout << "Any False!" << endl; }

	std::array<int, 3> arr = { 1, 2, 3 };
	if (gentau::allTrue(arr)) { cout << "All True!" << endl; }
	std::vector<string> str_arr = { "a", "b", "" };
	if (gentau::anyFalse(str_arr)) { cout << "Any False!" << endl; }

	spdlog::shutdown();

	return 0;
}