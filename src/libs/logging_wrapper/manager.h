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
/**
 *  \file   manager.h
 *  \brief  Logging manager for C++ with support for channels and severity levels.
 *  \ingroup logging_wrapper_module
 */

#ifndef _LIBS_LOGGING_WRAPPER_MANAGER_H_
#define _LIBS_LOGGING_WRAPPER_MANAGER_H_

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

/**
 *  \brief  Factory function for creating logger objects.
 *  \tparam TLogger - custom logger type to be initialized.
 *  \param  ch - name of the logging channel to which the created logger is bound.
 *  \return The constructed logger object bound to the specified channel.
 *
 *  \details    This function serves as the primary extension point for the
 *      library. By default, it does not provide a generic implementation and
 *      strictly requires an explicit specialization for each logger type used.
 *
 *  \attention  To integrate a custom logger into the wrapper, needs to implement
 *      a full specialization of this template inside the `wstux::logging` namespace.
 *
 *  \note   **TLogger Interface Requirements:**
 *  - **For C-style logging (`LOGF_*`):** The class or structure must either
 *      overload the call operator (`operator()`). This function/operator must
 *      accept a format string and a variadic list of arguments (`...`).
 *  - **For CPP-style logging (`LOG_*`):** The class must return a stream object
 *      (e.g., `std::ostream&`) via a method or an overloaded operator to support
 *      method-chaining using the `<<` operator.
 *
 *  \warning The factory implementation must guarantee thread safety if multiple
 *      threads can simultaneously initialize loggers for the same channel.
 *
 *  Example 1: Implementation for a stream-based logger (CPP-style):
 *  \code
 *  struct clog_logger final
 *  {
 *      template<typename T> std::ostream& operator<<(const T& val) { return std::clog << val; }
 *  };
 *
 *  namespace wstux { namespace logging {
 *  template<> clog_logger make_logger<clog_logger>(const std::string&) { return clog_logger(); }
 *  }}
 *  \endcode
 *
 *  Example 2: Implementation for a formatted logger (C-style):
 *  \code
 *  struct printf_logger final
 *  {
 *      int operator()(const char* p_fmt, ...)
 *      {
 *          va_list args;
 *          va_start(args, p_fmt);
 *          const int rc = vprintf(p_fmt, args);
 *          va_end(args);
 *          return rc;
 *      }
 *  };
 *
 *  namespace wstux { namespace logging {
 *  template<> printf_logger make_logger<printf_logger>(const std::string&) { return printf_logger(); }
 *  }}
 *  \endcode
 */
template<typename TLogger>
TLogger make_logger(const std::string& ch);

} // namespace logging
} // namespace wstux

namespace wstux {
namespace logging {
namespace details {

////////////////////////////////////////////////////////////////////////////////
/// \struct base_logger

/**
 *  \brief  Abstract base class for managing the metadata of an individual
 *      logging channel.
 *
 *  \details Stores the channel name and its current log filtering level. The
 *      logging level is thread-safe (std::atomic), allowing it to be changed
 *      dynamically at runtime.
 */
struct base_logger
{
    using severity_level_t = std::atomic<severity_level>; ///< Data type for atomic storage of the logging level.
    using ptr = std::shared_ptr<base_logger>;             ///< Smart pointer to the base logger.

    /// \brief  Destructor.
    virtual ~base_logger() {}

    /// \brief  Logging level check function.
    /// \param  lvl - required severity level.
    /// \return True, if the required severity level corresponds to the requested.
    ///     Otherwise, false.

    /// \brief  Checks if the specified logging level is enabled for the current channel.
    /// \param  lvl - required severity level for recording.
    /// \return True, if the message level is less than or equal to the channel's
    ///     set level (recording is permitted). Otherwise, false - the message
    ///     should be filtered out and ignored.
    inline bool can_log(severity_level lvl) const { return level >= lvl; }

    const std::string channel;  ///< Channel name.
    severity_level_t level;     ///< Severity level for this channel.

protected:
    /// \brief  Protected constructor for invocation by derived classes.
    /// \param  ch - name of the logging channel.
    /// /param  lvl - initial severity level for the channel.
    base_logger(const std::string& ch, const severity_level lvl)
        : channel(ch)
        , level(lvl)
    {}

private:
    // Copy and assignment are deleted (Copy Semantics disabled)
    base_logger(const base_logger&);
    base_logger& operator=(const base_logger&);
};

////////////////////////////////////////////////////////////////////////////////
/// \struct logger_impl

/**
 *  \brief  Wrapper around a specific custom logger implementation.
 *  \tparam TLogger - custom logger type.
 *
 *  \details    Implements the Concrete Strategy pattern, encapsulating any
 *      third-party logger object within the polymorphic base_logger interface.
 */
template<typename TLogger>
struct logger_impl final : public base_logger
{
    using logger_type = TLogger;              ///< Type alias for the encapsulated custom logger.
    using ptr = std::shared_ptr<logger_impl>; ///< Smart pointer to the specific wrapper implementation.

    /// \brief  Constructor for the logger implementation.
    /// \param  channel - name of the logging channel.
    /// \param  lvl - initial severity level for the channel.
    /// \details Initializes the base parameters and automatically creates the
    ///     custom logger object using the specialized factory function
    ///     `make_logger<TLogger>`.
    logger_impl(const std::string& channel, severity_level lvl)
        : base_logger(channel, lvl)
        , logger(make_logger<logger_type>(channel))
    {}

    /// \brief  Destructor.
    virtual ~logger_impl() {}

    logger_type logger; ///< Instance of the actual custom logger.
};

} // namespace details

////////////////////////////////////////////////////////////////////////////////
/// \struct logger

/**
 *  \brief  Lightweight external interface for interacting with the logger.
 *  \tparam TLogger - custom logger type wrapped by this interface.
 *
 *  \details    This class is designed as a cheap-to-copy descriptor. It conceals
 *      a polymorphic pointer to `logger_impl`, effectively erasing the underlying
 *      logger type differences. It allows uniform handling of both stream-based
 *      (CPP-style) and printf-like (C-style) loggers.
 *
 *  \attention  For this class to operate correctly, an explicit specialization
 *      of the `make_logger<TLogger>` function must be declared for the `TLogger`
 *      type.
 *
 *  Usage example:
 *  \code
 *  // 1. Define custom logger
 *  struct clog_logger final
 *  {
 *      template<typename T>
 *      inline std::ostream& operator<<(const T& val) { return std::clog << val; }
 *  };
 *
 *  // 2. Specialize the factory function within the correct namespace
 *  namespace wstux { namespace logging {
 *
 *  template<>
 *  clog_logger make_logger<clog_logger>(const std::string&) { return clog_logger(); }
 *
 *  }}
 *
 *  // 3. Utilize the logger interface in application code
 *  void do_work()
 *  {
 *      using logger_t = ::wstux::logging::logger<clog_logger>;
 *
 *      logger_t logger = ::wstux::logging::manager::get_logger<logger_t>("Root");
 *
 *      // Write to the log using the appropriate macro
 *      LOG_INFO(logger, "System started successfully. Step " << 1);
 *  }
 *  \endcode
 */
template<typename TLogger>
struct logger final
{
    using logger_impl_t = details::logger_impl<TLogger>; ///< Internal type of the concrete implementation for this template.
    using logger_type = TLogger;                         ///< Custom type of the underlying logger.

    /// \brief  Checks if the specified logging level is enabled for this logger.
    /// \param  lvl - required severity level.
    /// \return true if recording is permitted, false otherwise.
    bool can_log(severity_level lvl) const { return p_logger_impl->can_log(lvl); }

    /// \brief  Retrieves the channel name of the current logger.
    /// \return Reference to a constant string containing the channel name.
    const std::string& channel() const { return p_logger_impl->channel; }

    /// \brief  Provides direct access to the custom logger object.
    /// \return Reference to the instance of the custom `TLogger` class.
    /// \details    Utilized by internal logging macros (`_LOG` / `_LOGF`) to
    ///     invoke `operator()` or `operator<<`.
    logger_type& get_logger() { return p_logger_impl->logger; }

    typename logger_impl_t::ptr p_logger_impl; ///< Shared pointer to the implementation object containing metadata.

    /// \brief  Constructor for the logger interface.
    /// \param  p_logger - smart pointer to the corresponding logger implementation.
    /// \attention  The struct can only be created via ::wstux::logging::manager.
    explicit logger(typename logger_impl_t::ptr p_logger)
        : p_logger_impl(p_logger)
    {}
};

////////////////////////////////////////////////////////////////////////////////
/// \class manager

/**
 *  \brief  Global logging manager. Provides centralized access and control for all channels.
 *
 *  \details    This class functions as a static singleton (Registry pattern).
 *      It manages the lifecycle of registered loggers, handles thread-safe
 *      severity level filtering, and provides timestamp generation utilities.
 *
 *  Usage example:
 *  \code
 *  using logger_t = ::wstux::logging::logger<clog_logger>;
 *
 *  // 1. Initialize the manager
 *  ::wstux::logging::manager::init(::wstux::logging::severity_level::debug);
 *
 *  // 2. Retrieve or create a logger for a named channel
 *  logger_t root_logger = ::wstux::logging::manager::get_logger<logger_t>("Root");
 *  ...
 *  // 3. Deinitialize the manager and release resources
 *  ::wstux::logging::manager::deinit();
 *  \endcode
 */
class manager final
{
public:
    using init_fn_t = std::function<void()>; ///< Callback function type for custom initialization of logger subsystems.

    /// \brief  Fast-path check: determines if logging is enabled globally for
    ///     the specified severity level.
    /// \param  lvl - required severity level to verify.
    /// \return true, if the global level permits processing this message, false
    ///     otherwise, if the message should be discarded without performing
    ///     granular channel checks.
    static bool cal_log(severity_level lvl) { return m_global_level >= lvl; }

    /// \brief  Deinitialization of the log manager.
    /// \details    Clears the internal map of registered loggers, resetting all
    ///     held `shared_ptr` smart pointers.
    static void deinit();

    /// \brief  Retrieves or creates a logger for the specified channel.
    /// \tparam TLogger - the external handle class of the logger (`wstux::logging::logger<T>`).
    /// \param  channel - unique name of the logging channel.
    /// \return A logger descriptor object ready for use.
    /// \details    If a channel with the given name already exists, the associated
    ///     logger is returned. If the channel does not exist, it is registered
    ///     with the default level `severity_level::debug`.
    template<typename TLogger>
    static TLogger get_logger(const std::string& channel);

    /// \brief  Retrieves a logger while forcing a setting of the channel's default level.
    /// \tparam TLogger - the external handle class of the logger (`wstux::logging::logger<T>`).
    /// \param  channel - unique name of the logging channel.
    /// \param  lvl - severity level to be forcibly applied to this channel.
    /// \return A logger descriptor object.
    /// \details    Before returning the logger, this method triggers an update
    ///     of the severity level for this specific channel.
    template<typename TLogger>
    static TLogger get_logger_dfl(const std::string& channel, severity_level lvl);

    /// \brief  Retrieves the current global logging level.
    /// \return The current value of the `severity_level` enumeration.
    static severity_level global_level() { return m_global_level; }

    /// \brief  Initializes the logging manager subsystem.
    /// \param  global_lvl - initial global filtering level (defaults to `warning`).
    /// \param  init_fn - optional custom callback functor for lazy logging configuration.
    static void init(severity_level global_lvl = severity_level::warning, init_fn_t init_fn = []() -> void {});

    /// \brief  Changes the global logging level using an integer value (int).
    /// \param  lvl - integer representation of the level. Automatically cast to the `severity_level` type.
    static void set_global_level(int lvl) { set_global_level((severity_level)lvl); }

    /// \brief  Sets a new global logging level.
    /// \param  lvl - new global severity level.
    /// \details    If the level has been marked as immutable, the invocation
    ///     will be ignored.
    static void set_global_level(severity_level lvl);

    /// \brief  Sets the global logging level and locks it from subsequent modifications.
    /// \param  lvl - fixed global severity level.
    /// \details    Sets the immutable flag to `true`. Useful for production builds.
    static void set_immutable_global_level(severity_level lvl);

    /// \brief  Sets or dynamically modifies the logging level for a specific channel.
    /// \param  channel - name of the target channel.
    /// \param  lvl - new severity level for this channel.
    static void set_logger_level(const std::string& channel, severity_level lvl);

    /// \brief  Writes the current high-resolution time into a raw C-string buffer.
    /// \param  buf - pointer to the character array where the date/time will be written.
    /// \param  size - size limit of the buffer (at least 24 bytes recommended).
    /// \return Number of characters successfully written.
    static int timestamp(char* buf, size_t size);

    /// \brief  Generates a string containing the current time.
    /// \return Formatted string in the `YYYY-MM-DD HH:MM:SS.mmm` format.
    static std::string timestamp();

private:
    using base_logger_t = details::base_logger;           ///< Type alias for the base polymorphic channel metadata class.
    using severity_level_t = std::atomic<severity_level>; ///< Atomic variable type for log levels.

    /**
     *  \brief  Internal manager container for storing a specific channel's
     *      metadata and implementation.
     *
     *  \details    Allows holding a polymorphic pointer to the implementation
     *      while the user code operates with generalized interfaces.
     */
    struct logger_holder final
    {
        using ptr = std::shared_ptr<logger_holder>;                      ///< Smart pointer to the container.
        using map = std::unordered_map<std::string, logger_holder::ptr>; ///< Hash map type for storing the channel registry.

        /// \brief  Constructor for the logger channel container.
        /// \param  ch - name of the channel.
        /// \param  lvl - initial logging level of the channel.
        explicit logger_holder(const std::string& ch, severity_level lvl)
            : channel(ch)
            , level(lvl)
        {}

        /// \brief  Lazy creation or retrieval of the underlying polymorphic
        ///     logger implementation.
        /// \tparam TLogger - internal target implementation class (`details::logger_impl<T>`).
        /// \return Typed pointer to the implementation object.
        template<typename TLogger>
        std::shared_ptr<TLogger> get_logger();

        /// \brief  Modifies the logging level for the current channel holder.
        /// \param  lvl - new severity level.
        void set_level(severity_level lvl);

        const std::string channel;        ///< Name of the logging channel.
        severity_level level;             ///< Current severity level of the channel.
        base_logger_t::ptr p_base_logger; ///< Polymorphic pointer to the base log channel metadata.
    };

private:
    /// \brief  Internal registration of a new channel within the manager's registry.
    /// \param  channel - name of the channel.
    /// \param  lvl - initial severity level.
    /// \return The created and registered channel container.
    static logger_holder::ptr register_logger(const std::string& channel, severity_level lvl);

private:
    static severity_level_t m_global_level;      ///< Global atomic filtering level for the entire system.
    static std::atomic_bool m_is_immutable;      ///< Atomic flag locking the global level from modifications.

    static std::recursive_mutex m_loggers_mutex; ///< Recursive mutex protecting the thread safety of the `m_loggers_map` registry.
    static logger_holder::map m_loggers_map;     ///< Central hash registry of all registered log channels.
};

////////////////////////////////////////////////////////////////////////////////
// class manager::logger_holder definition

template<typename TLogger>
std::shared_ptr<TLogger> manager::logger_holder::get_logger()
{
    using logger_impl_t = TLogger;

    // Static assertions to ensure compile-time type safety
    static_assert(std::is_base_of<base_logger_t, logger_impl_t>::value, "manager::logger_holder::get_logger: invalid TLogger type");
    static_assert(! std::is_same<base_logger_t, logger_impl_t>::value, "manager::logger_holder::get_logger: invalid TLogger type");

    std::shared_ptr<logger_impl_t> p_logger;
    if (! p_base_logger.get()) {
        // Lazy memory allocation for the specific implementation upon first access
        p_logger = std::make_shared<logger_impl_t>(channel, level);
        p_base_logger = p_logger;
    } else {
        // \todo ARCHITECTURAL RISK: If the same string channel is requested with different logger types
        // (e.g., C-style first, then CPP-style), `dynamic_pointer_cast` will return nullptr,
        // and the `static_pointer_cast` below will result in runtime Undefined Behavior on non-debug builds.
        // It is recommended to add a release-mode check and throw a `std::bad_cast` exception.
        assert(std::dynamic_pointer_cast<logger_impl_t>(p_base_logger) && "manager::get_logger: invalid pointer cast");
        p_logger = std::static_pointer_cast<logger_impl_t>(p_base_logger);
    }

    return p_logger;
}

////////////////////////////////////////////////////////////////////////////////
// class manager definition

template<typename TLogger>
TLogger manager::get_logger(const std::string& channel)
{
    // Extract the underlying types from the logger wrapper's handle class
    using logger_type_t = typename TLogger::logger_type;
    using logger_impl_t = details::logger_impl<logger_type_t>;

    // Protect multithreaded access to the loggers hash map
    std::lock_guard<std::recursive_mutex> lock(m_loggers_mutex);
    logger_holder::map::iterator it = m_loggers_map.find(channel);

    logger_holder::ptr p_holder;
    if (it == m_loggers_map.end()) {
        // Channel not found - registering from scratch
        p_holder = register_logger(channel, severity_level::debug);
    } else {
        // Channel found - retrieving the pointer to its container
        p_holder = it->second;
    }
    // Construct and return a cheap logger handle object
    return TLogger(p_holder->get_logger<logger_impl_t>());
}

template<typename TLogger>
TLogger manager::get_logger_dfl(const std::string& channel, severity_level lvl)
{
    // \todo PERFORMANCE BOTTLENECK: set_logger_level internally acquires the
    // same m_loggers_mutex. Then get_logger is invoked, which acquires this
    // recursive mutex once again. While a recursive mutex permits this, double
    // acquisition on every "get_logger_dfl" call is redundant. Consider
    // refactoring the lookup/creation logic into an internal lock-free private
    // method, while public methods handle the mutex acquisition exactly once.
    set_logger_level(channel, lvl);
    return get_logger<TLogger>(channel);
}

} // namespace logging
} // namespace wstux

#endif /* _LIBS_LOGGING_WRAPPER_MANAGER_H_ */
