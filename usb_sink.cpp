/**
 * @file    usb_sink.cpp
 * @author  Samuel Martel
 * @date    2024-04-08
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
#include "usb_sink.h"

#include <task.h>

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
// #define LOG_BELL_E       "\a"
#define LOG_BELL_E
#define LOG_BELL_W
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
        case Level::error: return s_errorColor;
        case Level::warning: return s_warningColor;
        case Level::info: return s_infoColor;
        case Level::debug: return s_debugColor;
        case Level::trace: return s_traceColor;
        case Level::all:
        case Level::none:
        default: return "";
    }
}
}    // namespace

void UsbSink::onWrite(Level level, const char* string, size_t length)
{
    if (m_droppedMessages != 0) {
        char        msg[32];
        std::size_t len   = std::snprintf(&msg[0], sizeof(msg), "Dropped %d messages!\n\r", m_droppedMessages);
        m_droppedMessages = 0;
        onWrite(Level::error, &msg[0], len);
    }

    auto color = colorStrFromLevel(level);

    if (!color.empty()) { queueData(color.data(), color.size()); }

    const char* ptr = string;
    while (length != 0) {
        std::size_t chunkSize = std::min(m_bufferSize, length);
        queueData(ptr, length);
        length -= chunkSize;
        ptr += chunkSize;
    }

    if (!color.empty()) { queueData(s_resetColor.data(), s_resetColor.size()); }

    CDC_SendQueue(m_usb);
}

bool UsbSink::queueData(const char* data, std::size_t length)
{
//    TickType_t ticksToWait = s_maxWaitTime;
//    TimeOut_t  timeout;
//    vTaskSetTimeOutState(&timeout);

//    while (CDC_Queue(m_usb, reinterpret_cast<uint8_t*>(const_cast<char*>(data)), length) != USBD_OK) {
//        if (xTaskCheckForTimeOut(&timeout, &ticksToWait) == pdFALSE) {
//            // No timeout yet.
//            vTaskDelay(pdMS_TO_TICKS(1));    // Block the task for 1ms, hoping to clear the USB.
//        }
//        else {
//            ++m_droppedMessages;
//            return true;
//        }
//    }
    CDC_Queue(m_usb, reinterpret_cast<uint8_t*>(const_cast<char*>(data)), length);

    return false;
}
}    // namespace Logging
