/**
 * @file    uart_sink.h
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
#ifndef UART_SINK_H
#define UART_SINK_H
#include "sink.h"
#include "usart.h"

namespace Logging {

class UartSink final : public Sink {
private:
    UART_HandleTypeDef* m_uart;

public:
    explicit UartSink(UART_HandleTypeDef* handle) : m_uart(handle) {}
    ~UartSink() final = default;

    void onWrite(Level level, const char* string, size_t length) final;
};

}    // namespace Logging

#endif    // UART_SINK_H
