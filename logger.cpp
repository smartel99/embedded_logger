/**
 * @file    logger.cpp
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

#include "logger.h"

#include <cassert>
#include <cstdarg>
#include <cstdio>

namespace Logging {
Logger::LoggerStorage::iterator Logger::getLoggerInstance(std::string_view tag)
{
    for (auto it = s_loggers.begin(); it != s_loggers.end(); ++it) {
        if (it->tag == tag) { return it; }
    }
    return s_loggers.end();
}

void Logger::setGetTime(GetTimeFunc getTime)
{
    assert(getTime != nullptr && "getTime func can't be null!");
    s_getTime = getTime;
}

void Logger::write(LoggerView logger, Level level, const char* fmt, ...)
{
    if (!logger.shouldLog(level)) {
        // This level is disabled.
        return;
    }
    va_list args;
    va_start(args, fmt);
    vWrite(logger, level, fmt, args);
    va_end(args);
}

void Logger::vWrite(LoggerView logger, Level level, const char* fmt, va_list args)
{
    if (!logger.shouldLog(level)) {
        // This level is disabled.
        return;
    }
    static constexpr size_t maxLength = 512;
    static char             buffer[maxLength];
    size_t                  length = vsnprintf(&buffer[0], maxLength, fmt, args);
    assert(length < maxLength && "String too long to be logged");
    for (auto&& sink : s_globalSinks) {
        sink->onWrite(level, &buffer[0], length);
    }
}

void Logger::setLevel(std::string_view tag, Level level)
{
    auto it = getLoggerInstance(tag);
    if (it != s_loggers.end()) {
        it->level = level;
        return;
    }
    s_loggers.emplace_back(tag, level);
}

Level Logger::getLevel(std::string_view tag)
{
    auto [t, level] = getLogger(tag);
    return *level;
}

void Logger::clearLevel(std::string_view tag)
{
    auto it = getLoggerInstance(tag);
    // logger doesn't exist, do nothing.
    if (it == s_loggers.end()) { return; }
    it->level = std::nullopt;
}

Logger::LoggerView& Logger::getLogger(std::string_view tag)
{
    static LoggerView loggerView = {.tag = tag, .level = &s_globalLevel};
    loggerView                   = {.tag = tag, .level = &s_globalLevel};
    if (auto loggerIt = getLoggerInstance(tag); loggerIt != s_loggers.end()) {
        if (loggerIt->level.has_value()) { loggerView.level = &loggerIt->level.value(); }
    }
    return loggerView;
}

void Logger::writeHexArray(LoggerView logger, Level level, const std::uint8_t* buff, std::size_t len)
{
    if (!logger.shouldLog(level)) { return; }
    if (len == 0 || buff == nullptr) { return; }
    char                hexBuffer[3 * s_bytesPerLine + 1];
    const std::uint8_t* ptrLine      = nullptr;
    std::size_t         bytesCurLine = 0;

    do {
        if (len > s_bytesPerLine) { bytesCurLine = s_bytesPerLine; }
        else {
            bytesCurLine = len;
        }
        ptrLine = buff;

        for (std::size_t i = 0; i < bytesCurLine; i++) {
            std::snprintf(&hexBuffer[0] + 3 * i, sizeof(hexBuffer) - (3 * i), "%02x ", ptrLine[i]);
        }
        LOGGER_LOG_HELPER_IMPL(logger, level, "%s", &hexBuffer[0]);
        buff += bytesCurLine;
        len -= bytesCurLine;
    } while (len != 0);
}

void Logger::writeCharArray(LoggerView logger, Level level, const std::uint8_t* buff, std::size_t len)
{
    if (!logger.shouldLog(level)) { return; }
    if (len == 0 || buff == nullptr) { return; }
    char                charBuffer[s_bytesPerLine + 1];
    const std::uint8_t* ptrLine      = nullptr;
    std::size_t         bytesCurLine = 0;

    do {
        if (len > s_bytesPerLine) { bytesCurLine = s_bytesPerLine; }
        else {
            bytesCurLine = len;
        }
        ptrLine = buff;

        for (std::size_t i = 0; i < bytesCurLine; i++) {
            sprintf(&charBuffer[0] + i, "%c", ptrLine[i]);
        }
        LOGGER_LOG_HELPER_IMPL(logger, level, "%s", &charBuffer[0]);
        buff += bytesCurLine;
        len -= bytesCurLine;
    } while (len != 0);
}

void Logger::writeHexdumpArray(LoggerView logger, Level level, const std::uint8_t* buff, std::size_t len)
{
    if (!logger.shouldLog(level)) { return; }
    if (len == 0 || buff == nullptr) { return; }
    const std::uint8_t* ptrLine = nullptr;
    // format: field[length]
    //  ADDR[10]+"   "+DATA_HEX[8*3]+" "+DATA_HEX[8*3]+"  |"+DATA_CHAR[8]+"|"
    char        hdBuffer[10 + 3 + s_bytesPerLine * 3 + 3 + s_bytesPerLine + 1 + 1];
    char*       ptrHd        = nullptr;
    std::size_t bytesCurLine = 0;

    do {
        if (len > s_bytesPerLine) { bytesCurLine = s_bytesPerLine; }
        else {
            bytesCurLine = len;
        }
        ptrLine = buff;
        ptrHd   = &hdBuffer[0];

        ptrHd += std::sprintf(ptrHd, "%p ", reinterpret_cast<const void*>(buff));
        for (std::size_t i = 0; i < s_bytesPerLine; i++) {
            if ((i & 7) == 0) { ptrHd += std::sprintf(ptrHd, " "); }
            if (i < bytesCurLine) { ptrHd += std::sprintf(ptrHd, " %02x", ptrLine[i]); }
            else {
                ptrHd += std::sprintf(ptrHd, "   ");
            }
        }
        ptrHd += std::sprintf(ptrHd, "  |");
        for (std::size_t i = 0; i < bytesCurLine; i++) {
            if (std::isprint(static_cast<int>(ptrLine[i])) != 0) { ptrHd += std::sprintf(ptrHd, "%c", ptrLine[i]); }
            else {
                ptrHd += std::sprintf(ptrHd, ".");
            }
        }
        ptrHd += std::sprintf(ptrHd, "|");

        LOGGER_LOG_HELPER_IMPL(logger, level, "%s", &hdBuffer[0]);
        buff += bytesCurLine;
        len -= bytesCurLine;
    } while (len != 0);
}

}    // namespace Logging
