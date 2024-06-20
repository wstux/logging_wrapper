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
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "logging_wrapper/severity_level.h"

namespace wstux {
namespace logging {
namespace details {

////////////////////////////////////////////////////////////////////////////////
// struct base_logger

struct base_logger
{
    using severity_level_t = std::atomic<severity_level>;
    using ptr = std::shared_ptr<base_logger>;

    virtual ~base_logger() {}

    inline bool can_log(severity_level lvl) const { return level >= lvl; }

    const std::string channel;
    severity_level_t level;

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
    using ptr = std::shared_ptr<logger_impl>;

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

////////////////////////////////////////////////////////////////////////////////
// free functions

int timestamp(char* buf, size_t size);

std::string timestamp();

} // namespace details

class manager;

////////////////////////////////////////////////////////////////////////////////
// struct logger

/**
 *  \brief  The wrapper around abstract logger.
 *  \details    The wrapper stores any type of logger.The wrapper does not
 *              distinguish between logger types and can work with both
 *              stream-like and printf-like loggers.
 *
 *  \code
 *      logger<logger_t> logger = manager::get_logger<logger_t>("Root");
 *      LOG_INFO(logger, "message");
 *  \endcode
 */
template<typename TLogger>
struct logger final
{
    friend manager;

    using logger_impl_t = details::logger_impl<TLogger>;
    using logger_type = TLogger;

    bool can_log(severity_level lvl) const { return p_logger_impl->can_log(lvl); }

    const std::string& channel() const { return p_logger_impl->channel; }

    logger_type& get_logger() { return p_logger_impl->logger; }

    typename logger_impl_t::ptr p_logger_impl;

private:
    explicit logger(typename logger_impl_t::ptr p_logger)
        : p_logger_impl(p_logger)
    {}
};

////////////////////////////////////////////////////////////////////////////////
// class manager

/**
 *  \brief  Logger manager. Provides access to all registered loggers.
 *  \details
 *
 *  \code
 *  \endcode
 */
class manager final
{
public:
    static bool cal_log(severity_level lvl) { return m_global_level >= lvl; }

    static void clear();

    template<typename TLogger>
    static logger<TLogger> get_logger(const std::string& channel);

    static severity_level global_level() { return m_global_level; }

    static void immutable_global_level() { m_immutable_global_level = true; }

    template<typename TLogger>
    static void init(const std::function<TLogger(const std::string&)>& make_logger_fn);

    static void set_global_level(int lvl) { set_global_level((severity_level)lvl); }

    static void set_global_level(severity_level lvl);

    static void set_logger_level(const std::string& channel, severity_level lvl);

private:
    using base_logger_t = details::base_logger;
    using severity_level_t = std::atomic<severity_level>;

    struct logger_holder final
    {
        using ptr = std::shared_ptr<logger_holder>;
        using map = std::unordered_map<std::string, logger_holder::ptr>;

        explicit logger_holder(const std::string& ch, severity_level lvl)
            : channel(ch)
            , level(lvl)
        {}

        template<typename TLogger>
        std::shared_ptr<TLogger> get_logger();

        void set_level(severity_level lvl);

        const std::string channel;
        severity_level level;
        base_logger_t::ptr p_base_logger;
    };

private:
    static logger_holder::ptr register_logger(const std::string& channel, severity_level lvl = severity_level::debug);

private:
    static severity_level_t m_global_level;
    static std::atomic_bool m_immutable_global_level;

    static std::recursive_mutex m_loggers_mutex;
    static logger_holder::map m_loggers_map;
};

////////////////////////////////////////////////////////////////////////////////
// class manager::logger_holder definition

template<typename TLogger>
std::shared_ptr<TLogger> manager::logger_holder::get_logger()
{
    using logger_impl_t = TLogger;

    static_assert(std::is_base_of<base_logger_t, logger_impl_t>::value, "manager::logger_holder::get_logger: invalid TLogger type");
    static_assert(! std::is_same<base_logger_t, logger_impl_t>::value, "manager::logger_holder::get_logger: invalid TLogger type");

    std::shared_ptr<logger_impl_t> p_logger;
    if (! p_base_logger.get()) {
        p_logger = std::make_shared<logger_impl_t>(channel, level);
        p_base_logger = p_logger;
    } else {
        assert(std::dynamic_pointer_cast<logger_impl_t>(p_base_logger) && "manager::get_logger: invalid pointer cast");
        p_logger = std::static_pointer_cast<logger_impl_t>(p_base_logger);
    }

    return p_logger;
}

////////////////////////////////////////////////////////////////////////////////
// class manager definition

template<typename TLogger>
logger<TLogger> manager::get_logger(const std::string& channel)
{
    using logger_impl_t = details::logger_impl<TLogger>;

    std::lock_guard<std::recursive_mutex> lock(m_loggers_mutex);
    logger_holder::map::iterator it = m_loggers_map.find(channel);

    logger_holder::ptr p_holder;
    if (it == m_loggers_map.end()) {
        p_holder = register_logger(channel);
    } else {
        p_holder = it->second;
    }
    return logger<TLogger>(p_holder->get_logger<logger_impl_t>());
}

template<typename TLogger>
void manager::init(const std::function<TLogger(const std::string&)>& make_logger_fn)
{
    details::logger_impl<TLogger>::make_logger_fn = make_logger_fn;
}

} // namespace logging
} // namespace wstux

#endif /* _LOGGING_WRAPPER_LOGGING_WRAPPER_LOGGINGC_MANAGER_H_ */

