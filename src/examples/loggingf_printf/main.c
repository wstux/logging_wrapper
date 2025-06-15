/*
 * logging_wrapper
 * Copyright (C) 2025  Chistyakov Alexander
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
