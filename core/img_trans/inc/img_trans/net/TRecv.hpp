#pragma once

#include "img_trans/net/TReassembly.hpp"

#include "utils/TTypeRedef.hpp"

#include <arpa/inet.h>
#include <bits/types/struct_timeval.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <thread>

namespace gentau {
class TRecv
{
  public:
	using UniPtr    = std::unique_ptr<TRecv>;
	using TimePoint = std::chrono::steady_clock::time_point;

  private:
	struct V4Addr
	{
		u32 ip   = 0;
		u16 port = 0;

		V4Addr(u32 _ipNetworkOrder, u16 _port) : ip(_ipNetworkOrder), port(_port) {}

		bool isValid() const { return ip != 0 && port != 0; }

		std::string toString() const
		{
			auto ipStrOpt = TRecv::V4Addr::ipToStr(ip);
			if (!ipStrOpt.has_value()) { return "Invalid IP"; }
			return std::move(ipStrOpt).value() + ":" + std::to_string(port);
		}

		static std::optional<V4Addr> create(const char* ipStr, u16 port)
		{
			auto ipOpt = strToIp(ipStr);
			if (!ipOpt.has_value()) { return std::nullopt; }
			return V4Addr(ipOpt.value(), port);
		}

		static std::optional<u32> strToIp(const char* ipStr)
		{
			u32 addr;
			if (inet_pton(AF_INET, ipStr, &addr) <= 0) { return std::nullopt; };
			return addr;
		}

		static std::optional<std::string> ipToStr(u32 ipNetworkOrder)
		{
			char str[INET_ADDRSTRLEN];
			if (inet_ntop(AF_INET, &ipNetworkOrder, str, INET_ADDRSTRLEN) == nullptr) {
				return std::nullopt;
			}
			return std::string(str);
		}
	};

  private:
	const TReassembly::SharedPtr reassembler;
	int                          sockfd       = -1;
	sockaddr_in                  listenAddr   = {};
	std::atomic<TimePoint>       lastRecvTime = TimePoint::min();

  private:
	static constexpr i32     kRecvBufferSize = 4 * 1024 * 1024;  // 4MB
	static constexpr timeval kRecvTimeout    = { 0, 100'000 };   // 100ms

  private:
	// 这里必须放在所有字段的后面，以确保在析构时先停止线程，避免访问已销毁的成员变量。
	std::jthread recvThread;

  public:
	int  start();
	void stop();
	void stopAsync();

  public:
	/**
	 * @brief: Get the last received time point. if no packet has been received,
	 *         it returns TimePoint::min().
	 * 
	 * @note: MT-SAFE
	 */
	TimePoint getLastRecvTime() const noexcept { return lastRecvTime.load(); }

  public:
	/**
	 * @brief: Bind to a specific IPv4 address and port.
	 *
	 * @return: 0 on success, else the POSIX errno code of the failure reason.
	 *          - EINVAL if the provided IP address string is invalid.
	 *          - other errno codes from socket() and bind() error.
	 * 
	 * @note: NOT MT-SAFE! Will stop the current receiving thread if 
	 *        it's running when calling this method.
	 */
	i32                   bindV4(u16 port, const char* ip);
	bool                  isBound() const noexcept { return sockfd > -1; }
	std::optional<V4Addr> getListenAddr() const noexcept
	{
		if (!isBound()) { return std::nullopt; }
		return V4Addr(listenAddr.sin_addr.s_addr, ::ntohs(listenAddr.sin_port));
	}

  public:
	TRecv(TReassembly::SharedPtr _reassembler, u16 _port = 3334, const char* _ip = "127.0.0.1");
	~TRecv();

	/**
	 * @brief: create a shared pointer to TRecv instance. 
	 *
	 * @throws: std::invalid_argument if reassembler is nullptr.
	 */
	[[nodiscard("Should not ignored the created TRecv::UniPtr")]] static UniPtr create(
		TReassembly::SharedPtr reassembler, u16 port = 3334, const char* ip = "127.0.0.1"
	)
	{
		return std::make_unique<TRecv>(reassembler, port, ip);
	}

	TRecv(const TRecv&)            = delete;  // Forbid copy or move
	TRecv& operator=(const TRecv&) = delete;
	TRecv(TRecv&&)                 = delete;
	TRecv& operator=(TRecv&&)      = delete;
};
}  // namespace gentau