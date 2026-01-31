#pragma once

#include "conf/version.hpp"

#include <memory>

#include "spdlog/async_logger.h"
#include "spdlog/spdlog.h"

namespace gentau {

using LoggerPtr = std::shared_ptr<spdlog::async_logger>;

LoggerPtr getImgTransLogger();
LoggerPtr getProtoLogger();

}  // namespace gentau

#if defined(GEN_TAU_LOG_ENABLED) && (GEN_TAU_LOG_ENABLED == 1)
// Image Transmission Module Logging Macros
#define tImgTransLogTrace(fmt, ...)                                                                \
	SPDLOG_LOGGER_TRACE(gentau::getImgTransLogger(), T_LOG_TAG_IMG fmt __VA_OPT__(, ) __VA_ARGS__)
#define tImgTransLogDebug(fmt, ...)                                                                \
	SPDLOG_LOGGER_DEBUG(gentau::getImgTransLogger(), T_LOG_TAG_IMG fmt __VA_OPT__(, ) __VA_ARGS__)
#define tImgTransLogInfo(fmt, ...)                                                                \
	SPDLOG_LOGGER_INFO(gentau::getImgTransLogger(), T_LOG_TAG_IMG fmt __VA_OPT__(, ) __VA_ARGS__)
#define tImgTransLogWarn(fmt, ...)                                                                \
	SPDLOG_LOGGER_WARN(gentau::getImgTransLogger(), T_LOG_TAG_IMG fmt __VA_OPT__(, ) __VA_ARGS__)
#define tImgTransLogError(fmt, ...)                                                                \
	SPDLOG_LOGGER_ERROR(gentau::getImgTransLogger(), T_LOG_TAG_IMG fmt __VA_OPT__(, ) __VA_ARGS__)

#define tImgTransLogCritical(fmt, ...)                                            \
	SPDLOG_LOGGER_CRITICAL(                                                       \
		gentau::getImgTransLogger(), T_LOG_TAG_IMG fmt __VA_OPT__(, ) __VA_ARGS__ \
	)

// Protobuf Module Logging Macros
#define tProtoLogTrace(fmt, ...)                                                                  \
	SPDLOG_LOGGER_TRACE(gentau::getProtoLogger(), T_LOG_TAG_PROTO fmt __VA_OPT__(, ) __VA_ARGS__)
#define tProtoLogDebug(fmt, ...)                                                                  \
	SPDLOG_LOGGER_DEBUG(gentau::getProtoLogger(), T_LOG_TAG_PROTO fmt __VA_OPT__(, ) __VA_ARGS__)
#define tProtoLogInfo(fmt, ...)                                                                  \
	SPDLOG_LOGGER_INFO(gentau::getProtoLogger(), T_LOG_TAG_PROTO fmt __VA_OPT__(, ) __VA_ARGS__)
#define tProtoLogWarn(fmt, ...)                                                                  \
	SPDLOG_LOGGER_WARN(gentau::getProtoLogger(), T_LOG_TAG_PROTO fmt __VA_OPT__(, ) __VA_ARGS__)
#define tProtoLogError(fmt, ...)                                                                  \
	SPDLOG_LOGGER_ERROR(gentau::getProtoLogger(), T_LOG_TAG_PROTO fmt __VA_OPT__(, ) __VA_ARGS__)
#define tProtoLogCritical(fmt, ...) \
	SPDLOG_LOGGER_CRITICAL(gentau::getProtoLogger(), T_LOG_TAG_PROTO fmt __VA_OPT__(, ) __VA_ARGS__)

#else
#define tImgTransLogTrace(fmt, ...)    ((void)0)
#define tImgTransLogDebug(fmt, ...)    ((void)0)
#define tImgTransLogInfo(fmt, ...)     ((void)0)
#define tImgTransLogWarn(fmt, ...)     ((void)0)
#define tImgTransLogError(fmt, ...)    ((void)0)
#define tImgTransLogCritical(fmt, ...) ((void)0)

#define tProtoLogTrace(fmt, ...)    ((void)0)
#define tProtoLogDebug(fmt, ...)    ((void)0)
#define tProtoLogInfo(fmt, ...)     ((void)0)
#define tProtoLogWarn(fmt, ...)     ((void)0)
#define tProtoLogError(fmt, ...)    ((void)0)
#define tProtoLogCritical(fmt, ...) ((void)0)

#endif