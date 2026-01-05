#pragma once

#include <spdlog/spdlog.h>

namespace qdriver::logger {

/**
 * @brief 基于 spdlog 的 Logger 包装
 */
class Logger {
public:
    Logger() = default;
    explicit Logger(std::shared_ptr<spdlog::logger> logger): logger_(logger) {}

    template<typename... Args>
    void trace(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        if (logger_) {
            logger_->trace(fmt, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void debug(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        if (logger_) {
            logger_->debug(fmt, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void info(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        if (logger_) {
            logger_->info(fmt, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void warn(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        if (logger_) {
            logger_->warn(fmt, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void error(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        if (logger_) {
            logger_->error(fmt, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void critical(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        if (logger_) {
            logger_->critical(fmt, std::forward<Args>(args)...);
        }
    }

    std::shared_ptr<spdlog::logger> getSpdLogger() const {
        return logger_;
    }

private:
    std::shared_ptr<spdlog::logger> logger_;
};

/**
 * @brief Logger 工厂类
 */
class LoggerFactory {
public:
    /**
     * @brief 创建控制台 Logger
     * @param name 日志器名称
     * @param level 日志等级（默认 info）
     * @return Logger 智能指针
     */
    static std::shared_ptr<Logger> createConsoleLogger(
        const std::string& name,
        spdlog::level::level_enum level = spdlog::level::info
    );

    /**
     * @brief 创建文件 Logger
     * @param name 日志器名称
     * @param filename 日志文件路径
     * @param level 日志等级（默认 info）
     * @return Logger 智能指针
     */
    static std::shared_ptr<Logger> createFileLogger(
        const std::string& name,
        const std::string& filename,
        spdlog::level::level_enum level = spdlog::level::info
    );

    /**
     * @brief 创建组合 Logger（控制台 + 文件）
     * @param name 日志器名称
     * @param filename 日志文件路径
     * @param level 日志等级（默认 info）
     * @return Logger 智能指针
     */
    static std::shared_ptr<Logger> createCombinedLogger(
        const std::string& name,
        const std::string& filename,
        spdlog::level::level_enum level = spdlog::level::info
    );

    /**
     * @brief 获取或创建默认 Logger
     * @return 默认 Logger 智能指针
     */
    static std::shared_ptr<Logger> getDefaultLogger();

private:
    static std::shared_ptr<Logger> defaultLogger_;
};

} // namespace qdriver::logger
