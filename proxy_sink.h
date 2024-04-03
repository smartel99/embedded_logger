/**
 * @file    proxy_sink.h
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


#ifndef VENDOR_LOGGING_PROXY_SINK_H
#define VENDOR_LOGGING_PROXY_SINK_H
#include "sink.h"
#include "mt_sink.h"

namespace Logging {

class ProxySink : public Sink {
private:
    Sink* m_sink;

public:
    explicit ProxySink(Sink* sink) : m_sink(sink) {}
    ProxySink(const ProxySink&)            = default;
    ProxySink(ProxySink&&)                 = default;
    ProxySink& operator=(const ProxySink&) = default;
    ProxySink& operator=(ProxySink&&)      = default;
    ~ProxySink() override                  = default;

    void onWrite(Level level, const char* string, size_t length) override { m_sink->onWrite(level, string, length); }
};

}    // namespace Logging
#endif    // VENDOR_LOGGING_PROXY_SINK_H
