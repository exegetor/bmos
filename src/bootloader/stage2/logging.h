#pragma once

#define LOG_TRACE 0x01
#define LOG_DEBUG 0x02
#define LOG_INFO  0x04
#define LOG_WARN  0x08
#define LOG_ERROR 0x10
#define LOG_FATAL 0x20

/*
** edit this line for global effect _OR_ use makefile to override
*/
#if !defined LOG_LEVEL
#	define LOG_LEVEL LOG_TRACE+LOG_DEBUG+LOG_INFO+LOG_WARN+LOG_ERROR+LOG_FATAL
#endif


#if LOG_LEVEL & LOG_TRACE == LOG_TRACE
#	define TRACE(fmt, ...) printf("[TRACE] " fmt, ##__VA_ARGS__)
#else
#	define TRACE(fmt, ...)
#endif

#if LOG_LEVEL & LOG_DEBUG == LOG_DEBUG
#	define DEBUG(fmt, ...) printf("[DEBUG] " fmt, ##__VA_ARGS__)
#else
#	define DEBUG(fmt, ...)
#endif

#if LOG_LEVEL & LOG_INFO == LOG_INFO
#	define INFO(fmt, ...) printf(" [INFO] " fmt, ##__VA_ARGS__)
#else
#	define INFO(fmt, ...)
#endif

#if LOG_LEVEL & LOG_WARN == LOG_WARN
#	define WARN(fmt, ...) printf(" [WARN] " fmt, ##__VA_ARGS__)
#else
#	define WARN(fmt, ...)
#endif

#if LOG_LEVEL & LOG_ERROR == LOG_ERROR
#	define ERROR(fmt, ...) printf("[ERROR] " fmt, ##__VA_ARGS__)
#else
#	define ERROR(fmt, ...)
#endif

#if LOG_LEVEL & LOG_FATAL == LOG_FATAL
#	define FATAL(fmt, ...) printf("[FATAL] " fmt, ##__VA_ARGS__)
#else
#	define FATAL(fmt, ...)
#endif
