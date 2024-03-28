/**
 * @file    logger.h
 * @author  Paul Thomas
 * @date    2023-12-04
 * @brief
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. If
 * not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */
#ifndef VENDOR_LOGGING_LOGGER_H
#define VENDOR_LOGGING_LOGGER_H

#include <cstdarg>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "level.h"
#include "sink.h"

namespace Logging {
class Logger {
    struct LoggerInstance {
        std::optional<Level>                              level = std::nullopt;
        std::optional<std::vector<std::unique_ptr<Sink>>> sinks = std::nullopt;
    };

    using GetTimeFunc = std::uint32_t (*)();

private:
    static constexpr Level                           s_defaultLevel = Level::all;
    inline static Level                              s_globalLevel  = s_defaultLevel;
    inline static std::vector<std::unique_ptr<Sink>> s_globalSinks  = {};

    inline static std::unordered_map<std::string_view, LoggerInstance> s_loggers = {};
    inline static GetTimeFunc                                          s_getTime = [] -> std::uint32_t { return 0; };

public:
    static void          setGetTime(GetTimeFunc getTime);
    static std::uint32_t getTime() { return s_getTime(); }

    // TODO: This should allow us to add an already existing sink...
    template<typename T, typename... Args>
        requires std::derived_from<T, Sink> && std::constructible_from<T, Args...>
    static void addSink(Args&&... args)
    {
        s_globalSinks.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }
    static void clearSinks() { s_globalSinks.clear(); }
    static void setLevel(Level level) { s_globalLevel = level; }
    static void clearLevel() { s_globalLevel = s_defaultLevel; }

    template<typename T, typename... Args>
        requires std::derived_from<T, Sink> && std::constructible_from<T, Args...>
    static void addSink(std::string_view tag, Args&&... args)
    {
        auto& sink = s_loggers[tag];
        if (!sink.sinks) { sink.sinks = {}; }
        sink.sinks->push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    static void clearSinks(std::string_view tag);
    static void setLevel(std::string_view tag, Level level);
    static void clearLevel(std::string_view tag);

    static void write(std::string_view tag, Level level, const char* fmt, ...);
    static void vWrite(std::string_view tag, Level level, const char* fmt, va_list args);

private:
    static void vWriteImpl(
      Level currentLevel, std::vector<std::unique_ptr<Sink>>& sinks, Level desiredLevel, const char* fmt, va_list args);
};
}    // namespace Logging

#define LOGGER_LOG_HELPER(tag, level, msg, ...)                                                                        \
    ::Logging::Logger::write(tag,                                                                                      \
                             level,                                                                                    \
                             "%s (%05lu) [" tag "] " msg "\r\n",                                                       \
                             ::Logging::levelToChar(level),                                                          \
                             ::Logging::Logger::getTime() __VA_OPT__(, ) __VA_ARGS__)

#define LOGT(tag, msg, ...) LOGGER_LOG_HELPER(tag, ::Logging::Level::trace, msg __VA_OPT__(, ) __VA_ARGS__)
#define LOGD(tag, msg, ...) LOGGER_LOG_HELPER(tag, ::Logging::Level::debug, msg __VA_OPT__(, ) __VA_ARGS__)
#define LOGI(tag, msg, ...) LOGGER_LOG_HELPER(tag, ::Logging::Level::info, msg __VA_OPT__(, ) __VA_ARGS__)
#define LOGW(tag, msg, ...) LOGGER_LOG_HELPER(tag, ::Logging::Level::warning, msg __VA_OPT__(, ) __VA_ARGS__)
#define LOGE(tag, msg, ...) LOGGER_LOG_HELPER(tag, ::Logging::Level::error, msg __VA_OPT__(, ) __VA_ARGS__)

#define ROOT_LOGGER_TAG "ROOT"

#define ROOT_LOGT(msg, ...) LOGT(ROOT_LOGGER_TAG, msg __VA_OPT__(, ) __VA_ARGS__)
#define ROOT_LOGD(msg, ...) LOGD(ROOT_LOGGER_TAG, msg __VA_OPT__(, ) __VA_ARGS__)
#define ROOT_LOGI(msg, ...) LOGI(ROOT_LOGGER_TAG, msg __VA_OPT__(, ) __VA_ARGS__)
#define ROOT_LOGW(msg, ...) LOGW(ROOT_LOGGER_TAG, msg __VA_OPT__(, ) __VA_ARGS__)
#define ROOT_LOGE(msg, ...) LOGE(ROOT_LOGGER_TAG, msg __VA_OPT__(, ) __VA_ARGS__)



#endif    // VENDOR_LOGGING_LOGGER_H
