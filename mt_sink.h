/**
 * @file    mt_sink.h
 * @author  Samuel Martel
 * @date    2024-04-02
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


#ifndef VENDOR_LOGGING_MT_SINK_H
#define VENDOR_LOGGING_MT_SINK_H

#include "sink.h"

#include <FreeRTOS.h>
#include <message_buffer.h>
#include <task.h>

#include <atomic>
#include <concepts>

namespace Logging {
template<std::derived_from<Sink> T>
class MtSink : public T {
    MessageBufferHandle_t m_messageBuffer = nullptr;
    TaskHandle_t          m_task          = nullptr;

public:
    template<typename... Args>
        requires std::constructible_from<T, Args...>
    MtSink(Args&&... args) : T(std::forward<Args>(args)...)
    {
        // Create message buffer,
        // Create task,
        // Abort if either of those fails.
    }
    MtSink(const MtSink&)            = delete;
    MtSink(MtSink&&)                 = delete;
    MtSink& operator=(const MtSink&) = delete;
    MtSink& operator=(MtSink&&)      = delete;

    // Close and delete task
    // Delete message buffer
    // TODO should we wait for message buffer to be empty?
    ~MtSink() override;

    void write(Level level, const char* string, std::size_t length) override;

private:
    [[noreturn]] void task(void* args);
};
}    // namespace Logging

#endif    // VENDOR_LOGGING_MT_SINK_H
