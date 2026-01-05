#include "logger.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace qdriver::logger {

std::shared_ptr<Logger> LoggerFactory::defaultLogger_ = nullptr;

std::shared_ptr<Logger> LoggerFactory::createConsoleLogger(
    const std::string& name,
    spdlog::level::level_enum level
) {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(level);
    
    auto spdlogger = std::make_shared<spdlog::logger>(name, console_sink);
    spdlogger->set_level(level);
    spdlogger->flush_on(spdlog::level::warn);
    
    return std::make_shared<Logger>(spdlogger);
}

std::shared_ptr<Logger> LoggerFactory::createFileLogger(
    const std::string& name,
    const std::string& filename,
    spdlog::level::level_enum level
) {
    // 使用旋转文件 sink ,限制单文件 5MB, 最多保留3个文件
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        filename, 1024 * 1024 * 5, 3
    );
    file_sink->set_level(level);
    
    auto spdlogger = std::make_shared<spdlog::logger>(name, file_sink);
    spdlogger->set_level(level);
    spdlogger->flush_on(spdlog::level::warn);
    
    return std::make_shared<Logger>(spdlogger);
}

std::shared_ptr<Logger> LoggerFactory::createCombinedLogger(
    const std::string& name,
    const std::string& filename,
    spdlog::level::level_enum level
) {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(level);
    
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        filename, 1024 * 1024 * 5, 3
    );
    file_sink->set_level(level);
    
    std::vector<spdlog::sink_ptr> sinks { console_sink, file_sink };
    auto spdlogger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
    spdlogger->set_level(level);
    spdlogger->flush_on(spdlog::level::warn);
    
    return std::make_shared<Logger>(spdlogger);
}

std::shared_ptr<Logger> LoggerFactory::getDefaultLogger() {
    if (!defaultLogger_) {
        defaultLogger_ = createConsoleLogger("qdriver_default", spdlog::level::info);
    }
    return defaultLogger_;
}

} // namespace qdriver::logger
