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

#include "logging_wrapper/manager.h"

#define TS_FILL_DFL(ts_buf, buf_size)                               \
    memcpy(ts_buf, "yyyy-MM-dd hh:mm:ss.mil", (buf_size < 24) ? buf_size : 24)

namespace wstux {
namespace logging {

manager::severity_level_t manager::m_global_level = {severity_level::info};
std::recursive_mutex manager::m_loggers_mutex = {};
manager::logger_holder::map manager::m_loggers_map = {};

////////////////////////////////////////////////////////////////////////////////
// class manager::logger_holder definition

void manager::logger_holder::set_level(severity_level lvl)
{
    level = lvl;
    if (p_base_logger) {
        p_base_logger->level = level;
    }
}

////////////////////////////////////////////////////////////////////////////////
// class manager definition

void manager::deinit()
{
    std::lock_guard<std::recursive_mutex> lock(m_loggers_mutex);
    m_loggers_map.erase(m_loggers_map.begin(), m_loggers_map.end());
}

void manager::init(severity_level global_lvl)
{
    set_global_level(global_lvl);
}

manager::logger_holder::ptr manager::register_logger(const std::string& channel, severity_level lvl)
{
    using return_type = std::pair<logger_holder::map::iterator, bool>;

    logger_holder::ptr ptr = std::make_shared<logger_holder>(channel, lvl);
    const return_type rc = m_loggers_map.emplace(ptr->channel, ptr);
    return rc.first->second;
}

void manager::set_global_level(severity_level lvl)
{
    if ((lvl < severity_level::emerg) || (lvl > severity_level::trace)) {
        return;
    }
    m_global_level = lvl;
}

void manager::set_logger_level(const std::string& channel, severity_level lvl)
{
    std::lock_guard<std::recursive_mutex> lock(m_loggers_mutex);
    logger_holder::map::iterator it = m_loggers_map.find(channel);
    if (it == m_loggers_map.end()) {
        register_logger(channel, lvl);
    } else {
        it->second->set_level(lvl);
    }
}

int manager::timestamp(char* buf, size_t size)
{
    struct timeval cur_tv;
    struct tm cur_tm;

    if (gettimeofday(&cur_tv, NULL) != 0) {
        TS_FILL_DFL(buf, size);
        return -1;
    }
    if (localtime_r(&cur_tv.tv_sec, &cur_tm) == NULL) {
        TS_FILL_DFL(buf, size);
        return -1;
    }

    int rc = snprintf(buf, size, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                cur_tm.tm_year + 1900, cur_tm.tm_mon + 1, cur_tm.tm_mday,
                cur_tm.tm_hour, cur_tm.tm_min, cur_tm.tm_sec, (int)(cur_tv.tv_usec / 1000));
    if (rc < 0) {
        TS_FILL_DFL(buf, size);
        return -1;
    }
    buf[rc] = '\0';
    return 0;
}

std::string manager::timestamp()
{
    constexpr size_t ts_size = 24;
    char cur_ts[ts_size];
    timestamp(cur_ts, ts_size);

	return std::string(cur_ts, ts_size - 1);
}

} // namespace logging
} // namespace wstux

#undef TS_FILL_DFL

