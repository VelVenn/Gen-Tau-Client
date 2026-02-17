#include "img_trans/net/TRecv.hpp"

#include "utils/TLog.hpp"

#include <asm-generic/socket.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>

#include <array>
#include <future>
#include <stop_token>
#include <system_error>
#include <thread>

#define T_LOG_TAG_IMG "[UDP Receiver] "

using namespace std;

namespace gentau {
void TRecv::stop()
{
	if (recvThread.joinable()) {
		recvThread.request_stop();
		recvThread.join();
	}
}

void TRecv::stopAsync()
{
	if (recvThread.joinable()) { recvThread.request_stop(); }
}

int TRecv::start()
{
	if (!isBound()) {
		tImgTransLogError("Cannot start receiving thread: socket is not bound");
		return EBADF;  // Bad file descriptor
	}

	if (recvThread.joinable()) {
		tImgTransLogWarn("Receiving thread is already running");
		return 0;  // Already running, consider it a success
	}

	promise<i32> threadErrPassThru;
	auto         passThruFuture = threadErrPassThru.get_future();

	recvThread = jthread([this,
						  passThru = std::move(threadErrPassThru)](stop_token sToken) mutable {
		if (::setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &kRecvBufferSize, sizeof(i32)) < 0) {
			tImgTransLogError(
				"Failed to set socket kernel receive buffer size, error: {}",
				error_code(errno, system_category()).message()
			);
			passThru.set_value(errno);
			return;
		}

		if (::setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &kRecvTimeout, sizeof(timeval)) < 0) {
			tImgTransLogError(
				"Failed to set socket receive timeout, error: {}",
				error_code(errno, system_category()).message()
			);
			passThru.set_value(errno);
			return;
		}

		passThru.set_value(0);

		array<u8, MTU_LEN> recvBuffer{};
		memset(&recvBuffer, 0, MTU_LEN);

		while (!sToken.stop_requested()) {
			auto ret = ::recv(sockfd, recvBuffer.data(), MTU_LEN, 0);

			if (ret > 0) {
				lastRecvTime.store(chrono::steady_clock::now());

				auto header = TReassembly::Header::parse(recvBuffer);
				if (header) {
					tImgTransLogDebug(
						"Received packet - frameIdx: {}, secIdx: {}, frameLen: {}",
						header->frameIdx,
						header->secIdx,
						header->frameLen
					);
				} else {
					tImgTransLogWarn("Received packet too small to contain header, size: {}", ret);
				}
			} else {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					// Do some periodic task...
					continue;  // Timeout, just try again
				} else {
					tImgTransLogError(
						"Error receiving data, error: {}",
						error_code(errno, system_category()).message()
					);
					break;
				}
			}
		}

		tImgTransLogTrace("UDP Receive thread stopped");
	});

	auto threadErr = passThruFuture.get();

	return threadErr;
}

i32 TRecv::bindV4(u16 port, const char* ip)
{
	stop();

	auto newSockfd = ::socket(AF_INET, SOCK_DGRAM, 0);

	if (newSockfd < 0) {
		tImgTransLogError(
			"Failed to create new socket, error: {}", error_code(errno, system_category()).message()
		);
		return errno;
	}

	sockaddr_in newAddr{};

	auto v4Addr = V4Addr::create(ip, port);
	if (!v4Addr.has_value()) {
		tImgTransLogError("Invalid IP address: {}:{}", ip, port);
		::close(newSockfd);
		return EINVAL;  // Invalid argument
	}

	newAddr.sin_family      = AF_INET;
	newAddr.sin_addr.s_addr = v4Addr->ip;
	newAddr.sin_port        = ::htons(v4Addr->port);

	if (::bind(newSockfd, reinterpret_cast<sockaddr*>(&newAddr), sizeof(newAddr)) < 0) {
		tImgTransLogError(
			"Failed to bind socket to ip: {}, error: {}",
			v4Addr->toString(),
			error_code(errno, system_category()).message()
		);
		::close(newSockfd);
		return errno;
	}

	sockfd     = newSockfd;
	listenAddr = newAddr;

	tImgTransLogInfo("New socket created successfully");

	return 0;
}

TRecv::TRecv(TReassembly::SharedPtr _reassembler, u16 _port, const char* _ip) :
	reassembler(std::move(_reassembler))
{
	if (!reassembler) { throw std::invalid_argument("Reassembler pointer cannot be null"); }

	auto bindResult = bindV4(_port, _ip);
	if (bindResult != 0) {
		tImgTransLogError(
			"Failed to bind to {}:{}, error: {}",
			_ip,
			_port,
			error_code(bindResult, system_category()).message()
		);
	}
}

TRecv::~TRecv()
{
	if (isBound()) {
		::close(sockfd);
		tImgTransLogDebug("TRecv resources released, socket closed");
	}
}

}  // namespace gentau