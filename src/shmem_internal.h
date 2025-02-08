/* -*- C -*-
 *
 * Copyright 2011 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S.  Government
 * retains certain rights in this software.
 *
 * Copyright (c) 2017 Intel Corporation. All rights reserved.
 * This software is available to you under the BSD license.
 *
 * This file is part of the Sandia OpenSHMEM software package. For license
 * information, see the LICENSE file in the top level directory of the
 * distribution.
 *
 */

#ifndef PORTALS_SHMEM_INTERNAL_H
#define PORTALS_SHMEM_INTERNAL_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>
#include <sys/param.h>

#include "shmemx.h"
#include "runtime.h"
#include "config.h"
#include "shmem_env.h"
#include "shmem_decl.h"

extern int shmem_internal_my_pe;
extern int shmem_internal_num_pes;

extern int shmem_internal_init_counter;
extern int shmem_internal_finalized;
extern int shmem_internal_thread_level;

extern void *shmem_internal_heap_base;
extern long shmem_internal_heap_length;
extern void *shmem_internal_data_base;
extern long shmem_internal_data_length;

extern void *shmem_external_heap_base;
extern long shmem_external_heap_length;

extern int shmem_external_heap_pre_initialized;
extern int shmem_external_heap_device_type;
extern int shmem_external_heap_device;

extern unsigned int shmem_internal_rand_seed;

#ifdef USE_HWLOC
#include <hwloc.h>
extern hwloc_topology_t shmem_internal_topology;
#endif

#define SHMEM_INTERNAL_HEAP_OVERHEAD (1024*1024)
#define SHMEM_INTERNAL_DIAG_STRLEN 1024
#define SHMEM_INTERNAL_DIAG_WRAPLEN 72

/* Note: must be accompanied by shmem_internal_my_pe in arguments */
#define RAISE_PE_PREFIX      "[%04d]        "
#define RAISE_PREINIT_PREFIX "[????]        "
#define RAISE_PREFIX         "              "


#define RAISE_WARN(ret)                                                 \
    do {                                                                \
        fprintf(stderr, "[%04d] WARN:  %s:%d %s, return code %d\n",     \
                shmem_internal_my_pe, __FILE__, __LINE__, __func__,     \
                (int) ret);                                             \
    } while (0)


#define RAISE_ERROR(ret)                                                \
    do {                                                                \
        fprintf(stderr, "[%04d] ERROR: %s:%d %s, return code %d\n",     \
                shmem_internal_my_pe, __FILE__, __LINE__, __func__,     \
                (int) ret);                                             \
        shmem_runtime_abort(1, PACKAGE_NAME " exited in error");        \
    } while (0)


#define RETURN_ERROR_STR(str)                                           \
    do {                                                                \
        fprintf(stderr, "[%04d] ERROR: %s:%d: %s\n"                     \
                RAISE_PE_PREFIX "%s\n",                                 \
                shmem_internal_my_pe, __FILE__, __LINE__, __func__,     \
                shmem_internal_my_pe, str);                             \
    } while (0)


#define RAISE_ERROR_STR(str)                                            \
    do {                                                                \
        RETURN_ERROR_STR(str);                                          \
        shmem_runtime_abort(1, PACKAGE_NAME " exited in error");        \
    } while (0)


#define RAISE_WARN_STR(str)                                             \
    do {                                                                \
        fprintf(stderr, "[%04d] WARN:  %s:%d: %s\n"                     \
                RAISE_PE_PREFIX "%s\n",                                 \
                shmem_internal_my_pe, __FILE__, __LINE__, __func__,     \
                shmem_internal_my_pe, str);                             \
    } while (0)


#define RETURN_ERROR_MSG(...)                                           \
    do {                                                                \
        char str[SHMEM_INTERNAL_DIAG_STRLEN];                           \
        size_t off;                                                     \
        off = snprintf(str, sizeof(str), "[%04d] ERROR: %s:%d: %s\n",   \
                       shmem_internal_my_pe, __FILE__, __LINE__, __func__); \
        off+= snprintf(str+off, sizeof(str)-off, RAISE_PE_PREFIX,       \
                       shmem_internal_my_pe);                           \
        off+= snprintf(str+off, sizeof(str)-off, __VA_ARGS__);          \
        fprintf(stderr, "%s", str);                                     \
    } while (0)


#define RAISE_ERROR_MSG(...)                                            \
    do {                                                                \
        RETURN_ERROR_MSG(__VA_ARGS__);                                  \
        shmem_runtime_abort(1, PACKAGE_NAME " exited in error");        \
    } while (0)


#define RAISE_WARN_MSG(...)                                             \
    do {                                                                \
        char str[SHMEM_INTERNAL_DIAG_STRLEN];                           \
        size_t off;                                                     \
        off = snprintf(str, sizeof(str), "[%04d] WARN:  %s:%d: %s\n",   \
                       shmem_internal_my_pe, __FILE__, __LINE__, __func__); \
        off+= snprintf(str+off, sizeof(str)-off, RAISE_PE_PREFIX,       \
                       shmem_internal_my_pe);                           \
        off+= snprintf(str+off, sizeof(str)-off, __VA_ARGS__);          \
        fprintf(stderr, "%s", str);                                     \
    } while (0)


#define RETURN_ERROR_MSG_PREINIT(...)                                   \
    do {                                                                \
        char str[SHMEM_INTERNAL_DIAG_STRLEN];                           \
        size_t off;                                                     \
        off = snprintf(str, sizeof(str), "[????] ERROR: %s:%d: %s\n",   \
                       __FILE__, __LINE__, __func__);                   \
        off+= snprintf(str+off, sizeof(str)-off, RAISE_PREINIT_PREFIX); \
        off+= snprintf(str+off, sizeof(str)-off, __VA_ARGS__);          \
        fprintf(stderr, "%s", str);                                     \
    } while (0)


#define DEBUG_STR(str)                                                  \
    do {                                                                \
        if(shmem_internal_params.DEBUG) {                               \
            fprintf(stderr, "[%04d] DEBUG: %s:%d: %s\n"                 \
                    RAISE_PE_PREFIX "%s\n",                             \
                    shmem_internal_my_pe, __FILE__, __LINE__, __func__, \
                    shmem_internal_my_pe, str);                         \
        }                                                               \
    } while(0)

#define DEBUG_MSG(...)                                                  \
    do {                                                                \
        if(shmem_internal_params.DEBUG) {                               \
            char str[SHMEM_INTERNAL_DIAG_STRLEN];                       \
            size_t off;                                                 \
            off = snprintf(str, sizeof(str), "[%04d] DEBUG: %s:%d: %s\n", \
                           shmem_internal_my_pe, __FILE__, __LINE__,    \
                           __func__);                                   \
            off+= snprintf(str+off, sizeof(str)-off, RAISE_PE_PREFIX,   \
                           shmem_internal_my_pe);                       \
            off+= snprintf(str+off, sizeof(str)-off, __VA_ARGS__);      \
            fprintf(stderr, "%s", str);                                 \
        }                                                               \
    } while(0)

#define SHMEM_CHECK_GOTO_MSG(ret, lbl, ...)                              \
    do {                                                                 \
        if (ret) {                                                       \
            RAISE_WARN_MSG(__VA_ARGS__);                                 \
            goto lbl;                                                    \
        }                                                                \
    } while(0)

#ifdef ENABLE_ERROR_CHECKING
#define SHMEM_ERR_CHECK_INITIALIZED()                                    \
    do {                                                                 \
        if (!shmem_internal_init_counter) {                               \
            RETURN_ERROR_STR(PACKAGE_NAME " library not initialized\n"); \
            abort();                                                     \
        }                                                                \
    } while (0)

#define SHMEM_ERR_CHECK_POSITIVE(arg)                                   \
    do {                                                                \
        if ((arg) <= 0) {                                               \
            RAISE_ERROR_MSG("Argument %s must be positive (%ld)\n",     \
                            #arg, (long) arg);                          \
        }                                                               \
    } while (0)

#define SHMEM_ERR_CHECK_NON_NEGATIVE(arg)                               \
    do {                                                                \
        if ((arg) < 0) {                                                \
            RAISE_ERROR_MSG("Argument %s must be greater or equal to zero (%ld)\n", \
                            #arg, (long) arg);                          \
        }                                                               \
    } while (0)

#define SHMEM_ERR_CHECK_ACTIVE_SET(PE_start, PE_stride, PE_size)                                        \
    do {                                                                                                \
        if (PE_start < 0 || (PE_stride) < 1 || PE_size < 0 ||                                           \
            PE_start + ((PE_size - 1) * (PE_stride)) > shmem_internal_num_pes) {                        \
            RAISE_ERROR_MSG("Invalid active set (PE_start = %d, PE_stride = %d, PE_size = %d)\n",       \
                            PE_start, (PE_stride), PE_size);                                            \
        }                                                                                               \
        if (! (shmem_internal_my_pe >= PE_start &&                                                      \
               shmem_internal_my_pe <= PE_start + ((PE_size-1) * (PE_stride)) &&                        \
               (shmem_internal_my_pe - PE_start) % (PE_stride) == 0)) {                                 \
            RAISE_ERROR_MSG("Calling PE (%d) is not a member of the active set\n",                      \
                            shmem_internal_my_pe);                                                      \
        }                                                                                               \
    } while (0)

#define SHMEM_ERR_CHECK_TEAM_VALID(team)                                                                \
    do {                                                                                                \
        if (team == SHMEM_TEAM_INVALID) {                                                               \
            RAISE_ERROR_STR("Invalid team argument");                                                   \
        }                                                                                               \
    } while (0)

#define SHMEM_ERR_CHECK_PE(pe)                                          \
    do {                                                                \
        if ((pe) < 0 || (pe) >= shmem_internal_num_pes) {               \
            RAISE_ERROR_MSG("PE argument (%d) is invalid\n", (pe));     \
        }                                                               \
    } while (0)

#define SHMEM_ERR_CHECK_CTX(ctx)                                          \
    do {                                                                  \
        if (ctx == SHMEM_CTX_INVALID) {                                   \
            RAISE_ERROR_STR("Invalid ctx argument\n");                    \
        }                                                                 \
    } while (0)

#define SHMEM_ERR_CHECK_SYMMETRIC(ptr_in, len)                                                \
    do {                                                                                      \
        const void *ptr_base = (void*)(ptr_in);                                               \
        const void *ptr_ext  = (void*)((uint8_t *) (ptr_base) + (len));                       \
        const void *data_ext = (void*)((uint8_t *) shmem_internal_data_base +                 \
                                                   shmem_internal_data_length);               \
        const void *heap_ext = (void*)((uint8_t *) shmem_internal_heap_base +                 \
                                                   shmem_internal_heap_length);               \
        if (len == 0) {                                                                       \
            break; /* Skip this check when the length is 0 */                                 \
        }                                                                                     \
        else if (ptr_base >= shmem_internal_data_base && ptr_base < data_ext) {               \
            if (ptr_ext > data_ext) {                                                         \
                RAISE_ERROR_MSG("Argument \"%s\" [%p..%p) exceeds "                           \
                                "sym. data region [%p..%p)\n", #ptr_in, ptr_base,             \
                                ptr_ext, shmem_internal_data_base, data_ext);                 \
            }                                                                                 \
        }                                                                                     \
        else if (ptr_base >= shmem_internal_heap_base && ptr_base < heap_ext) {               \
            if (ptr_ext > heap_ext) {                                                         \
                RAISE_ERROR_MSG("Argument \"%s\" [%p..%p) exceeds "                           \
                                "sym. heap region [%p..%p)\n", #ptr_in, ptr_base,             \
                                ptr_ext, shmem_internal_heap_base, heap_ext);                 \
            }                                                                                 \
        }                                                                                     \
        else if (shmem_external_heap_base) {                                                  \
            const void *heap_ext_external = (void*)((uint8_t *) shmem_external_heap_base +    \
                                                   shmem_external_heap_length);               \
            if (ptr_base >= shmem_external_heap_base && ptr_base < heap_ext_external) {       \
                if (ptr_ext > heap_ext_external) {                                            \
                    RAISE_ERROR_MSG("Argument \"%s\" [%p..%p) exceeds device "                \
                                    "sym. heap region [%p..%p)\n", #ptr_in, ptr_base,         \
                                    ptr_ext, shmem_external_heap_base, heap_ext_external);    \
                }                                                                             \
            }                                                                                 \
        }                                                                                     \
        else {                                                                                \
            RAISE_ERROR_MSG("Argument \"%s\" is not symmetric (%p)\n",                        \
                            #ptr_in, ptr_base);                                               \
        }                                                                                     \
    } while (0)

#define SHMEM_ERR_CHECK_SYMMETRIC_HEAP(ptr_in)                                                                 \
    do {                                                                                                       \
        const void *ptr_base      = (void*)(ptr_in);                                                           \
        const void *heap_ext = (void*)((uint8_t *) shmem_internal_heap_base +                                  \
                                                   shmem_internal_heap_length);                                \
        if (! (ptr_base >= shmem_internal_heap_base && ptr_base < heap_ext)) {                                 \
            if (shmem_external_heap_base) {                                                                    \
                const void *heap_ext_external = (void*)((uint8_t *) shmem_external_heap_base +                 \
                                                   shmem_external_heap_length);                                \
                if (! (ptr_base >= shmem_external_heap_base && ptr_base < heap_ext_external)) {                \
                    RAISE_ERROR_MSG("Argument \"%s\" (%p) is not in host symm. heap [%p..%p) or device symm. heap [%p..%p)\n",    \
                        #ptr_in, ptr_base, shmem_internal_heap_base, heap_ext, shmem_external_heap_base, heap_ext_external);      \
                }                                                                                              \
                break;                                                                                         \
            }                                                                                                  \
            RAISE_ERROR_MSG("Argument \"%s\" (%p) is not in symm. heap [%p..%p)\n",                            \
                    #ptr_in, ptr_base, shmem_internal_heap_base, heap_ext);                                    \
        }                                                                                                      \
    } while (0)

/* Indicates whether the memory region pointed to by ptr1 (having size1) overlaps with
 * the buffer at ptr2 (having size2).
 *   SHMEM usage examples:
 *     For the collective routines (src/dest arguments):
 *         size1 == size2 and complete_overlap_allowed is 1
 *     For the pt2pt sync routines (ivars/status/indices arguments):
 *         size1 != size2 and complete_overlap_allowed is 0 */
#define SHMEM_ERR_CHECK_OVERLAP(ptr1, ptr2, size1, size2, complete_overlap_allowed, precheck) \
    do {                                                                                      \
        if (precheck) {                                                                       \
            const void *p1 = (void*)(ptr1);                                                   \
            const void *p2 = (void*)(ptr2);                                                   \
            const void *ptr_low  = p1 > p2 ? p2 : p1;                                         \
            const void *ptr_high = p1 > p2 ? p1 : p2;                                         \
            const size_t sz_low  = p1 > p2 ? size2 : size1;                                   \
            const void *ptr_extent = (void *)((char *)ptr_low + sz_low);                      \
            if (complete_overlap_allowed && p1 == p2) {                                       \
                break; /* Skip this check when buffer is allowed to completely overlap  */    \
            }                                                                                 \
            if (ptr_extent > ptr_high) {                                                      \
                RAISE_ERROR_MSG("Argument \"%s\" [%p..%p) overlaps argument (%p)\n", #ptr1,   \
                                ptr_low, ptr_extent, ptr_high);                               \
            }                                                                                 \
        }                                                                                     \
    } while (0)

#define SHMEM_ERR_CHECK_NULL(ptr, nelems)                                               \
    do {                                                                                \
        if (nelems > 0 && (ptr) == NULL) {                                              \
                RAISE_ERROR_MSG("Argument \"%s\" is NULL\n", #ptr);                     \
        }                                                                               \
    } while(0)

#define SHMEM_ERR_CHECK_CMP_OP(op)                                                      \
    do {                                                                                \
        switch(op) {                                                                    \
            case SHMEM_CMP_EQ:                                                          \
            case SHMEM_CMP_NE:                                                          \
            case SHMEM_CMP_GT:                                                          \
            case SHMEM_CMP_GE:                                                          \
            case SHMEM_CMP_LT:                                                          \
            case SHMEM_CMP_LE:                                                          \
                break;                                                                  \
            default:                                                                    \
                RAISE_ERROR_MSG("Argument \"%s\", invalid comparison operation (%d)\n", \
                                #op, (int) (op));                                       \
        }                                                                               \
    } while (0)

#define SHMEM_ERR_CHECK_SIG_OP(op)                                                      \
    do {                                                                                \
        switch(op) {                                                                    \
            case SHMEM_SIGNAL_SET:                                                      \
            case SHMEM_SIGNAL_ADD:                                                      \
                break;                                                                  \
            default:                                                                    \
                RAISE_ERROR_MSG("Argument \"%s\", invalid atomic operation for signal (%d)\n", \
                        #op, (int) (op));                                               \
        }                                                                               \
    } while (0)

#define SHMEM_ERR_CHECK_STRIDE_GTE_BSIZE(stride, bsize)                                 \
    do {                                                                                \
        if (stride < bsize) {                                                           \
            RAISE_ERROR_MSG("Stride argument \"%s\" (%zu), is less than block size %zu\n", \
                    #stride, stride, bsize);                                            \
        }                                                                               \
    } while(0)

#else
#define SHMEM_ERR_CHECK_INITIALIZED()
#define SHMEM_ERR_CHECK_POSITIVE(arg)
#define SHMEM_ERR_CHECK_NON_NEGATIVE(arg)
#define SHMEM_ERR_CHECK_ACTIVE_SET(PE_start, PE_stride, PE_size)
#define SHMEM_ERR_CHECK_TEAM_VALID(team)
#define SHMEM_ERR_CHECK_PE(pe)
#define SHMEM_ERR_CHECK_CTX(ctx)
#define SHMEM_ERR_CHECK_SYMMETRIC(ptr, len)
#define SHMEM_ERR_CHECK_SYMMETRIC_HEAP(ptr)
#define SHMEM_ERR_CHECK_OVERLAP(ptr1, ptr2, size1, size2, complete_overlap_allowed, precheck)
#define SHMEM_ERR_CHECK_NULL(ptr, nelems)
#define SHMEM_ERR_CHECK_CMP_OP(op)
#define SHMEM_ERR_CHECK_SIG_OP(op)
#define SHMEM_ERR_CHECK_STRIDE_GTE_BSIZE(stride, bsize)

#endif /* ENABLE_ERROR_CHECKING */


/*
 * Internal Assertions
 *
 * Assertions are not compiled unless ENABLE_ERROR_CHECKING is defined.  The
 * "persistent" assertion, shmem_internal_assertp is always compiled.
 */
#define shmem_internal_assert_fail(file_, line_, func_, cond_)          \
    do {                                                                \
        fprintf(stderr, "[%04d] Assertion Failed: %s:%d: %s\n"          \
                        "[%04d]                   %s\n",                \
                shmem_internal_my_pe, file_, line_, __func__,           \
                shmem_internal_my_pe, cond_);                           \
        shmem_runtime_abort(1, PACKAGE_NAME " exited in error");        \
    } while (0)

#define shmem_internal_assertp(cond)                                    \
    do {                                                                \
        if (!(cond)) {                                                  \
            shmem_internal_assert_fail(__FILE__, __LINE__, __func__, #cond); \
        }                                                               \
    } while (0)

#ifdef ENABLE_ERROR_CHECKING
#define shmem_internal_assert(cond)                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            shmem_internal_assert_fail(__FILE__, __LINE__, __func__, #cond); \
        }                                                               \
    } while (0)
#else
#define shmem_internal_assert(cond)
#endif


#ifdef ENABLE_THREADS

#   ifdef ENABLE_PTHREAD_MUTEX
#include <pthread.h>
typedef pthread_mutex_t shmem_internal_mutex_t;

#   define SHMEM_MUTEX_INIT(_mutex)                                     \
    do {                                                                \
        if (shmem_internal_thread_level == SHMEM_THREAD_MULTIPLE)       \
            pthread_mutex_init(&_mutex, NULL);                          \
    } while (0)
#   define SHMEM_MUTEX_DESTROY(_mutex)                                  \
    do {                                                                \
        if (shmem_internal_thread_level == SHMEM_THREAD_MULTIPLE)       \
            pthread_mutex_destroy(&_mutex);                             \
    } while (0)
#   define SHMEM_MUTEX_LOCK(_mutex)                                     \
    do {                                                                \
        if (shmem_internal_thread_level == SHMEM_THREAD_MULTIPLE)       \
            pthread_mutex_lock(&_mutex);                                \
    } while (0)
#   define SHMEM_MUTEX_UNLOCK(_mutex)                                   \
    do {                                                                \
        if (shmem_internal_thread_level == SHMEM_THREAD_MULTIPLE)       \
            pthread_mutex_unlock(&_mutex);                              \
    } while (0)

#   else  /* !ENABLE_PTHREAD_MUTEX */
#include <shmem_atomic.h>
typedef shmem_spinlock_t shmem_internal_mutex_t;

#   define SHMEM_MUTEX_INIT(_mutex)                                     \
    do {                                                                \
        if (shmem_internal_thread_level == SHMEM_THREAD_MULTIPLE)       \
            shmem_spinlock_init(&_mutex);                               \
    } while (0)
#   define SHMEM_MUTEX_DESTROY(_mutex)                                  \
    do {                                                                \
        if (shmem_internal_thread_level == SHMEM_THREAD_MULTIPLE)       \
            shmem_spinlock_fini(&_mutex);                               \
    } while (0)
#   define SHMEM_MUTEX_LOCK(_mutex)                                     \
    do {                                                                \
        if (shmem_internal_thread_level == SHMEM_THREAD_MULTIPLE)       \
            shmem_spinlock_lock(&_mutex);                               \
    } while (0)
#   define SHMEM_MUTEX_UNLOCK(_mutex)                                   \
    do {                                                                \
        if (shmem_internal_thread_level == SHMEM_THREAD_MULTIPLE)       \
            shmem_spinlock_unlock(&_mutex);                             \
    } while (0)

#   endif /* ENABLE_PTHREAD_MUTEX */

extern shmem_internal_mutex_t shmem_internal_mutex_alloc;
extern shmem_internal_mutex_t shmem_internal_mutex_rand_r;

#else
#   define SHMEM_MUTEX_INIT(_mutex)
#   define SHMEM_MUTEX_DESTROY(_mutex)
#   define SHMEM_MUTEX_LOCK(_mutex)
#   define SHMEM_MUTEX_UNLOCK(_mutex)
#endif /* ENABLE_THREADS */

void shmem_internal_start_pes(int npes);
int  shmem_internal_init(int tl_requested, int *tl_provided);
int  shmem_internal_heap_preinit(int tl_requested, int *tl_provided);
int  shmem_internal_heap_postinit(void);
void shmem_internal_finalize(void);
void shmem_internal_global_exit(int status) SHMEM_ATTRIBUTE_NORETURN;

int shmem_internal_symmetric_init(void);
int shmem_internal_symmetric_fini(void);
int shmem_internal_collectives_init(void);

/* internal allocation, without a barrier */
void *shmem_internal_shmalloc(size_t size);
void* shmem_internal_get_next(intptr_t incr);

void dlfree(void*);

static inline void shmem_internal_free(void *ptr)
{
    /* It's fine to call dlfree with NULL, but better to avoid unnecessarily
     * taking the mutex in the threaded case. */
    if (ptr != NULL) {
        SHMEM_MUTEX_LOCK(shmem_internal_mutex_alloc);
        dlfree(ptr);
        SHMEM_MUTEX_UNLOCK(shmem_internal_mutex_alloc);
    }
}

/* Query PEs reachable using shared memory */
static inline int shmem_internal_get_shr_rank(int pe)
{
#ifdef USE_ON_NODE_COMMS
    return shmem_runtime_get_node_rank(pe);
#elif defined(USE_MEMCPY)
    return pe == shmem_runtime_get_rank() ? 0 : -1;
#else
    return -1;
#endif
}

static inline int shmem_internal_get_shr_size(void)
{
#ifdef USE_ON_NODE_COMMS
    return shmem_runtime_get_node_size();
#elif defined(USE_MEMCPY)
    return 1;
#else
    return 0;
#endif
}

static inline double shmem_internal_wtime(void)
{
    double wtime = 0.0;

#ifdef HAVE_CLOCK_GETTIME
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    wtime = tv.tv_sec;
    wtime += (double)tv.tv_nsec / 1.0e9;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    wtime = tv.tv_sec;
    wtime += (double)tv.tv_usec / 1.0e6;
#endif
    return wtime;
}

/* Utility functions */
char *shmem_util_wrap(const char *str, const size_t wraplen, const char *indent);
char *shmem_util_strerror(int errnum, char *buf, size_t buflen);

/* Backtrace functions */
void shmem_util_backtrace(void); 

#ifndef MAX
#define MAX(A,B) (A) > (B) ? (A) : (B)
#endif

extern uint64_t (*shmem_internal_gettid_fn)(void);
extern void shmem_internal_register_gettid(uint64_t (*gettid_fn)(void));

static inline
void shmem_internal_bit_set(unsigned char *ptr, size_t size, size_t index)
{
    shmem_internal_assert(size > 0 && (index < size * CHAR_BIT));

    size_t which_byte = index / CHAR_BIT;
    ptr[which_byte] |= (1 << (index % CHAR_BIT));

    return;
}

static inline
void shmem_internal_bit_clear(unsigned char *ptr, size_t size, size_t index)
{
    shmem_internal_assert(size > 0 && (index < size * CHAR_BIT));

    size_t which_byte = index / CHAR_BIT;
    ptr[which_byte] &= ~(1 << (index % CHAR_BIT));

    return;
}

static inline
unsigned char shmem_internal_bit_fetch(unsigned char *ptr, size_t size, size_t index)
{
    shmem_internal_assert(size > 0 && (index < size * CHAR_BIT));

    size_t which_byte = index / CHAR_BIT;
    return (ptr[which_byte] >> (index % CHAR_BIT)) & 1;
}

static inline
size_t shmem_internal_bit_1st_nonzero(const unsigned char *ptr, const size_t size)
{
    /* The following ignores endianess: */
    for(size_t i = 0; i < size; i++) {
        unsigned char bit_val = ptr[i];
        for (size_t j = 0; bit_val && j < CHAR_BIT; j++) {
            if (bit_val & 1) return i * CHAR_BIT + j;
            bit_val >>= 1;
        }
    }

    return -1;
}

/* Create a bit string of the format AAAAAAAA.BBBBBBBB into str for the byte
 * array passed via ptr. */
static inline
void shmem_internal_bit_to_string(char *str, size_t str_size,
                                  unsigned char *ptr, size_t ptr_size)
{
    size_t off = 0;

    for (size_t i = 0; i < ptr_size; i++) {
        for (size_t j = 0; j < CHAR_BIT; j++) {
            off += snprintf(str+off, str_size-off, "%s",
                            (ptr[i] & (1 << (CHAR_BIT-1-j))) ? "1" : "0");
            if (off >= str_size) return;
        }
        if (i < ptr_size - 1) {
            off += snprintf(str+off, str_size-off, ".");
            if (off >= str_size) return;
        }
    }
}

/* Return -1 if `global_pe` is not in the given active set.
 * If `global_pe` is in the active set, return the PE index within this set. */
static inline
int shmem_internal_pe_in_active_set(int global_pe, int PE_start, int PE_stride, int PE_size)
{
    if (PE_size == 1) return PE_start == global_pe ? 0 : -1;
    if (PE_stride == 0) return -1;
    int n = (global_pe - PE_start) / PE_stride;
    if ((global_pe < PE_start && PE_stride > 0) || (global_pe > PE_start && PE_stride < 0) ||
        (global_pe - PE_start) % PE_stride || n >= PE_size)
        return -1;
    else {
        return n;
    }
}

#endif
