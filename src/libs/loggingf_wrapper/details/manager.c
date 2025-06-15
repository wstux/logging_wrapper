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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "loggingf_wrapper/manager.h"

#define _CHANNEL_LOG_LEVEL_DFL      debug
#define _TS_FILL_DFL(ts_buf, buf_size)                               \
    memcpy(ts_buf, "yyyy-MM-dd hh:mm:ss.mil", (buf_size < 24) ? buf_size : 24)

struct _lw_hash_node;
typedef struct _lw_hash_node    hash_node_t;
typedef struct lw_loggerf       _lw_loggerf_t;
typedef _lw_loggerf_t*	(*get_logger_fn_t)(const char*);

struct _lw_hash_node
{
    hash_node_t* p_next;
    _lw_loggerf_t logger;
    int channel_length;
};

struct _lw_loggingf_manager
{
    volatile sig_atomic_t global_lvl;
    hash_node_t** p_bucket;
    size_t size;
    size_t capacity;
    hash_node_t* p_pool;
    _lw_loggerf_t* p_root_logger;
    lw_loggerf_fn_t logger_fn;
    get_logger_fn_t get_logger_fn;
};

typedef struct _lw_loggingf_manager loggingf_manager_t;

static loggingf_manager_t* g_p_manager = NULL;

/*
 * The Dan Bernstein popuralized hash..  See
 * https://github.com/pjps/ndjbdns/blob/master/cdb_hash.c#L26 Due to hash
 * collisions it seems to be replaced with "siphash" in n-djbdns, see
 * https://github.com/pjps/ndjbdns/commit/16cb625eccbd68045737729792f09b4945a4b508
 */
static size_t hash_fn(const char* p_key, size_t length)
{
    size_t h = 5381;
    while (length--) {
        /* h = 33 * h ^ s[i]; */
        h += (h << 5);
        h ^= *p_key++;
    }
    return h;
}

static _lw_loggerf_t* get_logger_dynamic_size(const char* channel)
{
    assert(g_p_manager != NULL && "Logging manager is not initialized");

    int length = strlen(channel);
    if (length > (LOG_CHANNEL_LEN - 1)) {
        length = LOG_CHANNEL_LEN - 1;
    }
    size_t hash = hash_fn(channel, length);
    int i = hash % g_p_manager->capacity;

    hash_node_t** p_node;
    for (p_node = &g_p_manager->p_bucket[i]; *p_node != NULL; p_node = &(*p_node)->p_next) {
        if ((*p_node)->channel_length == length && memcmp((*p_node)->logger.channel, channel, length) == 0) {
            return &(*p_node)->logger;
        }
    }

    if (g_p_manager->size >= g_p_manager->capacity) {
        size_t capacity = g_p_manager->capacity * 2;
        hash_node_t** p_bucket = (hash_node_t**)malloc(capacity * sizeof(hash_node_t*));
        if (p_bucket == NULL) {
            return NULL;
        }
        for (size_t i = 0; i < capacity; ++i) {
            p_bucket[i] = NULL;
        }
        for (size_t i = 0; i < g_p_manager->capacity; ++i) {
            if (g_p_manager->p_bucket[i] == NULL) {
                continue;
            }
            for (hash_node_t* p_old_node = g_p_manager->p_bucket[i]; p_old_node != NULL;) {
                hash = hash_fn(p_old_node->logger.channel, p_old_node->channel_length);
                p_node = &p_bucket[hash % capacity];
                while (*p_node != NULL) {
                    p_node = &(*p_node)->p_next;
                }
                *p_node = p_old_node;
                p_old_node = p_old_node->p_next;
                (*p_node)->p_next = NULL;
            }
        }
        g_p_manager->capacity = capacity;
        free(g_p_manager->p_bucket);
        g_p_manager->p_bucket = p_bucket;
        return get_logger_dynamic_size(channel);
    }

    *p_node = (hash_node_t*)malloc(sizeof(hash_node_t));
    (*p_node)->p_next = NULL;
    ++g_p_manager->size;

    (*p_node)->logger.p_logger = g_p_manager->logger_fn;
    (*p_node)->logger.level = _CHANNEL_LOG_LEVEL_DFL;
    memcpy((*p_node)->logger.channel, channel, length);
    (*p_node)->logger.channel[length] = '\0';
    (*p_node)->channel_length = length;
    return &(*p_node)->logger;
}

static _lw_loggerf_t* get_logger_fixed_size(const char* channel)
{
    assert(g_p_manager != NULL && "Logging manager is not initialized");

    int length = strlen(channel);
    if (length > (LOG_CHANNEL_LEN - 1)) {
        length = LOG_CHANNEL_LEN - 1;
    }
    size_t hash = hash_fn(channel, length);
    int i = hash % g_p_manager->capacity;

    hash_node_t** p_node;
    for (p_node = &g_p_manager->p_bucket[i]; *p_node != NULL; p_node = &(*p_node)->p_next) {
        if ((*p_node)->channel_length == length && memcmp((*p_node)->logger.channel, channel, length) == 0) {
            return &(*p_node)->logger;
        }
    }

    if (g_p_manager->size >= g_p_manager->capacity) {
        return NULL;
    }
    if (g_p_manager->p_pool == NULL) {
        return NULL;
    }

    *p_node = &g_p_manager->p_pool[g_p_manager->size];
    (*p_node)->p_next = NULL;
    ++g_p_manager->size;

    (*p_node)->logger.level = _CHANNEL_LOG_LEVEL_DFL;
    memcpy((*p_node)->logger.channel, channel, length);
    (*p_node)->logger.channel[length] = '\0';
    (*p_node)->channel_length = length;
    return &(*p_node)->logger;
}

bool lw_can_log(int lvl)
{
    assert(g_p_manager != NULL && "Logging manager is not initialized");
    if (lvl < 0 || lvl > LVL_TRACE) {
        return false;
    }
    return g_p_manager->global_lvl >= lvl;
}

bool lw_can_channel_log(lw_loggerf_t p_logger, int lvl)
{
    if (p_logger == NULL) {
        return false;
    }
    if (lvl < 0 || lvl > LVL_TRACE) {
        return false;
    }
    return p_logger->level >= lvl;
}

lw_loggerf_t lw_get_logger(const char* channel)
{
    assert(g_p_manager != NULL && "Logging manager is not initialized");
    return g_p_manager->get_logger_fn(channel);
}

lw_severity_level_t lw_global_level(void)
{
    assert(g_p_manager != NULL && "Logging manager is not initialized");
    return g_p_manager->global_lvl;
}

bool lw_init_logging(lw_loggerf_fn_t p_logger_fn, lw_logging_policy_t policy, size_t channel_count,
                     lw_severity_level_t dfl_lvl, const char* p_root_ch)
{
    assert(g_p_manager == NULL && "Logging manager is initialized");
    assert(p_logger_fn != NULL);

    if (policy == fixed_size && channel_count == 0) {
        return false;
    }

    g_p_manager = (loggingf_manager_t*)malloc(sizeof(loggingf_manager_t));
    if (g_p_manager == NULL) {
        return false;
    }

    g_p_manager->size = 0;
    g_p_manager->capacity = channel_count;
    g_p_manager->global_lvl = dfl_lvl;
    g_p_manager->p_bucket = NULL;
    g_p_manager->p_pool = NULL;
    g_p_manager->p_root_logger = NULL;
    g_p_manager->logger_fn = p_logger_fn;
    if (policy == fixed_size) {
        g_p_manager->get_logger_fn = get_logger_fixed_size;
    } else {
        g_p_manager->capacity = channel_count = 8;
        g_p_manager->get_logger_fn = get_logger_dynamic_size;
    }

    g_p_manager->p_bucket = (hash_node_t**)malloc(channel_count * sizeof(hash_node_t*));
    if (g_p_manager->p_bucket == NULL) {
        lw_deinit_logging();
        return false;
    }
    for (size_t i = 0; i < channel_count; ++i) {
        g_p_manager->p_bucket[i] = NULL;
    }

    if (policy == fixed_size) {
        g_p_manager->p_pool = (hash_node_t*)malloc(channel_count * sizeof(hash_node_t));
        if (g_p_manager->p_pool == NULL) {
            lw_deinit_logging();
            return false;
        }
        for (size_t i = 0; i < channel_count; ++i) {
            g_p_manager->p_pool[i].logger.p_logger = p_logger_fn;
            g_p_manager->p_pool[i].p_next = NULL;
        }
    }

    if (p_root_ch != NULL) {
        g_p_manager->p_root_logger = g_p_manager->get_logger_fn(p_root_ch);
    }

    return true;
}

bool lw_deinit_logging(void)
{
    if (g_p_manager == NULL) {
        return true;
    }

    if (g_p_manager->p_bucket != NULL && g_p_manager->p_pool == NULL) {
        for (size_t i = 0; i < g_p_manager->capacity; ++i) {
            hash_node_t* p_node = g_p_manager->p_bucket[i];
            while (p_node != NULL) {
                hash_node_t* p_del_node = p_node;
                p_node = p_node->p_next;
                free(p_del_node);
            }
        }
    }

    free(g_p_manager->p_pool);
    free(g_p_manager->p_bucket);
    free(g_p_manager);
    g_p_manager = NULL;
    return true;
}

lw_loggerf_t lw_root_logger(void)
{
    assert(g_p_manager != NULL && "Logging manager is not initialized");
    return g_p_manager->p_root_logger;
}

void lw_set_global_level(lw_severity_level_t lvl)
{
    assert(g_p_manager != NULL && "Logging manager is not initialized");
    g_p_manager->global_lvl = lvl;
}

void lw_set_logger_level(const char* channel, lw_severity_level_t lvl)
{
    assert(g_p_manager != NULL && "Logging, lw_logging_policy_t policy manager is not initialized");

    _lw_loggerf_t* p_logger = g_p_manager->get_logger_fn(channel);
    if (p_logger != NULL) {
        p_logger->level = lvl;
    }
}

int lw_timestamp(char* buf, size_t size)
{
    struct timeval cur_tv;
    struct tm cur_tm;

    if (gettimeofday(&cur_tv, NULL) != 0) {
        _TS_FILL_DFL(buf, size);
        return -1;
    }
    if (localtime_r(&cur_tv.tv_sec, &cur_tm) == NULL) {
        _TS_FILL_DFL(buf, size);
        return -1;
    }

    int rc = snprintf(buf, size, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                cur_tm.tm_year + 1900, cur_tm.tm_mon + 1, cur_tm.tm_mday,
                cur_tm.tm_hour, cur_tm.tm_min, cur_tm.tm_sec, (int)(cur_tv.tv_usec / 1000));
    if (rc < 0) {
        _TS_FILL_DFL(buf, size);
        return -1;
    }
    buf[rc] = '\0';
    return 0;
}

#undef _CHANNEL_LOG_LEVEL_DFL
#undef _TS_FILL_DFL

