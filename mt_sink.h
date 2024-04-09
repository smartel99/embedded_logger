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
#include <semphr.h>
#include <task.h>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdio>
#include <utility>

#if (INCLUDE_vTaskDelete != 1)
#    error "vTaskDelete is required by MtSink, please set INCLUDE_vTaskDelete to 1"
#endif
extern unsigned int g_yieldedCauseFull;
namespace Logging {
/**
 * Multi-Producer, Single Consumer sink.
 * @tparam T
 *
 * @attention T's onWrite method must accept strings that are not null terminated.
 */
template<std::derived_from<Sink> T>
class MtSink : public Sink {
    struct MessageHeader {
        Level       level = {};
        std::size_t len   = 0;
    };

    //! Size in bytes.
    static constexpr std::size_t s_messageMaxLen          = 128;
    static constexpr std::size_t s_maxLenMessagesInBuffer = 8;
    //! Size of the header used internally by FreeRTOS's message buffer.
    static constexpr std::size_t s_internalMessageHeaderLen = sizeof(configMESSAGE_BUFFER_LENGTH_TYPE);
    //! A message always has the Level, the length of the message, then the message itself.
    static constexpr std::size_t s_messageBufferSize =
      (sizeof(MessageHeader) + s_messageMaxLen + s_internalMessageHeaderLen) * s_maxLenMessagesInBuffer;

    //! Maximum amount of time (in ms) that a producer can wait when writing messages.
    static constexpr auto s_producerMaxBlockTime = portMAX_DELAY;

    static constexpr std::size_t s_taskStackSize =
      configMINIMAL_STACK_SIZE * 2 + (s_messageMaxLen / sizeof(configSTACK_DEPTH_TYPE));
    static constexpr UBaseType_t s_taskPriority = 1;    //!< Low priority.
    //! Task will check whether it needs to stop running once every `s_taskRefreshPeriod` ms.
    static constexpr std::size_t s_taskRefreshPeriod = pdMS_TO_TICKS(10);

    MessageBufferHandle_t m_messageBuffer = nullptr;
    TaskHandle_t          m_task          = nullptr;

    SemaphoreHandle_t m_semaphoreHandle = nullptr;
    StaticSemaphore_t m_semaphoreBuffer = {};

    volatile bool m_taskShouldRun = true;
    volatile bool m_taskIsRunning = false;

    std::size_t m_messagesDropped = 0;

public:
    template<typename... Args>
        requires std::constructible_from<T, Args...>
    MtSink(Args&&... args) : m_sink(std::forward<Args>(args)...)
    {
        // Create semaphore,
        m_semaphoreHandle = xSemaphoreCreateMutexStatic(&m_semaphoreBuffer);
        configASSERT(m_semaphoreHandle != nullptr);

        // Create message buffer,
        m_messageBuffer = xMessageBufferCreate(s_messageBufferSize);
        configASSERT(m_messageBuffer != nullptr);

        // Create task,
        auto res = xTaskCreate(&task, "MtSink", s_taskStackSize, this, configMAX_PRIORITIES - 1, &m_task);
        configASSERT(res == pdPASS);
    }
    MtSink(const MtSink&)            = delete;
    MtSink& operator=(const MtSink&) = delete;
    MtSink(MtSink&&)                 = delete;
    MtSink& operator=(MtSink&&)      = delete;

    // TODO should we wait for message buffer to be empty?
    ~MtSink() override
    {
        if (m_taskIsRunning && m_task != nullptr) {
            // Ask the worker to shut down; it will delete itself.
            m_taskShouldRun = false;
            // Increase its priority to ours +1 so that it can stop sooner.
            vTaskPrioritySet(m_task, uxTaskPriorityGet(nullptr) + 1);
            while (m_taskIsRunning) {
                portYIELD();
            }

            // We can now assume that the worker will not be using the message buffer, and that producers will not try
            // to take the semaphore and write to the message buffer.
            vMessageBufferDelete(m_messageBuffer);
            vSemaphoreDelete(m_semaphoreHandle);
        }
    }

    /**
     * Queues the message to be sent to the real sink.
     *
     * @param level
     * @param string
     * @param length
     *
     * @attention When called from an interrupt, the message will be dropped if:
     *  - The mutex is already locked, either from an irq with lower preemption or from the task that was interrupted
     *  <br>- The message buffer does not have enough room to fit the whole message in it.
     */
    void onWrite(Level level, const char* string, std::size_t length) override
    {
        if (string == nullptr || length == 0) {
            // Don't do work for no reason lol
            return;
        }
        if (!m_taskIsRunning) {
            // In space, no one can hear you scream.
            // The worker isn't running, so why should we bother?
            return;
        }

        if ((portNVIC_INT_CTRL_REG & 0x1FF) != 0) {
            // Function was called from an interrupt.
            onWriteIrq(level, string, length);
        }
        else {
            onWriteBlocking(level, string, length);
        }
    }

protected:
    T            m_sink;
    virtual void onWriteImpl(Level level, const char* string, std::size_t length)
    {
        m_sink.onWrite(level, string, length);
    }

private:
    void onWriteBlocking(Level level, const char* string, std::size_t length)
    {
        // TODO should we set a timeout to lock? If yes, what do we do on timeout, drop the message?
        if (xSemaphoreTake(m_semaphoreHandle, s_producerMaxBlockTime) == pdPASS) {
            // Send the message header first.
            MessageHeader header {level, length};
            xMessageBufferSend(m_messageBuffer, &header, sizeof(header), s_producerMaxBlockTime);

            // Send the message in chunks that can be read by the consumer.
            const char* ptr = string;
            while (length != 0) {
                std::size_t chunkLength = std::min(length, s_messageMaxLen);
                xMessageBufferSend(m_messageBuffer, ptr, chunkLength, s_producerMaxBlockTime);
                ptr += chunkLength;
                length -= chunkLength;
            }
            xSemaphoreGive(m_semaphoreHandle);
        }
        else {
            // Unable to take the mutex, drop the message.
            ++m_messagesDropped;
        }
    }

    void onWriteIrq(Level level, const char* string, std::size_t length)
    {
        if (xSemaphoreTakeFromISR(m_semaphoreHandle, nullptr) == pdPASS) {
            if (xMessageBufferSpaceAvailable(m_messageBuffer) >= getRealMessageLen(length)) {
                // Send the message header first.
                MessageHeader header {level, length};
                xMessageBufferSendFromISR(m_messageBuffer, &header, sizeof(header), nullptr);

                // Send the message in chunks that can be read by the consumer.
                const char* ptr = string;
                while (length != 0) {
                    std::size_t chunkLength = std::min(length, s_messageMaxLen);
                    xMessageBufferSendFromISR(m_messageBuffer, ptr, chunkLength, nullptr);
                    ptr += chunkLength;
                    length -= chunkLength;
                }
            }
            else {
                // Not enough room available in the buffer for the entire message, drop the message.
                ++m_messagesDropped;
            }

            xSemaphoreGiveFromISR(m_semaphoreHandle, nullptr);
        }
        else {
            // Unable to take the mutex, drop the message.
            ++m_messagesDropped;
        }
    }

    /**
     * Calculates the number of bytes a message of size len will actually occupy in the message buffer, including all
     * the overhead.
     * @param len The length of the message
     * @return The number of bytes that the message would take in the message buffer.
     */
    std::size_t getRealMessageLen(std::size_t len)
    {
        std::size_t realLen = s_internalMessageHeaderLen + sizeof(MessageHeader);

        while (len != 0) {
            std::size_t chunkLen = std::min(len, s_messageMaxLen);
            realLen += s_internalMessageHeaderLen + chunkLen;
            len -= chunkLen;
        }

        return realLen;
    }

    [[noreturn]] static void task(void* args)
    {
        configASSERT(args != nullptr);

        auto& that           = *reinterpret_cast<MtSink*>(args);
        that.m_taskIsRunning = true;
        vTaskPrioritySet(nullptr, s_taskPriority);

        enum class States : std::uint8_t { ReceiveHeader = 0, ReceiveChunks };
        States currentState = States::ReceiveHeader;

        MessageHeader currentHeader = {};
        auto          receiveHeader = [&] -> bool {
            if (that.m_messagesDropped != 0) {
                char        msg[30];
                std::size_t len = std::snprintf(&msg[0], sizeof(msg), "Dropped %d messages!", that.m_messagesDropped);
                that.onWriteImpl(Level::error, &msg[0], len);
                that.m_messagesDropped = 0;
            }

            // If we haven't received anything, we're good, keep waiting for a header.
            // If we've received something that isn't a header, we fucked up, so wait for the next thing that *looks*
            // like a header.
            return xMessageBufferReceive(
                     that.m_messageBuffer, &currentHeader, sizeof(currentHeader), s_taskRefreshPeriod) ==
                   sizeof(currentHeader);
        };

        auto receiveChunk = [&] -> bool {
            char        rxBuff[s_messageMaxLen];
            std::size_t received =
              xMessageBufferReceive(that.m_messageBuffer, &rxBuff[0], s_messageMaxLen, s_taskRefreshPeriod);
            if (received > currentHeader.len) {
                // Uh oh, we might have received something not related to the current message!!
                // Return in ReceiveHeader mode, to resync.
                return true;
            }

            that.m_sink.onWrite(currentHeader.level, &rxBuff[0], received);
            currentHeader.len -= received;
            return currentHeader.len == 0;    // When length is 0, there's no more chunks to be received.
        };

        while (that.m_taskShouldRun) {
            bool shouldSwitchState = false;
            switch (currentState) {
                case States::ReceiveHeader: shouldSwitchState = receiveHeader(); break;
                case States::ReceiveChunks: shouldSwitchState = receiveChunk(); break;
            }
            if (shouldSwitchState) {
                if (currentState == States::ReceiveHeader) { currentState = States::ReceiveChunks; }
                else {
                    currentState = States::ReceiveHeader;
                }
            }
        }

        that.m_taskIsRunning = false;
        // `that` is now dangling, do not use it anymore!
        vTaskDelete(nullptr);
        std::unreachable();
    }
};
}    // namespace Logging

#endif    // VENDOR_LOGGING_MT_SINK_H
