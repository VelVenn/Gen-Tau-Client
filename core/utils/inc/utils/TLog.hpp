#pragma once

#include "conf/version.hpp"

#include <memory>

#include "spdlog/async_logger.h"
#include "spdlog/spdlog.h"

namespace gentau {

using LoggerPtr = std::shared_ptr<spdlog::async_logger>;

// DO NOT CALL THIS DIRECTLY
LoggerPtr getImgTransLogger();

// DO NOT CALL THIS DIRECTLY
LoggerPtr getCommLogger();

// DO NOT CALL THIS DIRECTLY
LoggerPtr getGeneralLogger();

}  // namespace gentau

#if defined(GEN_TAU_LOG_ENABLED) && (GEN_TAU_LOG_ENABLED == 1)
// Image Transmission Module Logging Macros
#define tImgTransLogTrace(fmt, ...)                                                                \
	SPDLOG_LOGGER_TRACE(gentau::getImgTransLogger(), T_LOG_TAG_IMG fmt __VA_OPT__(, ) __VA_ARGS__)
#define tImgTransLogDebug(fmt, ...)                                                                \
	SPDLOG_LOGGER_DEBUG(gentau::getImgTransLogger(), T_LOG_TAG_IMG fmt __VA_OPT__(, ) __VA_ARGS__)
#define tImgTransLogInfo(fmt, ...)                                                                 \
	SPDLOG_LOGGER_INFO(gentau::getImgTransLogger(), T_LOG_TAG_IMG fmt __VA_OPT__(, ) __VA_ARGS__)
#define tImgTransLogWarn(fmt, ...)                                                                 \
	SPDLOG_LOGGER_WARN(gentau::getImgTransLogger(), T_LOG_TAG_IMG fmt __VA_OPT__(, ) __VA_ARGS__)
#define tImgTransLogError(fmt, ...)                                                                \
	SPDLOG_LOGGER_ERROR(gentau::getImgTransLogger(), T_LOG_TAG_IMG fmt __VA_OPT__(, ) __VA_ARGS__)

#define tImgTransLogCritical(fmt, ...)                                                             \
	SPDLOG_LOGGER_CRITICAL(                                                                        \
		gentau::getImgTransLogger(), T_LOG_TAG_IMG fmt __VA_OPT__(, ) __VA_ARGS__                  \
	)

// Communication Module Logging Macros
#define tCommLogTrace(fmt, ...)                                                                    \
	SPDLOG_LOGGER_TRACE(gentau::getCommLogger(), T_LOG_TAG_COMM fmt __VA_OPT__(, ) __VA_ARGS__)
#define tCommLogDebug(fmt, ...)                                                                    \
	SPDLOG_LOGGER_DEBUG(gentau::getCommLogger(), T_LOG_TAG_COMM fmt __VA_OPT__(, ) __VA_ARGS__)
#define tCommLogInfo(fmt, ...)                                                                     \
	SPDLOG_LOGGER_INFO(gentau::getCommLogger(), T_LOG_TAG_COMM fmt __VA_OPT__(, ) __VA_ARGS__)
#define tCommLogWarn(fmt, ...)                                                                     \
	SPDLOG_LOGGER_WARN(gentau::getCommLogger(), T_LOG_TAG_COMM fmt __VA_OPT__(, ) __VA_ARGS__)
#define tCommLogError(fmt, ...)                                                                    \
	SPDLOG_LOGGER_ERROR(gentau::getCommLogger(), T_LOG_TAG_COMM fmt __VA_OPT__(, ) __VA_ARGS__)
#define tCommLogCritical(fmt, ...)                                                                 \
	SPDLOG_LOGGER_CRITICAL(gentau::getCommLogger(), T_LOG_TAG_COMM fmt __VA_OPT__(, ) __VA_ARGS__)

// General Logging Macros
#define tLogTrace(fmt, ...)                                                                        \
	SPDLOG_LOGGER_TRACE(gentau::getGeneralLogger(), T_LOG_TAG fmt __VA_OPT__(, ) __VA_ARGS__)
#define tLogDebug(fmt, ...)                                                                        \
	SPDLOG_LOGGER_DEBUG(gentau::getGeneralLogger(), T_LOG_TAG fmt __VA_OPT__(, ) __VA_ARGS__)
#define tLogInfo(fmt, ...)                                                                         \
	SPDLOG_LOGGER_INFO(gentau::getGeneralLogger(), T_LOG_TAG fmt __VA_OPT__(, ) __VA_ARGS__)
#define tLogWarn(fmt, ...)                                                                         \
	SPDLOG_LOGGER_WARN(gentau::getGeneralLogger(), T_LOG_TAG fmt __VA_OPT__(, ) __VA_ARGS__)
#define tLogError(fmt, ...)                                                                        \
	SPDLOG_LOGGER_ERROR(gentau::getGeneralLogger(), T_LOG_TAG fmt __VA_OPT__(, ) __VA_ARGS__)
#define tLogCritical(fmt, ...)                                                                     \
	SPDLOG_LOGGER_CRITICAL(gentau::getGeneralLogger(), T_LOG_TAG fmt __VA_OPT__(, ) __VA_ARGS__)

#else
#define tImgTransLogTrace(fmt, ...)    ((void)0)
#define tImgTransLogDebug(fmt, ...)    ((void)0)
#define tImgTransLogInfo(fmt, ...)     ((void)0)
#define tImgTransLogWarn(fmt, ...)     ((void)0)
#define tImgTransLogError(fmt, ...)    ((void)0)
#define tImgTransLogCritical(fmt, ...) ((void)0)

#define tCommLogTrace(fmt, ...)    ((void)0)
#define tCommLogDebug(fmt, ...)    ((void)0)
#define tCommLogInfo(fmt, ...)     ((void)0)
#define tCommLogWarn(fmt, ...)     ((void)0)
#define tCommLogError(fmt, ...)    ((void)0)
#define tCommLogCritical(fmt, ...) ((void)0)

#define tLogTrace(fmt, ...)    ((void)0)
#define tLogDebug(fmt, ...)    ((void)0)
#define tLogInfo(fmt, ...)     ((void)0)
#define tLogWarn(fmt, ...)     ((void)0)
#define tLogError(fmt, ...)    ((void)0)
#define tLogCritical(fmt, ...) ((void)0)

#endif