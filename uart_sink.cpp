/**
 * @file    uart_sink.cpp
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

#include "uart_sink.h"

#include <cstdio>
#include <string_view>

namespace Logging {

#define LOG_COLOR_BLACK  ";30"
#define LOG_COLOR_RED    ";31"
#define LOG_COLOR_GREEN  ";32"
#define LOG_COLOR_BROWN  ";33"
#define LOG_COLOR_BLUE   ";34"
#define LOG_COLOR_PURPLE ";35"
#define LOG_COLOR_CYAN   ";36"
#define LOG_COLOR(COLOR) "\033[0" COLOR "m"
#define LOG_BOLD(COLOR)  "\033[1" COLOR "m"
#define LOG_RESET_COLOR  "\033[0m"
#define LOG_COLOR_E      LOG_COLOR(LOG_COLOR_RED)
#define LOG_COLOR_W      LOG_COLOR(LOG_COLOR_BROWN)
#define LOG_COLOR_I      LOG_COLOR(LOG_COLOR_GREEN)
#define LOG_COLOR_D      LOG_RESET_COLOR
#define LOG_COLOR_T      LOG_COLOR(LOG_COLOR_CYAN)
#define LOG_BELL_E       "\a"
#define LOG_BELL_W       "\a"
#define LOG_BELL_I
#define LOG_BELL_D
#define LOG_BELL_T

namespace {
constexpr std::string_view s_errorColor   = LOG_COLOR_E LOG_BELL_E;
constexpr std::string_view s_warningColor = LOG_COLOR_W LOG_BELL_W;
constexpr std::string_view s_infoColor    = LOG_COLOR_I LOG_BELL_I;
constexpr std::string_view s_debugColor   = LOG_COLOR_D LOG_BELL_D;
constexpr std::string_view s_traceColor   = LOG_COLOR_T LOG_BELL_T;

constexpr std::string_view s_resetColor = LOG_RESET_COLOR;

constexpr std::string_view colorStrFromLevel(Level level)
{
    switch (level) {
        case Level::error: return LOG_COLOR_E LOG_BELL_E;
        case Level::warning: return LOG_COLOR_W LOG_BELL_W;
        case Level::info: return LOG_COLOR_I LOG_BELL_I;
        case Level::debug: return LOG_COLOR_D LOG_BELL_D;
        case Level::trace: return LOG_COLOR_T LOG_BELL_T;
        case Level::all:
        case Level::none:
        default: return "";
    }
}
}    // namespace

void UartSink::onWrite(Level level, const char* string, size_t length)
{
    auto color = colorStrFromLevel(level);
    if (!color.empty()) {
        HAL_UART_Transmit(m_uart, reinterpret_cast<const uint8_t*>(color.data()), color.size(), HAL_MAX_DELAY);
    }
    HAL_UART_Transmit(m_uart, reinterpret_cast<const uint8_t*>(string), length, HAL_MAX_DELAY);
    if (!color.empty()) {
        HAL_UART_Transmit(
          m_uart, reinterpret_cast<const uint8_t*>(s_resetColor.data()), s_resetColor.size(), HAL_MAX_DELAY);
    }
}

}    // namespace Logging
