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

// TODO the whole sink thing begs for dangling pointers to happen when a sink or a logger gets removed...
namespace Logging {
class Logger {
    struct LoggerInstance {
        std::optional<Level>                              level = std::nullopt;
        std::optional<std::vector<std::unique_ptr<Sink>>> sinks = std::nullopt;
    };

public:
    struct LoggerView {
        std::string_view                    tag;
        Level*                              level = &s_globalLevel;
        std::vector<std::unique_ptr<Sink>>* sinks = &s_globalSinks;

        bool shouldLog(Level desiredLevel) const { return desiredLevel <= *level; }
    };

    using GetTimeFunc = std::uint32_t (*)();

private:
    //! print number of bytes per line for writeHexArray and writeCharArray
    static constexpr std::size_t                     s_bytesPerLine = 16;
    static constexpr Level                           s_defaultLevel = Level::all;
    inline static Level                              s_globalLevel  = s_defaultLevel;
    inline static std::vector<std::unique_ptr<Sink>> s_globalSinks  = {};

    inline static std::unordered_map<std::string_view, LoggerInstance> s_loggers = {};
    inline static GetTimeFunc                                          s_getTime = [] -> std::uint32_t { return 0; };

public:
    static void          setGetTime(GetTimeFunc getTime);
    static std::uint32_t getTime() { return s_getTime(); }

    static LoggerView getLogger(std::string_view tag);

    // TODO: This should allow us to add an already existing sink...
    template<typename T, typename... Args>
        requires std::derived_from<T, Sink> && std::constructible_from<T, Args...>
    static T* addSink(Args&&... args)
    {
        s_globalSinks.push_back(std::make_unique<T>(std::forward<Args>(args)...));
        return static_cast<T*>(s_globalSinks.back().get());
    }
    static void  clearSinks() { s_globalSinks.clear(); }
    static void  setLevel(Level level) { s_globalLevel = level; }
    static Level getLevel() { return s_globalLevel; }
    static void  clearLevel() { s_globalLevel = s_defaultLevel; }

    template<typename T, typename... Args>
        requires std::derived_from<T, Sink> && std::constructible_from<T, Args...>
    static T* addSink(std::string_view tag, Args&&... args)
    {
        auto& sink = s_loggers[tag];
        if (!sink.sinks) { sink.sinks = std::vector<std::unique_ptr<Sink>> {}; }
        sink.sinks->push_back(std::make_unique<T>(std::forward<Args>(args)...));
        return static_cast<T*>(sink.sinks->back().get());
    }

    static void  clearSinks(std::string_view tag);
    static void  setLevel(std::string_view tag, Level level);
    static Level getLevel(std::string_view tag);
    static void  clearLevel(std::string_view tag);

    static void write(LoggerView logger, Level level, const char* fmt, ...);
    static void vWrite(LoggerView logger, Level level, const char* fmt, va_list args);

    /**
     * @brief Log a buffer of hex bytes at specified level, separated into 16 bytes each line.
     *
     * @param  logger view of the logger and its sinks
     * @param  level    level of the log
     * @param  buf   Pointer to the buffer array
     * @param  len length of buffer in bytes
     */
    static void writeHexArray(LoggerView logger, Level level, const std::uint8_t* buff, std::size_t len);

    /**
     * @brief Log a buffer of characters at specified level, separated into 16 bytes each line. Buffer should contain
     * only printable characters.
     *
     * @param  logger view of the logger and its sinks
     * @param  level    level of the log
     * @param  buf   Pointer to the buffer array
     * @param  len length of buffer in bytes
     */
    static void writeCharArray(LoggerView logger, Level level, const std::uint8_t* buff, std::size_t len);

    /**
     * @brief Dump a buffer to the log at specified level.
     *
     * The dump log shows just like the one below:
     *
     *      W (195) log_example: 0x3ffb4280   45 53 50 33 32 20 69 73  20 67 72 65 61 74 2c 20  |ESP32 is great, |
     *      W (195) log_example: 0x3ffb4290   77 6f 72 6b 69 6e 67 20  61 6c 6f 6e 67 20 77 69  |working along wi|
     *      W (205) log_example: 0x3ffb42a0   74 68 20 74 68 65 20 49  44 46 2e 00              |th the IDF..|
     *
     * It is highly recommended to use terminals with over 102 text width.
     *
     * @param  logger view of the logger and its sinks
     * @param  level level of the log
     * @param  buf Pointer to the buffer array
     * @param  len length of buffer in bytes
     */
    static void writeHexdumpArray(LoggerView logger, Level level, const std::uint8_t* buff, std::size_t len);
};
}    // namespace Logging

#define LOGGER_HELPER_MSG_IS_STRING_LITERAL_IMPL(x)                                                                    \
    ([&]<typename T = char>() {                                                                                        \
        return std::is_same_v<decltype(x), T const(&)[sizeof(x)]> &&                                                   \
               requires { std::type_identity_t<T[sizeof(x) + 1]> {x}; };                                               \
    }())
#define LOGGER_HELPER_MSG_IS_STRING_LITERAL(x)                                                                         \
    static_assert(LOGGER_HELPER_MSG_IS_STRING_LITERAL_IMPL(x), "msg must be a string literal!")

// Clang-tidy is recommending that size information should be provided as well since std::string_view::data() is not
// guaranteed to be null-terminated. However:
//  - We're giving that string to snprintf; it doesn't give a fuck about the size,
//  - It is expected that users *only* uses string literals as the names of their loggers, so we *should* only ever get
//  null-terminated string_views
#define LOGGER_LOG_HELPER_IMPL_TAG_GETTER(logger) (logger).tag.data()    // NOLINT(*-suspicious-stringview-data-usage)

#define LOGGER_LOG_HELPER_IMPL(logger, level, msg, ...)                                                                \
    do {                                                                                                               \
        LOGGER_HELPER_MSG_IS_STRING_LITERAL(msg);                                                                      \
        ::Logging::Logger::write(logger,                                                                               \
                                 level,                                                                                \
                                 "%c (%05lu) [%s] " msg "\r\n",                                                        \
                                 ::Logging::levelToChar(level),                                                        \
                                 ::Logging::Logger::getTime(),                                                         \
                                 LOGGER_LOG_HELPER_IMPL_TAG_GETTER(logger) __VA_OPT__(, ) __VA_ARGS__);                \
    } while (0)

#define LOGGER_LOG_HELPER(tag, level, msg, ...)                                                                        \
    LOGGER_LOG_HELPER_IMPL(::Logging::Logger::getLogger(tag), level, msg, __VA_ARGS__)

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

#define LOGGER_LOG_BUFFER_DUMP_HELPER(kind, tag, level, buff, len)                                                     \
    ::Logging::Logger::write##kind##Array(::Logging::Logger::getLogger(tag), level, buff, len)

#define LOG_BUFFER_HEX_LEVEL(tag, level, buffer, len)  LOGGER_LOG_BUFFER_DUMP_HELPER(Hex, tag, level, buffer, len)
#define LOG_BUFFER_CHAR_LEVEL(tag, level, buffer, len) LOGGER_LOG_BUFFER_DUMP_HELPER(Char, tag, level, buffer, len)
#define LOG_BUFFER_HEXDUMP_LEVEL(tag, level, buffer, len)                                                              \
    LOGGER_LOG_BUFFER_DUMP_HELPER(Hexdump, tag, level, buffer, len)

#define LOG_BUFFER_HEX(tag, buffer, len)     LOG_BUFFER_HEX_LEVEL(tag, ::Logging::Level::info, buffer, len)
#define LOG_BUFFER_CHAR(tag, buffer, len)    LOG_BUFFER_CHAR_LEVEL(tag, ::Logging::Level::info, buffer, len)
#define LOG_BUFFER_HEXDUMP(tag, buffer, len) LOG_BUFFER_HEXDUMP_LEVEL(tag, ::Logging::Level::info, buffer, len)

#define ROOT_BUFFER_HEX_LEVEL(level, buffer, len)     LOG_BUFFER_HEX_LEVEL(ROOT_LOGGER_TAG, level, buffer, len)
#define ROOT_BUFFER_CHAR_LEVEL(level, buffer, len)    LOG_BUFFER_CHAR_LEVEL(ROOT_LOGGER_TAG, level, buffer, len)
#define ROOT_BUFFER_HEXDUMP_LEVEL(level, buffer, len) LOG_BUFFER_HEXDUMP_LEVEL(ROOT_LOGGER_TAG, level, buffer, len)

#define ROOT_BUFFER_HEX(buffer, len)     ROOT_BUFFER_HEX_LEVEL(::Logging::Level::info, buffer, len)
#define ROOT_BUFFER_CHAR(buffer, len)    ROOT_BUFFER_CHAR_LEVEL(::Logging::Level::info, buffer, len)
#define ROOT_BUFFER_HEXDUMP(buffer, len) ROOT_BUFFER_HEXDUMP_LEVEL(::Logging::Level::info, buffer, len)



#endif    // VENDOR_LOGGING_LOGGER_H
