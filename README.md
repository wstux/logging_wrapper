# Logging wrapper
*logging_wrapper* is a common wrapper for logging systems.
A high-performance, thread-safe, and lightweight logging framework for C and C++
projects. It supports log separation by named channels, flexible filtering by
severity levels (both globally and per individual channel), and is optimized for
high-load, multi-threaded environments.
The library provides a universal wrapper for a custom logger and contains a
declaration of an enumeration of logging levels, macros for logging, and a logger
manager that supports channeling.

## Contents

* [Description](#description)
  * [Severity levels](#severity_levels)
  * [Manager](#manager)
  * [Logger](#logger)
  * [Macros](#macros)
* [Build](#build)
* [Usage](#usage)
  * [C logging wrapper](#c_logging_wrapper)
  * [CPP logging wrapper](#cpp_logging_wrapper)
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

The manager is the central control core of the logging subsystem. It acts as a
coordinator and dispatcher: rather than being tied to a specific output device
(such as a console, file, or network), it manages the filtering logic, lifecycle,
and configuration of independent channels (components).

The manager provides:
* **Channel Registry:** a centralized database storing all created loggers. Each
    unique text name (e.g., `"AUTH"`, `"DB"`, `"NET"`) maps to its own `struct lw_loggerf`
    instance with its own individual severity level.
* **Global Filter (Global Threshold):** the system-wide filter. If the global
    level is set to `LVL_ERROR`, then `INFO` or `DEBUG` messages will be blocked
    at the macro level for *all* channels, even if a specific channel's individual
    level is configured as `LVL_TRACE`.

The manager is designed with multi-threaded environments in mind:
*   The logging level of each channel and the global level are stored in atomic variables.
*   Retrieving and adding loggers are protected by a mutex.

Key features:
*   **Early Exit Mechanism:** Macros verify logging levels before executing heavy
     formatting functions. If logging is disabled, the formatting arguments are
     not even evaluated.
*   **Channel-based logging:** Separation of logs by independent system modules/components,
     allowing fine-grained configuration for each channel.
*   **Implementation Isolation:** Complete encapsulation of the specific log
     output backend behind a polymorphic interface.

### C++ manager

The `::wstux::logging::manager` class is designed as a final static component
(singleton) that centrally manages the entire logging configuration within the
application. It encapsulates the global state and provides thread-safe access to
named channels.

### C manager

The C manager is the central control core of the logging subsystem. It acts as a
coordinator and dispatcher: rather than being tied to a specific output device
(such as a console, file, or network), it manages the filtering logic, lifecycle,
and configuration of independent channels (components).

During manager initialization, a memory allocation strategy for storing loggers
is specified, which is critical for high-load and resource-constrained software.

Memory management modes (Policies):
*   **`fixed_size` policy (Fixed pool):**
    *   The manager allocates a single array for channels strictly at application
     startup (the size is specified via the channel_count parameter).
    *   Zero dynamic allocation at runtime. This completely eliminates heap
     fragmentation and Out-Of-Memory (`OOM`) errors during application execution.
*   **`dynamic_size` policy (Dynamic Pool):**
    *   The channel array dynamically expands as new loggers are created via `lw_get_logger`.
    *   There is no need to know the exact number of system modules in advance.

### Logger

The logger is a wrapper structure that implements the **Facade** pattern over a
specific logging backend implementation.

This wrapper structure encapsulates the internal state of the logger and provides
a unified, lightweight interface accessed by the logging macros.

Key design features:
* **Lightweight Wrapper:** The `logger` object itself is highly efficient and
   introduces very low overhead.
* **Separation of interface and implementation (Pimpl-like):** By using a wrapper,
   the channel management logic is strictly decoupled from the actual backend
   implementation.

### Macros

Top-level macros serve as the primary entry point for writing messages into the
system. They are designed to completely eliminate overhead from evaluating log
arguments (such as function calls, string concatenation, or formatting) if a
message is filtered out due to its severity level.

#### Operating principle and Fast-Path filtering

The macros expand into a guarded `do { ... } while (0)` block. Before passing any
data to the backend, the macro executes a **two-level, short-circuit validation check**:
1. **Global Filter:** The global logging level threshold is checked first.
2. **Local Filter:** The individual logger's specific logging level threshold is
    verified next.

If either check returns `false`, execution of the block terminates instantly,
completely preventing the generation of formatted log output.

The library's base macro expands into the following isolated block:
```cpp
#define _LOG(logger, level, VARS)                                           \
    do {                                                                    \
        if (! ::wstux::logging::manager::cal_log(SEVERITY_LEVEL(level)) ||  \
            ! logger.can_log(SEVERITY_LEVEL(level))) {                      \
            break;                                                          \
        }                                                                   \
        _LOGGING_WRAPPER_IMPL(logger, level) << VARS << std::endl;          \
    }                                                                       \
    while (0)
```
* **`VARS`** — represents any sequence of arguments separated by the `<<` operator.
   It is passed to the internal temporary stream/wrapper `_LOGGING_WRAPPER_IMPL`.
* **Resource safety:** The line `LOG_DEBUG(log, "User: " << fetch_user_name_from_db())`
   is completely safe. If the `DEBUG` level is disabled, the expensive
   `fetch_user_name_from_db()` **method will never be executed**.

```cpp
#define _LOGF(logger, level, fmt, ...)                                      \
    do {                                                                    \
        if (! lw_can_log(level) || ! lw_can_channel_log(logger, level)) {   \
            break;                                                          \
        }                                                                   \
        _LOGGINGF_WRAPPER_IMPL(logger, level, fmt, __VA_ARGS__);            \
    }                                                                       \
    while (0)
```
* **`fmt` and `__VA_ARGS__`** — standard C syntax for passing a format string and
   a variadic argument list (`printf`-style). They are forwarded to the internal
   `_LOGGINGF_WRAPPER_IMPL` implementation only after successfully passing
   through the filters.

#### Public macros

For ease of use, the library provides ready-made aliases for each severity level.

##### Stream macros (`std::ostream` style)

Syntax: `LOG_<LEVEL>(logger, VARS)`

* `LOG_EMERG(logger, VARS)` - System is unusable (Emergency).
* `LOG_FATAL(logger, VARS)` - Critical error leading to application termination (Fatal).
* `LOG_CRIT(logger, VARS)` - Critical condition in business logic (Critical).
* `LOG_ERROR(logger, VARS)` - Runtime error requiring attention (Error).
* `LOG_WARN(logger, VARS)` - Warning about a potential issue (Warning).
* `LOG_NOTICE(logger, VARS)` - Important normal operational event (Notice).
* `LOG_INFO(logger, VARS)` - Informational message about system operation (Info).
* `LOG_DEBUG(logger, VARS)` - Debugging information for developers (Debug).
* `LOG_TRACE(logger, VARS)` - Maximum detail of execution steps/data dumps (Trace).

##### Formatting macros (`printf` style)

Syntax: `LOGF_<LEVEL>(logger, fmt, ...)`

* `LOGF_EMERG(logger, fmt, ...)` - System is unusable (Emergency).
* `LOGF_FATAL(logger, fmt, ...)` - Critical error leading to application termination (Fatal).
* `LOGF_CRIT(logger, fmt, ...)` - Critical condition in business logic (Critical).
* `LOGF_ERROR(logger, fmt, ...)` - Runtime error requiring attention (Error).
* `LOGF_WARN(logger, fmt, ...)` - Warning about a potential issue (Warning).
* `LOGF_NOTICE(logger, fmt, ...)` - Important normal operational event (Notice).
* `LOGF_INFO(logger, fmt, ...)` - Informational message about system operation (Info).
* `LOGF_DEBUG(logger, fmt, ...)` - Debugging information for developers (Debug).
* `LOGF_TRACE(logger, fmt, ...)` - Maximum detail of execution steps/data dumps (Trace).

#### Critical rules for safe usage

Because the macros evaluate arguments **strictly lazily** (only after passing
through the filtering stage), failing to adhere to the rules of the C preprocessor
can introduce elusive bugs (Heisenbugs).

1. **No Logic Inside Log Statements:** Log parameters must be side-effect free ("pure").
   ```c
   // CRITICAL ERROR: The counter increments ONLY when TRACE level logging is enabled!
   LOGF_TRACE(logger, "Processed item number %d", ++global_counter);
   ```

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

