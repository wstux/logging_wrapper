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

#ifndef _LOGGING_WRAPPER_LOGGING_WRAPPER_LOGGINGC_MANAGER_H_
#define _LOGGING_WRAPPER_LOGGING_WRAPPER_LOGGINGC_MANAGER_H_

#include <cassert>
#include <atomic>
#include <string>

#include "logging_wrapper/severity_level.h"

namespace wstux {
namespace logging {

class manager final
{
public:
    static bool cal_log(severity_level lvl) { return m_global_level >= lvl; }

    static severity_level global_level() { return m_global_level; }

    static void set_global_level(int lvl) { set_global_level((severity_level)lvl); }

    static void set_global_level(severity_level lvl);

private:
    using severity_level_t = std::atomic<severity_level>;

private:
    static severity_level_t m_global_level;
};

} // namespace logging
} // namespace wstux

#endif /* _LOGGING_WRAPPER_LOGGING_WRAPPER_LOGGINGC_MANAGER_H_ */

