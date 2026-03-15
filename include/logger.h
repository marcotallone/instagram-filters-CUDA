#pragma once

#ifdef ENABLE_LOGGING
#include <chrono>
#include <filesystem>
#include <string>

#include "config.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

inline void init_logger() {
    // Create logs/ directory if it doesn't exist
    std::filesystem::create_directories(config::LOGS_PATH);

    // Define log filename using date in format YYYYMMDD
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_time);
    char date_buffer[16];
    std::strftime(date_buffer, sizeof(date_buffer), "%Y%m%d", now_tm);
    std::string log_filename =
        config::LOGS_PATH / ("log_" + std::string(date_buffer) + ".log");

    // Define logging sinks
    std::vector<spdlog::sink_ptr> sinks;

    // File sink (always)
    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        log_filename,
        false  // ← false = append, don't overwrite existing logs
        ));

#ifdef ENABLE_DEBUG
    // Console sink (optional, for debug only)
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
#endif

    // Define logger with initialized sinks
    auto logger =
        std::make_shared<spdlog::logger>("logger", sinks.begin(), sinks.end());

    // Set pattern to include timestamp, level, function name, and message
    // Pattern tokens:
    //     %Y-%m-%d %H:%M:%S.%e — timestamp with milliseconds
    //     %l — log level (debug, info, error)
    //     %! — function name ← this is what you want
    //     %v — message
    // logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%!] %v");
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%#:%!] %v");
    // Example:
    // [2026-03-08 12:36:38.669] [debug] [load_images] Loaded: Lena.png
    // [512x512] [2026-03-08 12:36:38.669] [info] [load_images] Loaded 1 images

    logger->set_level(spdlog::level::debug);
    logger->flush_on(spdlog::level::err);

    // Only register if not already registered (avoids throw when
    // init_logger() is called multiple times in the same process)
    if (!spdlog::get("logger")) { spdlog::register_logger(logger); }
}

// Null-safe: skip logging silently if init_logger() was never called
#define LOG_INFO(msg)                                             \
    do {                                                          \
        auto _l = spdlog::get("logger");                          \
        if (_l) SPDLOG_LOGGER_CALL(_l, spdlog::level::info, msg); \
    } while (0)
#define LOG_ERROR(msg)                                           \
    do {                                                         \
        auto _l = spdlog::get("logger");                         \
        if (_l) SPDLOG_LOGGER_CALL(_l, spdlog::level::err, msg); \
    } while (0)
#define LOG_DEBUG(msg)                                             \
    do {                                                           \
        auto _l = spdlog::get("logger");                           \
        if (_l) SPDLOG_LOGGER_CALL(_l, spdlog::level::debug, msg); \
    } while (0)

#else
// When ENABLE_LOGGING is OFF, all these are no-ops
inline void init_logger() {}
#define LOG_INFO(msg) ((void)0)
#define LOG_ERROR(msg) ((void)0)
#define LOG_DEBUG(msg) ((void)0)
#endif