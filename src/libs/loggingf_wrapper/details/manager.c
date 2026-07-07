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
 *  \file
 *  \ingroup loggingf_wrapper_module
 */

#include <sys/time.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "loggingf_wrapper/manager.h"

/**
 *  \def    _TS_FILL_DFL(ts_buf, buf_size)
 *  \brief  Fills the buffer with a default timestamp template in case of a
 *      generation error.
 *  \param  ts_buf - buffer to write to.
 *  \param  buf_size - available buffer size.
 */
#define _TS_FILL_DFL(ts_buf, buf_size)                               \
    memcpy(ts_buf, "yyyy-MM-dd hh:mm:ss.mil", (buf_size < 24) ? buf_size : 24)

/*******************************************************************************
 * Private functions & Data Structures
 ******************************************************************************/

struct _lw_hash_node;
/** \brief  Alias for the internal hash table node structure. */
typedef struct _lw_hash_node    hash_node_t;
/** \brief  Internal alias for the logger structure. */
typedef struct lw_loggerf       _lw_loggerf_t;
/** \brief  Prototype of the internal function to retrieve a logger. */
typedef _lw_loggerf_t*	(*get_logger_fn_t)(const char*);

/**
 *  \brief  Hash table node containing a channel logger instance.
 */
struct _lw_hash_node
{
    hash_node_t* p_next;  /**< Pointer to the next node in case of a collision. */
    _lw_loggerf_t logger; /**< Logger structure (channel, level, output function). */
    int channel_length;   /**< Real length of the channel name (comparison optimization). */
};

/**
 *  \brief  Global management context of the entire logging system.
 */
struct _lw_loggingf_manager
{
    volatile sig_atomic_t global_lvl;   /**< Global logging level. */
    volatile sig_atomic_t is_immutable; /**< Flag preventing changes to the global level. */
    pthread_rwlock_t bucket_mutex;      /**< Read-write lock protecting hash table modifications and resizing. */
    hash_node_t** p_bucket;             /**< Array of hash table buckets. */
    size_t size;                        /**< Current number of registered channels. */
    size_t capacity;                    /**< Current hash table capacity (number of buckets). */
    hash_node_t* p_pool;                /**< Static pool of nodes (used with fixed_size policy). */
    _lw_loggerf_t* p_root_logger;       /**< Pointer to the root logger. */
    lw_loggerf_fn_t logger_fn;          /**< Function for log output. */
    get_logger_fn_t get_logger_fn;      /**< Pointer to the channel search/creation function being used. */
};

typedef struct _lw_loggingf_manager loggingf_manager_t;

/** \brief  Global pointer to the single instance of the logging manager. */
static loggingf_manager_t* g_p_manager = NULL;

/**
 *  \details    The Dan Bernstein popuralized hash..  See
 *  https://github.com/pjps/ndjbdns/blob/master/cdb_hash.c#L26 Due to hash
 *  collisions it seems to be replaced with "siphash" in n-djbdns, see
 *
 */
/**
 *  \brief  DJB2 hash function (Dan Bernstein).
 *  \param  p_key - the key string (channel name).
 *  \param  length - length of the key string.
 *  \return The calculated hash value of type size_t.
 *
 *  \details    Used to distribute channel names across the hash table buckets.
 *
 *  \see    https://github.com/pjps/ndjbdns/blob/master/cdb_hash.c#L26
 *  \todo   If channel names can contain user-controlled input, consider
 *      replacing this with SipHash to prevent Hash DoS attacks
 *      (https://github.com/pjps/ndjbdns/commit/16cb625eccbd68045737729792f09b4945a4b508).
 */
static size_t _hash_fn(const char* p_key, size_t length)
{
    size_t h = 5381;
    while (length--) {
        /* h = 33 * h ^ s[i]; */
        h += (h << 5);
        h ^= *p_key++;
    }
    return h;
}

/**
 *  \brief  Retrieves an existing logger channel or dynamically creates a new one.
 *  \param  channel - the name of the requested channel.
 *  \return Pointer to the internal logger structure, or NULL upon memory
 *      allocation error.
 *
 *  \details    If the capacity limit is reached (`size >= capacity`), it
 *      automatically doubles the size of the hash table and performs a rehash.
 */
static _lw_loggerf_t* _get_logger_dynamic_size(const char* channel)
{
    assert(g_p_manager != NULL && "Logging manager is not initialized");

    int length = strlen(channel);
    if (length > (LOG_CHANNEL_LEN - 1)) {
        length = LOG_CHANNEL_LEN - 1;
    }
    size_t hash = _hash_fn(channel, length);

    // Fast Read (Multiple threads simultaneously)
    pthread_rwlock_rdlock(&g_p_manager->bucket_mutex);
    int i = hash % g_p_manager->capacity;

    hash_node_t** p_node;
    for (p_node = &g_p_manager->p_bucket[i]; *p_node != NULL; p_node = &(*p_node)->p_next) {
        if ((*p_node)->channel_length == length && memcmp((*p_node)->logger.channel, channel, length) == 0) {
            //return &(*p_node)->logger;
            _lw_loggerf_t* p_logger = &(*p_node)->logger;
            pthread_rwlock_unlock(&g_p_manager->bucket_mutex);
            return p_logger;
        }
    }

    pthread_rwlock_unlock(&g_p_manager->bucket_mutex);

    // Exclusive Write (Only one thread)
    pthread_rwlock_wrlock(&g_p_manager->bucket_mutex);
    // Double-check. While we were waiting for the wrlock, another thread might
    // have already created this channel.
    i = hash % g_p_manager->capacity;
    for (p_node = &g_p_manager->p_bucket[i]; *p_node != NULL; p_node = &(*p_node)->p_next) {
        if ((*p_node)->channel_length == length && memcmp((*p_node)->logger.channel, channel, length) == 0) {
            _lw_loggerf_t* p_logger = &(*p_node)->logger;
            pthread_rwlock_unlock(&g_p_manager->bucket_mutex);
            return p_logger;
        }
    }

    // Hash table resizing and rehashing
    if (g_p_manager->size >= g_p_manager->capacity) {
        size_t capacity = g_p_manager->capacity * 2;
        hash_node_t** p_bucket = (hash_node_t**)malloc(capacity * sizeof(hash_node_t*));
        if (p_bucket == NULL) {
            pthread_rwlock_unlock(&g_p_manager->bucket_mutex);
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
                hash = _hash_fn(p_old_node->logger.channel, p_old_node->channel_length);
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
        // Recursive call to insert into the already expanded table
        pthread_rwlock_unlock(&g_p_manager->bucket_mutex);
        return _get_logger_dynamic_size(channel);

        // Recalculate the bucket index for the current insertion using the new capacity
        //i = hash % g_p_manager->capacity;
        //p_node = &g_p_manager->p_bucket[i];
        //while (*p_node != NULL) {
        //    p_node = &(*p_node)->p_next;
        //}
    }

    // Allocation of memory for a new node
    *p_node = (hash_node_t*)malloc(sizeof(hash_node_t));
    if (*p_node == NULL) {
        pthread_rwlock_unlock(&g_p_manager->bucket_mutex);
        return NULL;
    }
    (*p_node)->p_next = NULL;
    ++g_p_manager->size;

    (*p_node)->logger.p_logger = g_p_manager->logger_fn;
    // It is assumed that the level is initialized to default (hardcoded as debug in the code)
    (*p_node)->logger.level = debug;
    memcpy((*p_node)->logger.channel, channel, length);
    (*p_node)->logger.channel[length] = '\0';
    (*p_node)->channel_length = length;

    pthread_rwlock_unlock(&g_p_manager->bucket_mutex);
    return &(*p_node)->logger;
}

/**
 *  \brief  Retrieves an existing logger channel or takes a new one from the
 *      preallocated pool.
 *  \param  channel - the name of the requested channel.
 *  \return Pointer to the logger structure, or NULL if the pool is full.
 *
 *  \details    Operates without additional `malloc` system calls.
 */
static _lw_loggerf_t* _get_logger_fixed_size(const char* channel)
{
    assert(g_p_manager != NULL && "Logging manager is not initialized");

    int length = strlen(channel);
    if (length > (LOG_CHANNEL_LEN - 1)) {
        length = LOG_CHANNEL_LEN - 1;
    }
    size_t hash = _hash_fn(channel, length);

    // Fast Read (Multiple threads simultaneously)
    pthread_rwlock_rdlock(&g_p_manager->bucket_mutex);
    int i = hash % g_p_manager->capacity;

    hash_node_t** p_node;
    for (p_node = &g_p_manager->p_bucket[i]; *p_node != NULL; p_node = &(*p_node)->p_next) {
        if ((*p_node)->channel_length == length && memcmp((*p_node)->logger.channel, channel, length) == 0) {
            //return &(*p_node)->logger;
            _lw_loggerf_t* p_logger = &(*p_node)->logger;
            pthread_rwlock_unlock(&g_p_manager->bucket_mutex);
            return p_logger;
        }
    }

    pthread_rwlock_unlock(&g_p_manager->bucket_mutex);

    // Exclusive Write (Only one thread)
    pthread_rwlock_wrlock(&g_p_manager->bucket_mutex);
    // Double-check. While we were waiting for the wrlock, another thread might
    // have already created this channel.
    i = hash % g_p_manager->capacity;
    for (p_node = &g_p_manager->p_bucket[i]; *p_node != NULL; p_node = &(*p_node)->p_next) {
        if ((*p_node)->channel_length == length && memcmp((*p_node)->logger.channel, channel, length) == 0) {
            _lw_loggerf_t* p_logger = &(*p_node)->logger;
            pthread_rwlock_unlock(&g_p_manager->bucket_mutex);
            return p_logger;
        }
    }

    if (g_p_manager->size >= g_p_manager->capacity) {
        pthread_rwlock_unlock(&g_p_manager->bucket_mutex);
        return NULL;
    }
    if (g_p_manager->p_pool == NULL) {
        pthread_rwlock_unlock(&g_p_manager->bucket_mutex);
        return NULL;
    }

    // Allocation from a fixed array
    *p_node = &g_p_manager->p_pool[g_p_manager->size];
    (*p_node)->p_next = NULL;
    ++g_p_manager->size;

    (*p_node)->logger.level = debug;
    memcpy((*p_node)->logger.channel, channel, length);
    (*p_node)->logger.channel[length] = '\0';
    (*p_node)->channel_length = length;

    pthread_rwlock_unlock(&g_p_manager->bucket_mutex);
    return &(*p_node)->logger;
}

/*******************************************************************************
 * Public interface
 ******************************************************************************/

bool lw_can_log(int lvl)
{
    assert(g_p_manager != NULL && "Logging manager is not initialized");
    if (lvl < 0 || lvl > LVL_TRACE) {
        return false;
    }
    // Safe atomic read
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
    // Safe atomic read
    return p_logger->level >= lvl;
}

lw_loggerf_t lw_get_logger(const char* channel)
{
    assert(g_p_manager != NULL && "Logging manager is not initialized");
    return g_p_manager->get_logger_fn(channel);
}

lw_loggerf_t lw_get_logger_dfl(const char* channel, lw_severity_level_t dfl_lvl)
{
    assert(g_p_manager != NULL && "Logging manager is not initialized");

    _lw_loggerf_t* p_logger = g_p_manager->get_logger_fn(channel);
    if (p_logger != NULL) {
        p_logger->level = dfl_lvl;
    }
    return p_logger;
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

    // Initialize the read/write mutex
    if (pthread_rwlock_init(&g_p_manager->bucket_mutex, NULL) != 0) {
        free(g_p_manager);
        g_p_manager = NULL;
        return false;
    }

    g_p_manager->size = 0;
    g_p_manager->capacity = channel_count;
    g_p_manager->global_lvl = dfl_lvl;
    g_p_manager->is_immutable = 0;
    g_p_manager->p_bucket = NULL;
    g_p_manager->p_pool = NULL;
    g_p_manager->p_root_logger = NULL;
    g_p_manager->logger_fn = p_logger_fn;
    if (policy == fixed_size) {
        g_p_manager->get_logger_fn = _get_logger_fixed_size;
    } else {
        g_p_manager->capacity = channel_count;
        g_p_manager->get_logger_fn = _get_logger_dynamic_size;
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

    loggingf_manager_t* p_manager = g_p_manager;

    pthread_rwlock_wrlock(&p_manager->bucket_mutex);

    g_p_manager = NULL;

    // Destroy the lock before freeing memory
    pthread_rwlock_unlock(&p_manager->bucket_mutex);
    pthread_rwlock_destroy(&p_manager->bucket_mutex);

    if (p_manager->p_bucket != NULL && p_manager->p_pool == NULL) {
        for (size_t i = 0; i < p_manager->capacity; ++i) {
            hash_node_t* p_node = p_manager->p_bucket[i];
            while (p_node != NULL) {
                hash_node_t* p_del_node = p_node;
                p_node = p_node->p_next;
                free(p_del_node);
            }
        }
    }

    free(p_manager->p_pool);
    free(p_manager->p_bucket);
    free(p_manager);
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
    if (! g_p_manager->is_immutable) {
        if ((lvl < emerg) || (lvl > trace)) {
            return;
        }
        g_p_manager->global_lvl = lvl;
    }
}

void lw_set_immutable_global_level(lw_severity_level_t lvl)
{
    assert(g_p_manager != NULL && "Logging manager is not initialized");
    if (! g_p_manager->is_immutable) {
        lw_set_global_level(lvl);
        g_p_manager->is_immutable = 1;
    }
}

void lw_set_logger_level(const char* channel, lw_severity_level_t lvl)
{
    assert(g_p_manager != NULL && "Logging, lw_logging_policy_t policy manager is not initialized");

    _lw_loggerf_t* p_logger = g_p_manager->get_logger_fn(channel);
    if (p_logger != NULL) {
        if ((lvl < emerg) || (lvl > trace)) {
            return;
        }
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

#undef _TS_FILL_DFL
