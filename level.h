/**
 * @file    level.h
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
#ifndef CEP_SERVICES_LOGGING_LEVEL_H
#define CEP_SERVICES_LOGGING_LEVEL_H

#include <cstdint>

namespace Logging {
enum class Level : std::uint8_t {
    none = 0,
    error,
    warning,
    info,
    debug,
    trace,
    all,
};

static char levelToChar(Level level)
{
    switch (level) {
        case Level::error: return 'E';
        case Level::warning: return 'W';
        case Level::info: return 'I';
        case Level::debug: return 'D';
        case Level::trace: return 'T';
        case Level::none:
        case Level::all:
        default: return '?';
    }
}
}    // namespace Logging

#endif    // CEP_SERVICES_LOGGING_LEVEL_H
