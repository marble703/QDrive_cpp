#pragma once

#include <memory>

#ifndef WITHOUT_LOGGER
#include <spdlog/spdlog.h>
#endif

namespace qdriver::logger {

#ifndef WITHOUT_LOGGER
// 使用 spdlog 的完整实现

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

#else // WITHOUT_LOGGER

/**
 * @brief 空实现的 Logger（禁用日志时使用）
 */
class Logger {
public:
    Logger() = default;

    template<typename... Args>
    void trace(const char*, Args&&...) {}

    template<typename... Args>
    void debug(const char*, Args&&...) {}

    template<typename... Args>
    void info(const char*, Args&&...) {}

    template<typename... Args>
    void warn(const char*, Args&&...) {}

    template<typename... Args>
    void error(const char*, Args&&...) {}

    template<typename... Args>
    void critical(const char*, Args&&...) {}
};

/**
 * @brief Logger 工厂类（禁用日志时的空实现）
 */
class LoggerFactory {
public:
    static std::shared_ptr<Logger> createConsoleLogger(
        const std::string&,
        int = 0
    ) {
        return std::make_shared<Logger>();
    }

    static std::shared_ptr<Logger> createFileLogger(
        const std::string&,
        const std::string&,
        int = 0
    ) {
        return std::make_shared<Logger>();
    }

    static std::shared_ptr<Logger> createCombinedLogger(
        const std::string&,
        const std::string&,
        int = 0
    ) {
        return std::make_shared<Logger>();
    }

    static std::shared_ptr<Logger> getDefaultLogger() {
        static auto logger = std::make_shared<Logger>();
        return logger;
    }
};

#endif // WITHOUT_LOGGER

} // namespace qdriver::logger
