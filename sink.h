/**
 * @file    sink.h
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
#ifndef SINK_H
#define SINK_H

#include <cstdarg>
#include <cstddef>

#include "level.h"

namespace Logging {

class Sink {
 public:
  virtual ~Sink() = default;

  virtual void onWrite(Level level, const char* string, size_t length) = 0;
};

}  // namespace Logging

#endif  // SINK_H
