# Logging wrapper
*logging_wrapper* is a common wrapper for logging systems.
The library implements a universal wrapper for a custom logger and contains a
declaration of an enumeration of logging levels, macros for logging, and a logger
manager that supports channeling.

## Contents

* [Description](#description)
  * [Severity levels](#severity levels)
  * [Manager](#manager)
  * [Logger](#logger)
  * [Macros](#macros)
* [Build](#build)
* [Usage](#usage)
  * [C logging wrapper](#c logging wrapper)
  * [CPP logging wrapper](#cpp logging wrapper)
* [License](#license)

## Description

The library provides universal wrappers for logging systems. The library contains
two wrappers - *loggingf_wrapper* for C and *logging_wrapper* for C++ projects.

Both wrappers provide:
- supported severity levels;
- manager that stores and manages the lifetime of all created loggers;
- universal logger, compressing user logger and logging information;
- set of macros for logging.

### Severity levels

The library provides a set of logging levels:
- emerg  - the system is unusable;
- fatal  - actions that must be taken care of immediately;
- crit   - critical conditions;
- error  - non-critical error conditions;
- warn   - warning conditions that should be taken care of;
- notice - normal, but significant events;
- info   - informational messages that require no action;
- debug  - debugging messages, output if the developer enabled debugging at compile time;
- trace  - the most detailed level of logging within the spectrum of log levels that developers can use.

### Manager

### Logger

### Macros

## Build

## Usage

### C logging wrapper

Usage example:
```
#include <stdio.h>

#define LOGGINGF_WRAPPER_IMPL(logger, level, fmt, ...)                         \
    char cur_ts[24];                                                           \
    lw_timestamp(cur_ts, 24);                                                  \
    logger->p_logger("%s " LOGF_LEVEL(level) " %s: " fmt "\n",          \
                         cur_ts, logger->channel __VA_OPT__(,) __VA_ARGS__)

#include "loggingf_wrapper/logging.h"

int main()
{
    if (! lw_init_logging(printf, fixed_size, 1, debug, NULL)) {
        return 1;
    }

    lw_loggerf_t root_logger = lw_get_logger("Root");
    LOGF_INFO(root_logger, "Hello, world!");

    lw_deinit_logging();
    return 0;
}
```

### CPP logging wrapper

Usage example:
```
#include <iostream>

#define LOGGING_WRAPPER_IMPL(logger, level)                                    \
    logger.get_logger() << ::wstux::logging::manager::timestamp() << " "       \
                        << LOG_LEVEL(level) << " " << logger.channel() << ": "

#include "logging_wrapper/logging.h"

struct clog_logger final
{
    template <typename T>
    inline std::ostream& operator<<(const T& val) { return std::clog << val; }
};

using logger_t = ::wstux::logging::logger<clog_logger>;

namespace wstux {
namespace logging {

template<>
clog_logger make_logger<clog_logger>(const std::string&) { return clog_logger(); }

} // namespace logging
} // namespace wstux

int main()
{
    ::wstux::logging::manager::init(::wstux::logging::severity_level::debug);

    logger_t root_logger = ::wstux::logging::manager::get_logger<logger_t>("Root");
    LOG_INFO(root_logger, "Hello, world!");

    ::wstux::logging::manager::deinit();
    return 0;
}
```

## License

&copy; 2024 Chistyakov Alexander.

Open sourced under MIT license, the terms of which can be read here — [MIT License](http://opensource.org/licenses/MIT).

