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
#include <memory>
#include <string>

#include "logging_wrapper/severity_level.h"

namespace wstux {
namespace logging {
namespace details {

////////////////////////////////////////////////////////////////////////////////
// struct base_logger

struct base_logger
{
    using ptr = std::shared_ptr<base_logger>;

    virtual ~base_logger() {}

    inline bool can_log(severity_level lvl) const { return level >= lvl; }

    const std::string channel;
    severity_level level;

protected:
    base_logger(const std::string& ch, const severity_level lvl)
        : channel(ch)
        , level(lvl)
    {}

private:
    base_logger(const base_logger&);
    base_logger& operator=(const base_logger&);
};

////////////////////////////////////////////////////////////////////////////////
// struct logger_impl

template<typename TLogger>
struct logger_impl final : public base_logger
{
    using base = base_logger;
    using logger_type = TLogger;
    using make_logger_fn_t = std::function<TLogger(const std::string&)>;
    using ptr = std::shared_ptr<base_logger>;

    logger_impl(const std::string& channel, severity_level lvl)
        : base(channel, lvl)
        , logger(make_logger_fn(channel))
    {}

    virtual ~logger_impl() {}

    logger_type logger;

    static make_logger_fn_t make_logger_fn;
};

template<typename TLogger>
typename logger_impl<TLogger>::make_logger_fn_t logger_impl<TLogger>::make_logger_fn = [] (const std::string& c) -> TLogger { return TLogger(c); };

} // namespace details

////////////////////////////////////////////////////////////////////////////////
// struct logger

template<typename TLogger>
struct logger final
{
    using logger_impl_ptr_t = typename details::logger_impl<TLogger>::ptr;

    explicit logger(logger_impl_ptr_t logger)
        : logger_impl(logger)
    {}

    logger_impl_ptr_t logger_impl;
};

////////////////////////////////////////////////////////////////////////////////
// class manager

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

