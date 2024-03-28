/**
 * @file    file_sink.h
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
#ifndef DOORSTOPPER_SRC_CEP_SERVICES_LOGGING_FILE_SINK_H
#define DOORSTOPPER_SRC_CEP_SERVICES_LOGGING_FILE_SINK_H

#include "sink.h"
namespace Logging {

class FileSink : public Sink {
 public:
  void onWrite(Level level, const char* string, size_t length) final;
};

}  // namespace Logging

#endif  // DOORSTOPPER_SRC_CEP_SERVICES_LOGGING_FILE_SINK_H
