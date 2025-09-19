/**
 * @file    usb_sink.h
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


#ifndef VENDOR_LOGGING_USB_SINK_H
#define VENDOR_LOGGING_USB_SINK_H
#include "mt_sink.h"
#include "sink.h"
#include "usbd_cdc_if.h"

#include <FreeRTOS.h>

namespace Logging {
class UsbSink : public Sink {
private:
    static constexpr std::size_t s_maxWaitTime    = pdMS_TO_TICKS(100);
    struct CDC_DeviceInfo*              m_usb            = nullptr;

    std::size_t m_bufferSize      = 0;
    std::size_t m_droppedMessages = 0;

public:
    explicit UsbSink(CDC_DeviceInfo* handle) : m_usb(handle), m_bufferSize(CDC_GetTxBufferSize(handle)) {}
    UsbSink(const UsbSink&)            = delete;
    UsbSink(UsbSink&&)                 = delete;
    UsbSink& operator=(const UsbSink&) = delete;
    UsbSink& operator=(UsbSink&&)      = delete;

    ~UsbSink() override = default;

    void onWrite(Level level, const char* string, std::size_t length) override;

private:
    /**
     * Tries to send data over USB.
     * @param data
     * @param length
     * @return True if operation failed, false otherwise.
     */
    bool queueData(const char* data, std::size_t length);
};

using MtUsbSink = MtSink<UsbSink>;

}    // namespace Logging
#endif    // VENDOR_LOGGING_USB_SINK_H
