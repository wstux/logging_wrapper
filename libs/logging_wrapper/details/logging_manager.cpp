/*
 * logging_wrapper
 * Copyright (C) 2024  Chistyakov Alexander
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/time.h>
#include <string.h>
#include <time.h>

#include "logging_wrapper/logging_manager.h"

#define TS_FILL_DFL(ts_buf, buf_size)                               \
    memcpy(ts_buf, "yyyy-MM-dd hh:mm:ss.mil", (buf_size < 24) ? buf_size : 24)

namespace wstux {
namespace logging {

manager::severity_level_t manager::m_global_level = {severity_level::info};

void manager::set_global_level(severity_level lvl)
{
    if ((lvl < severity_level::emerg) || (lvl > severity_level::trace)) {
        return;
    }
    m_global_level = lvl;
}

} // namespace logging
} // namespace wstux

#undef TS_FILL_DFL

