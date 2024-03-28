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
void Logger::setGetTime(Logger::GetTimeFunc getTime)
{
    assert(getTime != nullptr && "getTime func can't be null!");
    s_getTime = getTime;
}

void Logger::write(std::string_view tag, Level level, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vWrite(tag, level, fmt, args);
    va_end(args);
}

void Logger::vWrite(std::string_view tag, Level level, const char* fmt, va_list args)
{
    Level* currentLevel = &s_globalLevel;
    auto*  sinks        = &s_globalSinks;

    auto logger = s_loggers.find(tag);
    if (logger != s_loggers.end()) {
        if (logger->second.level.has_value()) { currentLevel = &logger->second.level.value(); }
        if (logger->second.sinks.has_value()) { sinks = &logger->second.sinks.value(); }
    }

    vWriteImpl(*currentLevel, *sinks, level, fmt, args);
}

void Logger::clearSinks(std::string_view tag)
{
    auto it = s_loggers.find(tag);
    // logger doesn't exist, do nothing.
    if (it == s_loggers.end()) { return; }
    it->second.sinks = std::nullopt;

    // If the logger doesn't have a custom level, straight up delete it from the list, it is useless now.
    if (!it->second.level.has_value()) { s_loggers.erase(it); }
}

void Logger::setLevel(std::string_view tag, Level level)
{
    s_loggers[tag].level = level;
}

void Logger::clearLevel(std::string_view tag)
{
    auto it = s_loggers.find(tag);
    // logger doesn't exist, do nothing.
    if (it == s_loggers.end()) { return; }
    it->second.level = std::nullopt;

    // If the logger doesn't have custom sinks, straight up delete it from the list, it is useless now.
    if (!it->second.sinks.has_value()) { s_loggers.erase(it); }
}

void Logger::vWriteImpl(
  Level currentLevel, std::vector<std::unique_ptr<Sink>>& sinks, Level desiredLevel, const char* fmt, va_list args)
{
    if (desiredLevel > currentLevel) {
        // This level is disabled.
        return;
    }
    static constexpr size_t maxLength = 500;
    char                    buffer[maxLength];
    size_t                  length = vsnprintf(buffer, maxLength, fmt, args);
    for (auto&& sink : sinks) {
        sink->onWrite(desiredLevel, buffer, length);
    }
}

}    // namespace Logging
