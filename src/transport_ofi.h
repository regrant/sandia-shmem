/* -*- C -*-
 *
 * Copyright (c) 2017 Intel Corporation. All rights reserved.
 * This software is available to you under the BSD license.
 *
 * This file is part of the Sandia OpenSHMEM software package. For license
 * information, see the LICENSE file in the top level directory of the
 * distribution.
 *
 */

#ifndef TRANSPORT_OFI_H
#define TRANSPORT_OFI_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <rdma/fabric.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_tagged.h>
#include <rdma/fi_rma.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_atomic.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <inttypes.h>
#include "shmem_free_list.h"
#include "shmem_internal.h"
#include "shmem_atomic.h"
#include "shmem_team.h"
#include <sys/types.h>


#if !defined(ENABLE_HARD_POLLING)
#define ENABLE_TARGET_CNTR 1
#else
#define ENABLE_TARGET_CNTR 0
#endif

#if ENABLE_TARGET_CNTR
extern struct fid_cntr*                 shmem_transport_ofi_target_cntrfd;
#endif
#if ENABLE_MANUAL_PROGRESS
extern struct fid_cq*                   shmem_transport_ofi_target_cq;
#endif
#ifndef ENABLE_MR_SCALABLE
extern uint64_t*                        shmem_transport_ofi_target_heap_keys;
extern uint64_t*                        shmem_transport_ofi_target_data_keys;
#ifdef ENABLE_REMOTE_VIRTUAL_ADDRESSING
extern int                              shmem_transport_ofi_use_absolute_address;
#else
extern uint8_t**                        shmem_transport_ofi_target_heap_addrs;
extern uint8_t**                        shmem_transport_ofi_target_data_addrs;
#endif /* ENABLE_REMOTE_VIRTUAL_ADDRESSING */
#endif /* ENABLE_MR_SCALABLE */

#ifdef USE_FI_HMEM
extern uint64_t*                        shmem_transport_ofi_external_heap_keys;
extern uint8_t**                        shmem_transport_ofi_external_heap_addrs;
#endif

extern struct fid_mr*                   shmem_transport_ofi_mrfd_list[3];
extern uint64_t                         shmem_transport_ofi_max_poll;
extern long                             shmem_transport_ofi_put_poll_limit;
extern long                             shmem_transport_ofi_get_poll_limit;
extern size_t                           shmem_transport_ofi_max_buffered_send;
extern size_t                           shmem_transport_ofi_max_msg_size;
extern size_t                           shmem_transport_ofi_bounce_buffer_size;
extern long                             shmem_transport_ofi_max_bounce_buffers;

extern pthread_mutex_t                  shmem_transport_ofi_progress_lock;

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#define OFI_CHECK_ERROR(ret)                                                    \
    do {                                                                        \
        if (ret) {                                                              \
            RAISE_ERROR_MSG("OFI error %d: %s\n", ret, fi_strerror(ret));       \
        }                                                                       \
    } while (0)

#define OFI_CTX_CHECK_ERROR(ctx, /* ssize_t */ err)                             \
    do {                                                                        \
        if ((err) == -FI_EAVAIL) {                                              \
            struct fi_cq_err_entry e = {0};                                     \
            ssize_t ret = fi_cq_readerr((ctx)->cq, (void *)&e, 0);              \
            if (ret == 1) {                                                     \
                const char *errmsg = fi_cq_strerror((ctx)->cq, e.prov_errno,    \
                                                    e.err_data, NULL, 0);       \
                RAISE_ERROR_MSG("Error in operation: %s\n", errmsg);            \
            } else {                                                            \
                RAISE_ERROR_MSG("Error reading from CQ (%zd)\n", ret);          \
            }                                                                   \
        } else if (err) {                                                       \
            RAISE_ERROR_MSG("OFI error %zd: %s\n", err, fi_strerror(err));      \
        }                                                                       \
    } while (0)

#define OFI_CHECK_ERROR_MSG(ret, ...)                                           \
    do {                                                                        \
        if (ret) {                                                              \
            RAISE_ERROR_MSG(__VA_ARGS__);                                       \
        }                                                                       \
    } while (0)

#define OFI_CHECK_RETURN_STR(ret, msg)                                          \
    do {                                                                        \
        if (ret) {                                                              \
            RAISE_WARN_STR(msg);                                                \
            return ret;                                                         \
        }                                                                       \
    } while (0)

#define OFI_CHECK_RETURN_MSG(ret, ...)                                          \
    do {                                                                        \
        if (ret) {                                                              \
            RAISE_WARN_MSG(__VA_ARGS__);                                        \
            return ret;                                                         \
        }                                                                       \
    } while (0)


#ifdef ENABLE_MR_SCALABLE
static inline
int shmem_transport_ofi_get_mr_desc_index(const void *addr) {
    int ret = -1;
#ifdef ENABLE_REMOTE_VIRTUAL_ADDRESSING
    ret = 0;
#else
    if ((void*) addr >= shmem_internal_data_base &&
        (uint8_t*) addr < (uint8_t*) shmem_internal_data_base + shmem_internal_data_length) {
        ret = 0;
    } else if ((void*) addr >= shmem_internal_heap_base &&
               (uint8_t*) addr < (uint8_t*) shmem_internal_heap_base + shmem_internal_heap_length) {
        ret = 1;
    } else {
        ret = -1;
    }
#endif /* ENABLE_REMOTE_VIRTUAL_ADDRESSING */
    return ret;
}
#else
static inline
int shmem_transport_ofi_get_mr_desc_index(const void *addr) {
    int ret = -1;
    if ((void*) addr >= shmem_internal_data_base &&
        (uint8_t*) addr < (uint8_t*) shmem_internal_data_base + shmem_internal_data_length) {
        ret = 0;
    } else if ((void*) addr >= shmem_internal_heap_base &&
               (uint8_t*) addr < (uint8_t*) shmem_internal_heap_base + shmem_internal_heap_length) {
        ret = 1;
    }
#ifdef USE_FI_HMEM
    else if (shmem_external_heap_pre_initialized) {
        if ((void*) addr >= shmem_external_heap_base &&
             (uint8_t*) addr < (uint8_t*) shmem_external_heap_base + shmem_external_heap_length) {
            ret = 2;
        }
    }
#endif /* USE_FI_HMEM */
    else {
        ret = -1;
    }
    return ret;
}
#endif

#ifdef ENABLE_MR_SCALABLE
static inline
void shmem_transport_ofi_get_mr(const void *addr, int dest_pe,
                                uint8_t **mr_addr, uint64_t *key) {
#ifdef ENABLE_REMOTE_VIRTUAL_ADDRESSING
    *key = 0;
    *mr_addr = (uint8_t*) addr;
#else
    if ((void*) addr >= shmem_internal_data_base &&
        (uint8_t*) addr < (uint8_t*) shmem_internal_data_base + shmem_internal_data_length) {

        *key = 0;
        *mr_addr = (uint8_t*) ((uint8_t *) addr - (uint8_t *) shmem_internal_data_base);

    } else if ((void*) addr >= shmem_internal_heap_base &&
               (uint8_t*) addr < (uint8_t*) shmem_internal_heap_base + shmem_internal_heap_length) {

        *key = 1;
        *mr_addr = (uint8_t*) ((uint8_t *) addr - (uint8_t *) shmem_internal_heap_base);
    } else {
        *key = 0;
        *mr_addr = NULL;
        RAISE_ERROR_MSG("address (%p) outside of symmetric areas\n", addr);
    }
#endif /* ENABLE_REMOTE_VIRTUAL_ADDRESSING */

}

#else
static inline
void shmem_transport_ofi_get_mr(const void *addr, int dest_pe,
                                uint8_t **mr_addr, uint64_t *key) {
    if ((void*) addr >= shmem_internal_data_base &&
        (uint8_t*) addr < (uint8_t*) shmem_internal_data_base + shmem_internal_data_length) {
        *key = shmem_transport_ofi_target_data_keys[dest_pe];
#ifdef ENABLE_REMOTE_VIRTUAL_ADDRESSING
        if (shmem_transport_ofi_use_absolute_address)
            *mr_addr = (uint8_t *) addr;
        else
            *mr_addr = (void *) ((uint8_t *) addr - (uint8_t *) shmem_internal_data_base);
#else
        *mr_addr = shmem_transport_ofi_target_data_addrs[dest_pe] +
            ((uint8_t *) addr - (uint8_t *) shmem_internal_data_base);
#endif
    }

    else if ((void*) addr >= shmem_internal_heap_base &&
             (uint8_t*) addr < (uint8_t*) shmem_internal_heap_base + shmem_internal_heap_length) {
        *key = shmem_transport_ofi_target_heap_keys[dest_pe];
#ifdef ENABLE_REMOTE_VIRTUAL_ADDRESSING
        if (shmem_transport_ofi_use_absolute_address)
            *mr_addr = (uint8_t *) addr;
        else
            *mr_addr = (void *) ((uint8_t *) addr - (uint8_t *) shmem_internal_heap_base);
#else
        *mr_addr = shmem_transport_ofi_target_heap_addrs[dest_pe] +
            ((uint8_t *) addr - (uint8_t *) shmem_internal_heap_base);
#endif
    }
#ifdef USE_FI_HMEM
    else if (shmem_external_heap_pre_initialized) {
        if ((void*) addr >= shmem_external_heap_base &&
             (uint8_t*) addr < (uint8_t*) shmem_external_heap_base + shmem_external_heap_length) {
            *key = shmem_transport_ofi_external_heap_keys[dest_pe];
            *mr_addr = shmem_transport_ofi_external_heap_addrs[dest_pe] +
                ((uint8_t *) addr - (uint8_t *) shmem_external_heap_base);
        }
    }
#endif /* USE_FI_HMEM */
    else {
        *key = -1;
        *mr_addr = NULL;
        RAISE_ERROR_MSG("address (%p) outside of symmetric areas\n", addr);
    }
}
#endif

/* Datatypes */
extern int shmem_transport_dtype_table[];

#define SHMEM_TRANSPORT_DTYPE(DTYPE) shmem_transport_dtype_table[(DTYPE)]

/* Operations */
typedef enum fi_op       shm_internal_op_t;

#define SHM_INTERNAL_BAND            FI_BAND
#define SHM_INTERNAL_BOR             FI_BOR
#define SHM_INTERNAL_BXOR            FI_BXOR
#define SHM_INTERNAL_MIN             FI_MIN
#define SHM_INTERNAL_MAX             FI_MAX
#define SHM_INTERNAL_SUM             FI_SUM
#define SHM_INTERNAL_PROD            FI_PROD

#define SHMEM_TRANSPORT_OFI_TYPE_BOUNCE 0x01
#define SHMEM_TRANSPORT_OFI_TYPE_LONG   0x02


extern fi_addr_t *addr_table;

#ifdef USE_AV_MAP
#define GET_DEST(dest) ((fi_addr_t)(addr_table[(dest)]))
#else
#define GET_DEST(dest) ((fi_addr_t)(dest))
#endif

#ifdef USE_FI_HMEM
#define GET_MR_DESC(index) ((index == -1) ? NULL : (void *) shmem_transport_ofi_mrfd_list[index])
#define GET_MR_DESC_ADDR(index) ((index == -1) ? NULL : (void **) &shmem_transport_ofi_mrfd_list[index])
#else
#define GET_MR_DESC(index) NULL
#define GET_MR_DESC_ADDR(index) NULL
#endif

struct shmem_transport_ofi_frag_t {
    shmem_free_list_item_t item;
    uint8_t mytype;
};

typedef struct shmem_transport_ofi_frag_t shmem_transport_ofi_frag_t;

struct shmem_transport_ofi_bounce_buffer_t {
    shmem_transport_ofi_frag_t frag;
    uint8_t data[];
};

typedef struct shmem_transport_ofi_bounce_buffer_t shmem_transport_ofi_bounce_buffer_t;

typedef int shmem_transport_ct_t;

enum shmem_internal_tid_t { tid_is_pid_t, tid_is_uint64_t };
struct shmem_internal_tid
{
    enum shmem_internal_tid_t tid_t;
    union
    {
        pid_t pid_val;
        uint64_t uint64_val;
    } val;
};

struct shmem_transport_ctx_t {
    int                             id;
#ifdef USE_CTX_LOCK
    shmem_internal_mutex_t          lock;
#endif
    long                            options;
    struct fid_ep*                  ep;
    struct fid_cntr*                put_cntr;
    struct fid_cntr*                get_cntr;
    struct fid_cq*                  cq;
#ifdef USE_CTX_LOCK
    /* Pending cntr accesses are protected by ctx lock */
    uint64_t                        pending_put_cntr;
    uint64_t                        pending_get_cntr;
#else
    shmem_internal_cntr_t           pending_put_cntr;
    shmem_internal_cntr_t           pending_get_cntr;
#endif
    /* These counters are protected by the BB lock */
    uint64_t                        pending_bb_cntr;
    uint64_t                        completed_bb_cntr;
    shmem_free_list_t              *bounce_buffers;
    int                             stx_idx;
    struct shmem_internal_tid       tid;
    struct shmem_internal_team_t   *team;
};

typedef struct shmem_transport_ctx_t shmem_transport_ctx_t;
extern shmem_transport_ctx_t shmem_transport_ctx_default;

extern struct fid_ep* shmem_transport_ofi_target_ep;

#ifdef USE_CTX_LOCK
#define SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx)                                       \
    do {                                                                        \
        if (!((ctx)->options & (SHMEM_CTX_PRIVATE | SHMEM_CTX_SERIALIZED)))     \
            SHMEM_MUTEX_LOCK((ctx)->lock);                                      \
    } while (0)

#define SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx)                                     \
    do {                                                                        \
        if (!((ctx)->options & (SHMEM_CTX_PRIVATE | SHMEM_CTX_SERIALIZED)))     \
            SHMEM_MUTEX_UNLOCK((ctx)->lock);                                    \
    } while (0)

#define SHMEM_TRANSPORT_OFI_CNTR_READ(cntr) *(cntr)
#define SHMEM_TRANSPORT_OFI_CNTR_INC(cntr) (*(cntr))++

#else
#define SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx)
#define SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx)
#define SHMEM_TRANSPORT_OFI_CNTR_READ(cntr) shmem_internal_cntr_read(cntr)
#define SHMEM_TRANSPORT_OFI_CNTR_INC(cntr) shmem_internal_cntr_inc(cntr)
#endif /* USE_CTX_LOCK */

#define SHMEM_TRANSPORT_OFI_CTX_BB_LOCK(ctx)                                    \
    do {                                                                        \
        shmem_internal_assert(ctx->bounce_buffers != NULL);                     \
        if (!((ctx)->options & (SHMEM_CTX_PRIVATE | SHMEM_CTX_SERIALIZED)))     \
            shmem_free_list_lock(ctx->bounce_buffers);                          \
    } while (0)

#define SHMEM_TRANSPORT_OFI_CTX_BB_UNLOCK(ctx)                                  \
    do {                                                                        \
        if (!((ctx)->options & (SHMEM_CTX_PRIVATE | SHMEM_CTX_SERIALIZED)))     \
            shmem_free_list_unlock(ctx->bounce_buffers);                        \
    } while (0)

static inline
void shmem_transport_probe(void)
{
#if defined(ENABLE_MANUAL_PROGRESS)
#  ifdef USE_THREAD_COMPLETION
    if (0 == pthread_mutex_trylock(&shmem_transport_ofi_progress_lock)) {
#  endif
        struct fi_cq_entry buf;
        int ret = fi_cq_read(shmem_transport_ofi_target_cq, &buf, 1);
        if (ret == 1)
            RAISE_WARN_STR("Unexpected event");
#  ifdef USE_THREAD_COMPLETION
        pthread_mutex_unlock(&shmem_transport_ofi_progress_lock);
    }
#  endif
#endif

    return;
}

int shmem_transport_ctx_create(struct shmem_internal_team_t *team, long options, shmem_transport_ctx_t **ctx);
void shmem_transport_ctx_destroy(shmem_transport_ctx_t *ctx);

int shmem_transport_session_start(shmem_transport_ctx_t *ctx, long options, const shmem_session_config_t *config, long config_mask);
int shmem_transport_session_stop(shmem_transport_ctx_t *ctx);

int shmem_transport_init(void);
int shmem_transport_startup(void);
int shmem_transport_fini(void);

extern size_t SHMEM_Dtsize[FI_DATATYPE_LAST];

static inline void shmem_transport_get_wait(shmem_transport_ctx_t* ctx);

/* Drain all available events from the CQ.  Note, ctx->bounce_buffers must be
 * locked before calling this routine */
static inline
void shmem_transport_ofi_drain_cq(shmem_transport_ctx_t *ctx)
{
    ssize_t ret = 0;
    struct fi_cq_entry buf;

    for (;;) {
        ret = fi_cq_read(ctx->cq, (void *)&buf, 1);

        if (ret == -FI_EAGAIN) break; /* No events */

        else if (ret == 1) {
            shmem_transport_ofi_frag_t *frag =
                (shmem_transport_ofi_frag_t *) buf.op_context;

            if (SHMEM_TRANSPORT_OFI_TYPE_BOUNCE == frag->mytype) {
                shmem_free_list_free(ctx->bounce_buffers,
                                     (shmem_transport_ofi_bounce_buffer_t *) frag);
                ctx->completed_bb_cntr++;
            } else {
                RAISE_ERROR_STR("Unrecognized completion object");
            }
        }

        else if (ret < 0) {
            OFI_CTX_CHECK_ERROR(ctx, ret);
        }

        else {
            RAISE_ERROR_MSG("fi_cq_read returned invalid event count (%zd)\n", ret);
        }
    }
}

static inline
shmem_transport_ofi_bounce_buffer_t * create_bounce_buffer(shmem_transport_ctx_t *ctx,
                                                           const void *source,
                                                           const size_t len)
{
    shmem_transport_ofi_bounce_buffer_t *buff;

    SHMEM_TRANSPORT_OFI_CTX_BB_LOCK(ctx);

    shmem_internal_assert(shmem_transport_ofi_max_bounce_buffers > 0);

    while (ctx->bounce_buffers->nalloc >= (uint64_t) shmem_transport_ofi_max_bounce_buffers) {
        shmem_transport_ofi_drain_cq(ctx);
    }

    buff = (shmem_transport_ofi_bounce_buffer_t*) shmem_free_list_alloc(ctx->bounce_buffers);
    ctx->pending_bb_cntr++;

    SHMEM_TRANSPORT_OFI_CTX_BB_UNLOCK(ctx);

    if (NULL == buff)
        RAISE_ERROR_STR("Bounce buffer allocation failed");

    shmem_internal_assert(buff->frag.mytype == SHMEM_TRANSPORT_OFI_TYPE_BOUNCE);

    memcpy(buff->data, source, len);

    return buff;
}

static inline
void shmem_transport_put_quiet(shmem_transport_ctx_t* ctx)
{
    SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);

    /* Wait for bounce buffered operations to complete */
    if (ctx->bounce_buffers) {
        SHMEM_TRANSPORT_OFI_CTX_BB_LOCK(ctx);

        while (ctx->bounce_buffers->nalloc > 0) {
            shmem_transport_ofi_drain_cq(ctx);
        }

        SHMEM_TRANSPORT_OFI_CTX_BB_UNLOCK(ctx);
    }

    /* wait for put counter to meet outstanding count value */

    /* Note: the communication routines increment pending put counters before
     * each FI call (thus before the corresponding event counter is
     * incremented), but the completion routine below reads the counters in the
     * reverse order: first the fid_cntr event counter, then the put issued
     * counter.  We'll want to preserve this property in the future.
     */
    uint64_t success, fail, cnt, cnt_new;
    long poll_count = 0;
    while (poll_count < shmem_transport_ofi_put_poll_limit ||
           shmem_transport_ofi_put_poll_limit < 0) {
        success = fi_cntr_read(ctx->put_cntr);
        fail = fi_cntr_readerr(ctx->put_cntr);
        cnt = SHMEM_TRANSPORT_OFI_CNTR_READ(&ctx->pending_put_cntr);

        shmem_transport_probe();

        if (success < cnt && fail == 0) {
            SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
            SPINLOCK_BODY();
            SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);
        } else if (fail) {
            RAISE_ERROR_MSG("Operations completed in error (%" PRIu64 ")\n", fail);
        } else {
            SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
            return;
        }
        poll_count++;
    }
    cnt_new = SHMEM_TRANSPORT_OFI_CNTR_READ(&ctx->pending_put_cntr);
    do {
        cnt = cnt_new;
        ssize_t ret = fi_cntr_wait(ctx->put_cntr, cnt, -1);
        cnt_new = SHMEM_TRANSPORT_OFI_CNTR_READ(&ctx->pending_put_cntr);
        OFI_CTX_CHECK_ERROR(ctx, ret);
    } while (cnt < cnt_new);
    shmem_internal_assert(cnt == cnt_new);

    SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
}

static inline
int shmem_transport_quiet(shmem_transport_ctx_t* ctx)
{

    shmem_transport_put_quiet(ctx);
    shmem_transport_get_wait(ctx);

    return 0;
}


static inline
int shmem_transport_fence(shmem_transport_ctx_t* ctx)
{
#if WANT_TOTAL_DATA_ORDERING == 0
    /* Communication is unordered; must wait for puts and buffered (injected)
     * non-fetching atomics to be completed in order to ensure ordering. */
    shmem_transport_put_quiet(ctx);
#endif
    /* Complete fetching ops; needed to support nonblocking fetch-atomics */
    shmem_transport_get_wait(ctx);

    return 0;
}


/* Process RMA operation return code.  If libfabric returned -FI_EAGAIN, attempt
 * to reclaim resources and indicate that the operation should be retried.  If
 * retry limit (ofi_max_poll) is exceeded, abort. */
static inline
int try_again(shmem_transport_ctx_t *ctx, const int ret, uint64_t *polled) {

    if (ret) {
        if (ret == -FI_EAGAIN) {
            if (ctx->bounce_buffers) {
                SHMEM_TRANSPORT_OFI_CTX_BB_LOCK(ctx);
                shmem_transport_ofi_drain_cq(ctx);
                SHMEM_TRANSPORT_OFI_CTX_BB_UNLOCK(ctx);
            }
            else {
                /* Poke CQ for errors to encourage progress */
                struct fi_cq_err_entry e = {0};
                ssize_t ret = fi_cq_readerr(ctx->cq, (void *)&e, 0);
                if (ret == 1) {
                    const char *errmsg = fi_cq_strerror(ctx->cq, e.prov_errno,
                                                        e.err_data, NULL, 0);
                    RAISE_ERROR_MSG("Error in operation: %s\n", errmsg);
                } else if (ret && ret != -FI_EAGAIN) {
                    RAISE_ERROR_MSG("Error reading from CQ (%zd)\n", ret);
                }
            }

            shmem_transport_probe();

            (*polled)++;

            if ((*polled) <= shmem_transport_ofi_max_poll) {
                return 1;
            }
            else {
                RAISE_ERROR_MSG("Operation retry limit exceeded (%" PRIu64 ")\n",
                                shmem_transport_ofi_max_poll);
            }
        }
        else {
            OFI_CTX_CHECK_ERROR(ctx, (ssize_t) ret);
        }
    }

    return 0;
}


static inline
void shmem_transport_put_scalar(shmem_transport_ctx_t* ctx, void *target, const
                               void *source, size_t len, int pe)
{
    int ret = 0;
    uint64_t dst = (uint64_t) pe;
    uint64_t polled = 0;
    uint64_t key;
    uint8_t *addr;

    shmem_transport_ofi_get_mr(target, pe, &addr, &key);

    shmem_internal_assert(len <= shmem_transport_ofi_max_buffered_send);

    SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);
    SHMEM_TRANSPORT_OFI_CNTR_INC(&ctx->pending_put_cntr);

    do {

        ret = fi_inject_write(ctx->ep,
                              source,
                              len,
                              GET_DEST(dst),
                              (uint64_t) addr,
                              key);

    } while (try_again(ctx, ret, &polled));
    SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
}

static inline
void shmem_transport_ofi_put_large(shmem_transport_ctx_t* ctx, void *target, const void *source,
                                   size_t len, int pe)
{
    int ret = 0;
    uint64_t dst = (uint64_t) pe;
    uint64_t polled = 0;
    uint64_t key;
    uint8_t *addr;

    shmem_transport_ofi_get_mr(target, pe, &addr, &key);

    uint8_t *frag_source = (uint8_t *) source;
    uint64_t frag_target = (uint64_t) addr;
    size_t frag_len = len;

    /* operation generates counting events and must be completed by
     * quiet. */
    SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);
    while (frag_source < ((uint8_t *) source) + len) {
        frag_len = MIN(shmem_transport_ofi_max_msg_size,
                       (size_t) (((uint8_t *) source) + len - frag_source));
        polled = 0;

        SHMEM_TRANSPORT_OFI_CNTR_INC(&ctx->pending_put_cntr);

        do {
            ret = fi_write(ctx->ep,
                           frag_source, frag_len,
                           GET_MR_DESC(shmem_transport_ofi_get_mr_desc_index(source)),
                           GET_DEST(dst), frag_target,
                           key, NULL);
        } while (try_again(ctx, ret, &polled));

        frag_source += frag_len;
        frag_target += frag_len;
    }
    SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
}

static inline
void shmem_transport_put_nb(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                            int pe, long *completion)
{
    int ret = 0;
    uint64_t dst = (uint64_t) pe;
    uint64_t polled = 0;
    uint64_t key;
    uint8_t *addr;

    shmem_internal_assert(completion != NULL);

    if (len <= shmem_transport_ofi_max_buffered_send) {

        shmem_transport_put_scalar(ctx, target, source, len, pe);

    } else if (len <= shmem_transport_ofi_bounce_buffer_size && ctx->bounce_buffers) {

        SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);
        SHMEM_TRANSPORT_OFI_CNTR_INC(&ctx->pending_put_cntr);
        shmem_transport_ofi_get_mr(target, pe, &addr, &key);

        shmem_transport_ofi_bounce_buffer_t *buff =
            create_bounce_buffer(ctx, source, len);
        polled = 0;

        const struct iovec      msg_iov = { .iov_base = buff->data, .iov_len = len };
        const struct fi_rma_iov rma_iov = { .addr = (uint64_t) addr, .len = len, .key = key };
        const struct fi_msg_rma msg     = {
                                            .msg_iov       = &msg_iov,
                                            .desc          = GET_MR_DESC_ADDR(shmem_transport_ofi_get_mr_desc_index(source)),
                                            .iov_count     = 1,
                                            .addr          = GET_DEST(dst),
                                            .rma_iov       = &rma_iov,
                                            .rma_iov_count = 1,
                                            .context       = buff,
                                            .data          = 0
                                          };
        do {
            ret = fi_writemsg(ctx->ep, &msg, FI_COMPLETION | FI_DELIVERY_COMPLETE);
        } while (try_again(ctx, ret, &polled));
        SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);

    } else {
        shmem_transport_ofi_put_large(ctx, target, source,len, pe);
        (*completion)++;
    }
}

static inline
void shmem_transport_put_signal_nbi(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                                    uint64_t *sig_addr, uint64_t signal, int sig_op, int pe)
{
    int ret = 0;
    uint64_t dst = (uint64_t) pe;
    uint64_t polled = 0;
    uint64_t key;
    uint8_t *addr;

    shmem_transport_ofi_get_mr(target, pe, &addr, &key);

    if (len <= shmem_transport_ofi_max_buffered_send) {
        uint8_t *src_buf = (uint8_t *) source;

        SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);
        SHMEM_TRANSPORT_OFI_CNTR_INC(&ctx->pending_put_cntr);

        const struct iovec msg_iov = {
                                       .iov_base = src_buf,
                                       .iov_len = len
                                     };
        const struct fi_rma_iov rma_iov = {
                                            .addr = (uint64_t) addr,
                                            .len = len,
                                            .key = key
                                          };
        const struct fi_msg_rma msg = {
                                        .msg_iov = &msg_iov,
                                        .desc = GET_MR_DESC_ADDR(shmem_transport_ofi_get_mr_desc_index(source)),
                                        .iov_count = 1,
                                        .addr = GET_DEST(dst),
                                        .rma_iov = &rma_iov,
                                        .rma_iov_count = 1,
                                        .context = src_buf,
                                        .data = 0
                                      };

        do {
            ret = fi_writemsg(ctx->ep, &msg, FI_DELIVERY_COMPLETE | FI_INJECT);
        } while (try_again(ctx, ret, &polled));

        SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
    } else {
        uint8_t *frag_source = (uint8_t *) source;
        uint64_t frag_target = (uint64_t) addr;
        size_t frag_len = len;

        struct iovec msg_iov = {
                                 .iov_base = frag_source,
                                 .iov_len = frag_len
                               };
        struct fi_rma_iov rma_iov = {
                                      .addr = frag_target,
                                      .len = frag_len,
                                      .key = key
                                    };
        struct fi_msg_rma msg = {
                                  .msg_iov = &msg_iov,
                                  .desc = GET_MR_DESC_ADDR(shmem_transport_ofi_get_mr_desc_index(source)),
                                  .iov_count = 1,
                                  .addr = GET_DEST(dst),
                                  .rma_iov = &rma_iov,
                                  .rma_iov_count = 1,
                                  .context = frag_source,
                                  .data = 0
                                };

        SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);
        while (frag_source < (((uint8_t *) source) + len)) {
            frag_len = MIN(shmem_transport_ofi_max_msg_size, 
                          (size_t) (((uint8_t *) source) + len - frag_source));
            polled = 0;

            msg_iov.iov_base = frag_source;
            msg_iov.iov_len = frag_len;

            rma_iov.addr = frag_target;
            rma_iov.len = frag_len;

            msg.msg_iov = &msg_iov;
            msg.rma_iov = &rma_iov;
            msg.context = frag_source;

            SHMEM_TRANSPORT_OFI_CNTR_INC(&ctx->pending_put_cntr);

            do {
                ret = fi_writemsg(ctx->ep, &msg, FI_DELIVERY_COMPLETE);
            } while (try_again(ctx, ret, &polled));

            frag_source += frag_len;
            frag_target += frag_len;
        }
        SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
    }

    uint64_t flags_signal = FI_DELIVERY_COMPLETE | FI_INJECT;
#ifndef USE_FI_FENCE /* FI_FENCE is not enabled by user. Using transport layer fence instead */
    shmem_transport_fence(ctx);
#else
    /* FI_FENCE assures completion of one or more (for fragmentation) prior puts through
     * signal delivery */
    flags_signal |= FI_FENCE;
#endif 

    /* Transmit the signal */
    shmem_transport_ofi_get_mr(sig_addr, pe, &addr, &key);
    polled = 0;
    ret = 0;
    int atomic_op = (sig_op == SHMEM_SIGNAL_ADD) ? FI_SUM : FI_ATOMIC_WRITE;

    SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);
    SHMEM_TRANSPORT_OFI_CNTR_INC(&ctx->pending_put_cntr);

    const struct fi_ioc msg_iov_signal = {
                                          .addr = (uint8_t *) &signal,
                                          .count = 1
                                        };
    const struct fi_rma_ioc rma_iov_signal = {
                                               .addr = (uint64_t) addr,
                                               .count = 1,
                                               .key = key
                                             };
    const struct fi_msg_atomic msg_signal = {
                                           .msg_iov = &msg_iov_signal,
                                           .desc = GET_MR_DESC_ADDR(shmem_transport_ofi_get_mr_desc_index((void *) &signal)),
                                           .iov_count = 1,
                                           .addr = GET_DEST(dst),
                                           .rma_iov = &rma_iov_signal,
                                           .rma_iov_count = 1,
                                           .datatype = FI_UINT64,
                                           .op = atomic_op,
                                           .context = (uint8_t *) &signal,
                                           .data = 0
                                         };

    do {
        ret = fi_atomicmsg(ctx->ep, &msg_signal, flags_signal);
    } while (try_again(ctx, ret, &polled));

    SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
}

/* compatibility with Portals transport */
static inline
void shmem_transport_put_wait(shmem_transport_ctx_t* ctx, long *completion) {

    shmem_internal_assert((*completion) >= 0);

    if((*completion) > 0) {
        shmem_transport_put_quiet(ctx);
        (*completion)--;
    }
}

static inline
void shmem_transport_put_nbi(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                             int pe)
{
    if (len <= shmem_transport_ofi_max_buffered_send) {

        shmem_transport_put_scalar(ctx, target, source, len, pe);

    } else {

        shmem_transport_ofi_put_large(ctx, target, source, len, pe);
    }
}


static inline
void shmem_transport_get(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len, int pe)
{
    int ret = 0;
    uint64_t dst = (uint64_t) pe;
    uint64_t polled = 0;
    uint64_t key;
    uint8_t *addr;

    shmem_transport_ofi_get_mr(source, pe, &addr, &key);

    SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);
    if (len <= shmem_transport_ofi_max_msg_size) {

        SHMEM_TRANSPORT_OFI_CNTR_INC(&ctx->pending_get_cntr);
        do {
            ret = fi_read(ctx->ep,
                          target,
                          len,
                          GET_MR_DESC(shmem_transport_ofi_get_mr_desc_index(target)),
                          GET_DEST(dst),
                          (uint64_t) addr,
                          key,
                          NULL);
        } while (try_again(ctx, ret, &polled));
    }
    else {
        uint8_t *frag_target = (uint8_t *) target;
        uint64_t frag_source = (uint64_t) addr;
        size_t frag_len = len;

        while (frag_target < ((uint8_t *) target) + len) {
            frag_len = MIN(shmem_transport_ofi_max_msg_size,
                           (size_t) (((uint8_t *) target) + len - frag_target));
            polled = 0;

            SHMEM_TRANSPORT_OFI_CNTR_INC(&ctx->pending_get_cntr);

            do {
                ret = fi_read(ctx->ep,
                              frag_target, frag_len,
                              GET_MR_DESC(shmem_transport_ofi_get_mr_desc_index(target)),
                              GET_DEST(dst), frag_source,
                              key, NULL);
            } while (try_again(ctx, ret, &polled));

            frag_source += frag_len;
            frag_target += frag_len;
        }
    }
    SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
}


static inline
void shmem_transport_get_wait(shmem_transport_ctx_t* ctx)
{
    /* wait for get counter to meet outstanding count value */

    /* Note: the communication routines increment pending get counters before
     * each FI call (thus before the corresponding event counter is
     * incremented), but the completion routine below reads the counters in the
     * reverse order: first the fid_cntr event counter, then the get issued
     * counter.  We'll want to preserve this property in the future.
     */
    uint64_t success, fail, cnt, cnt_new;
    long poll_count = 0;

    SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);

    while (poll_count < shmem_transport_ofi_get_poll_limit ||
           shmem_transport_ofi_get_poll_limit < 0) {
        success = fi_cntr_read(ctx->get_cntr);
        fail = fi_cntr_readerr(ctx->get_cntr);
        cnt = SHMEM_TRANSPORT_OFI_CNTR_READ(&ctx->pending_get_cntr);

        shmem_transport_probe();

        if (success < cnt && fail == 0) {
            SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
            SPINLOCK_BODY();
            SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);
        } else if (fail) {
            RAISE_ERROR_MSG("Operations completed in error (%" PRIu64 ")\n", fail);
        } else {
            SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
            return;
        }
        poll_count++;
    }
    cnt_new = SHMEM_TRANSPORT_OFI_CNTR_READ(&ctx->pending_get_cntr);
    do {
        cnt = cnt_new;
        ssize_t ret = fi_cntr_wait(ctx->get_cntr, cnt, -1);
        cnt_new = SHMEM_TRANSPORT_OFI_CNTR_READ(&ctx->pending_get_cntr);
        OFI_CTX_CHECK_ERROR(ctx, ret);
    } while (cnt < cnt_new);
    shmem_internal_assert(cnt == cnt_new);

    SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
}


static inline
void shmem_transport_cswap_nbi(shmem_transport_ctx_t* ctx, void *target, const
                               void *source, void *dest, const void *operand,
                               size_t len, int pe, int datatype)
{
    int ret = 0;
    uint64_t dst = (uint64_t) pe;
    uint64_t polled = 0;
    uint64_t key;
    uint8_t *addr;

    shmem_transport_ofi_get_mr(target, pe, &addr, &key);
    shmem_internal_assert(len <= sizeof(double _Complex));
    shmem_internal_assert(SHMEM_Dtsize[SHMEM_TRANSPORT_DTYPE(datatype)] == len);

    struct fi_ioc resultv = { .addr = dest, .count = 1 };
    const struct fi_ioc sourcev = { .addr = (void *) source, .count = 1 };
    const struct fi_ioc comparev = { .addr = (void *) operand, .count = 1 };
    const struct fi_rma_ioc rmav= { .addr = (uint64_t) addr, .count = 1, .key = key };
    const struct fi_msg_atomic msg = {
                                 .msg_iov       = &sourcev,
                                 .desc          = GET_MR_DESC_ADDR(shmem_transport_ofi_get_mr_desc_index(source)),
                                 .iov_count     = 1,
                                 .addr          = GET_DEST(dst),
                                 .rma_iov       = &rmav,
                                 .rma_iov_count = 1,
                                 .datatype      = SHMEM_TRANSPORT_DTYPE(datatype),
                                 .op            = FI_CSWAP,
                                 .context       = NULL,
                                 .data          = 0
                               };

    SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);
    SHMEM_TRANSPORT_OFI_CNTR_INC(&ctx->pending_get_cntr);

    do {
        ret = fi_compare_atomicmsg(ctx->ep,
                                   &msg,
                                   &comparev,
                                   NULL,
                                   1,
                                   &resultv,
                                   GET_MR_DESC_ADDR(shmem_transport_ofi_get_mr_desc_index(dest)),
                                   1,
                                   FI_INJECT);  /* FI_DELIVERY_COMPLETE is not required as 
                                                   it is implied for fetch atomicmsgs */
    } while (try_again(ctx, ret, &polled));
    SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
}


static inline
void shmem_transport_cswap(shmem_transport_ctx_t* ctx, void *target, const void *source, void *dest,
                           const void *operand, size_t len, int pe, int datatype)
{
#ifdef ENABLE_MR_ENDPOINT
    /* CXI provider currently does not support fetch atomics with FI_DELIVERY_COMPLETE
     * That is why non-blocking API is used which uses FI_INJECT. FI_ATOMIC_READ is
     * also not supported currently */
    shmem_transport_cswap_nbi(ctx, target, source,
                              dest, operand, len, pe, datatype);
#else
    int ret = 0;
    uint64_t dst = (uint64_t) pe;
    uint64_t polled = 0;
    uint64_t key;
    uint8_t *addr;

    shmem_transport_ofi_get_mr(target, pe, &addr, &key);

    shmem_internal_assert(len <= sizeof(double _Complex));
    shmem_internal_assert(SHMEM_Dtsize[SHMEM_TRANSPORT_DTYPE(datatype)] == len);

    SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);
    SHMEM_TRANSPORT_OFI_CNTR_INC(&ctx->pending_get_cntr);

    do {
        ret = fi_compare_atomic(ctx->ep,
                                source,
                                1,
                                GET_MR_DESC(shmem_transport_ofi_get_mr_desc_index(source)),
                                operand,
                                NULL,
                                dest,
                                GET_MR_DESC(shmem_transport_ofi_get_mr_desc_index(dest)),
                                GET_DEST(dst),
                                (uint64_t) addr,
                                key,
                                SHMEM_TRANSPORT_DTYPE(datatype),
                                FI_CSWAP,
                                NULL);
    } while (try_again(ctx, ret, &polled));
    SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
#endif
}


static inline
void shmem_transport_mswap(shmem_transport_ctx_t* ctx, void *target, const void *source, void *dest,
                           const void *mask, size_t len, int pe, int datatype)
{
    int ret = 0;
    uint64_t dst = (uint64_t) pe;
    uint64_t polled = 0;
    uint64_t key;
    uint8_t *addr;

    shmem_transport_ofi_get_mr(target, pe, &addr, &key);

    shmem_internal_assert(len <= sizeof(double _Complex));
    shmem_internal_assert(SHMEM_Dtsize[SHMEM_TRANSPORT_DTYPE(datatype)] == len);

    SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);
    SHMEM_TRANSPORT_OFI_CNTR_INC(&ctx->pending_get_cntr);

    do {
        ret = fi_compare_atomic(ctx->ep,
                                source,
                                1,
                                GET_MR_DESC(shmem_transport_ofi_get_mr_desc_index(source)),
                                mask,
                                NULL,
                                dest,
                                GET_MR_DESC(shmem_transport_ofi_get_mr_desc_index(dest)),
                                GET_DEST(dst),
                                (uint64_t) addr,
                                key,
                                SHMEM_TRANSPORT_DTYPE(datatype),
                                FI_MSWAP,
                                NULL);
    } while (try_again(ctx, ret, &polled));
    SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
}


static inline
void shmem_transport_atomic(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                            int pe, int op, int datatype)
{
    int ret = 0;
    uint64_t dst = (uint64_t) pe;
    uint64_t polled = 0;
    uint64_t key;
    uint8_t *addr;

    shmem_transport_ofi_get_mr(target, pe, &addr, &key);

    shmem_internal_assert(SHMEM_Dtsize[SHMEM_TRANSPORT_DTYPE(datatype)] == len);

    SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);
    SHMEM_TRANSPORT_OFI_CNTR_INC(&ctx->pending_put_cntr);

    do {
        ret = fi_inject_atomic(ctx->ep,
                               source,
                               1,
                               GET_DEST(dst),
                               (uint64_t) addr,
                               key,
                               SHMEM_TRANSPORT_DTYPE(datatype),
                               op);
    } while (try_again(ctx, ret, &polled));
    SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
}


static inline
void shmem_transport_atomicv(shmem_transport_ctx_t* ctx, void *target, const void *source,
                             size_t count, size_t type_size, int pe, int op, int datatype,
                             long *completion)
{
    size_t full_len = count*type_size;
    int ret = 0;
    uint64_t dst = (uint64_t) pe;
    int dt = SHMEM_TRANSPORT_DTYPE(datatype);
    size_t len = full_len/SHMEM_Dtsize[dt];
    uint64_t polled = 0;
    uint64_t key;
    uint8_t *addr;
    size_t max_atomic_size = 0;

    shmem_internal_assert(SHMEM_Dtsize[dt] * len == full_len);

    SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);
    ret = fi_atomicvalid(ctx->ep, dt, op,
                         &max_atomic_size);
    max_atomic_size = max_atomic_size * SHMEM_Dtsize[dt];
    if (max_atomic_size > shmem_transport_ofi_max_msg_size
        || ret || max_atomic_size == 0) {
        SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
        RAISE_ERROR_MSG("Atomic operation with datatype %d and op %d not supported\n",
                        datatype, op);
    }

    shmem_transport_ofi_get_mr(target, pe, &addr, &key);

    if ( full_len <= MIN(shmem_transport_ofi_max_buffered_send,
                         max_atomic_size)) {

        polled = 0;

        SHMEM_TRANSPORT_OFI_CNTR_INC(&ctx->pending_put_cntr);

        do {
            ret = fi_inject_atomic(ctx->ep,
                                   source,
                                   len,
                                   GET_DEST(dst),
                                   (uint64_t) addr,
                                   key,
                                   dt,
                                   op);
        } while (try_again(ctx, ret, &polled));

    } else if (full_len <=
               MIN(shmem_transport_ofi_bounce_buffer_size, max_atomic_size) &&
               ctx->bounce_buffers) {

        shmem_transport_ofi_bounce_buffer_t *buff =
            create_bounce_buffer(ctx, source, full_len);

        polled = 0;
        SHMEM_TRANSPORT_OFI_CNTR_INC(&ctx->pending_put_cntr);

        const struct fi_ioc        msg_iov = { .addr = buff->data, .count = len };
        const struct fi_rma_ioc    rma_iov = { .addr = (uint64_t) addr, .count = len, .key = key };
        const struct fi_msg_atomic msg     = {
                                               .msg_iov       = &msg_iov,
                                               .desc          = GET_MR_DESC_ADDR(shmem_transport_ofi_get_mr_desc_index(source)),
                                               .iov_count     = 1,
                                               .addr          = GET_DEST(dst),
                                               .rma_iov       = &rma_iov,
                                               .rma_iov_count = 1,
                                               .datatype      = dt,
                                               .op            = op,
                                               .context       = buff,
                                               .data          = 0
                                             };
        do {
            ret = fi_atomicmsg(ctx->ep, &msg, FI_COMPLETION | FI_DELIVERY_COMPLETE);
        } while (try_again(ctx, ret, &polled));

    } else {
        size_t sent = 0;

        while (sent < len) {

            size_t chunksize = MIN((len-sent),
                                   (max_atomic_size/SHMEM_Dtsize[dt]));
            polled = 0;
            SHMEM_TRANSPORT_OFI_CNTR_INC(&ctx->pending_put_cntr);
            do {
                ret = fi_atomic(ctx->ep,
                                (void *)((char *)source +
                                         (sent*SHMEM_Dtsize[dt])),
                                chunksize,
                                GET_MR_DESC(shmem_transport_ofi_get_mr_desc_index(source)),
                                GET_DEST(dst),
                                ((uint64_t) addr +
                                 (sent*SHMEM_Dtsize[dt])),
                                key,
                                dt,
                                op,
                                NULL);
            } while (try_again(ctx, ret, &polled));

            sent += chunksize;
        }
    }
    SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
}


/* Note: Both non-NBI and NBI versions of fetching atomic routines are
 * nonblocking.  The NBI routines buffer (i.e. inject) the source argument and
 * the non-NBI operations do not. */
static inline
void shmem_transport_fetch_atomic_nbi(shmem_transport_ctx_t* ctx, void *target,
                                      const void *source, void *dest,
                                      size_t len, int pe, int op, int datatype)
{
    int ret = 0;
    uint64_t dst = (uint64_t) pe;
    uint64_t polled = 0;
    uint64_t key;
    uint8_t *addr;

    shmem_transport_ofi_get_mr(target, pe, &addr, &key);
    shmem_internal_assert(len <= sizeof(double _Complex));
    shmem_internal_assert(SHMEM_Dtsize[SHMEM_TRANSPORT_DTYPE(datatype)] == len);

    struct fi_ioc resultv = { .addr = dest, .count = 1 };
    const struct fi_ioc sourcev = { .addr = (void *) source, .count = 1 };
    const struct fi_rma_ioc rmav= { .addr = (uint64_t) addr, .count = 1, .key = key };
    const struct fi_msg_atomic msg = {
                                 .msg_iov       = &sourcev,
                                 .desc          = GET_MR_DESC_ADDR(shmem_transport_ofi_get_mr_desc_index(source)),
                                 .iov_count     = 1,
                                 .addr          = GET_DEST(dst),
                                 .rma_iov       = &rmav,
                                 .rma_iov_count = 1,
                                 .datatype      = SHMEM_TRANSPORT_DTYPE(datatype),
                                 .op            = op,
                                 .context       = NULL,
                                 .data          = 0
                               };

    SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);
    SHMEM_TRANSPORT_OFI_CNTR_INC(&ctx->pending_get_cntr);

    do {
        ret = fi_fetch_atomicmsg(ctx->ep,
                                 &msg,
                                 &resultv,
                                 GET_MR_DESC_ADDR(shmem_transport_ofi_get_mr_desc_index(dest)),
                                 1,
                                 FI_INJECT); /* FI_DELIVERY_COMPLETE is not required as it's
                                                implied for fetch atomicmsgs */
    } while (try_again(ctx, ret, &polled));
    SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
}


static inline
void shmem_transport_fetch_atomic(shmem_transport_ctx_t* ctx, void *target,
                                  const void *source, void *dest,
                                  size_t len, int pe, int op, int datatype)
{
#ifdef ENABLE_MR_ENDPOINT
    /* CXI provider currently does not support fetch atomics with FI_DELIVERY_COMPLETE
     * That is why non-blocking API is used which uses FI_INJECT. FI_ATOMIC_READ is
     * also not supported currently */
    shmem_transport_fetch_atomic_nbi(ctx, target, source,
                                     dest, len, pe, op, datatype);
#else
    int ret = 0;
    uint64_t dst = (uint64_t) pe;
    uint64_t polled = 0;
    uint64_t key;
    uint8_t *addr;

    shmem_transport_ofi_get_mr(target, pe, &addr, &key);

    shmem_internal_assert(len <= sizeof(double _Complex));
    shmem_internal_assert(SHMEM_Dtsize[SHMEM_TRANSPORT_DTYPE(datatype)] == len);

    SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);
    SHMEM_TRANSPORT_OFI_CNTR_INC(&ctx->pending_get_cntr);

    do {
        ret = fi_fetch_atomic(ctx->ep,
                              source,
                              1,
                              GET_MR_DESC(shmem_transport_ofi_get_mr_desc_index(source)),
                              dest,
                              GET_MR_DESC(shmem_transport_ofi_get_mr_desc_index(dest)),
                              GET_DEST(dst),
                              (uint64_t) addr,
                              key,
                              SHMEM_TRANSPORT_DTYPE(datatype),
                              op,
                              NULL);
    } while (try_again(ctx, ret, &polled));
    SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
#endif
}


static inline
void shmem_transport_swap(shmem_transport_ctx_t* ctx, void *target,
                          const void *source, void *dest,
                          size_t len, int pe, int datatype)
{
    shmem_transport_fetch_atomic(ctx, target, source, dest, len, pe,
                                 FI_ATOMIC_WRITE, datatype);
}


static inline
void shmem_transport_swap_nbi(shmem_transport_ctx_t* ctx, void *target,
                              const void *source, void *dest, size_t len,
                              int pe, int datatype)
{
    shmem_transport_fetch_atomic_nbi(ctx, target, source, dest, len, pe,
                                     FI_ATOMIC_WRITE, datatype);
}


static inline
void shmem_transport_atomic_set(shmem_transport_ctx_t* ctx, void *target,
                                const void *source, size_t len, int pe,
                                int datatype)
{
    shmem_transport_atomic(ctx, target, source, len, pe, FI_ATOMIC_WRITE,
                           datatype);
}


static inline
void shmem_transport_atomic_fetch(shmem_transport_ctx_t* ctx, void *target,
                                  const void *source, size_t len, int pe,
                                  int datatype)
{
#ifdef ENABLE_MR_ENDPOINT
    /* CXI provider currently does not support fetch atomics with FI_DELIVERY_COMPLETE 
     * That is why non-blocking API is used which uses FI_INJECT. FI_ATOMIC_READ is 
     * also not supported currently */
    long long dummy = 0;
    shmem_transport_fetch_atomic_nbi(ctx, (void *) source, (const void *) &dummy,
                                     target, len, pe, FI_SUM, datatype);
#else
    shmem_transport_fetch_atomic_nbi(ctx, (void *) source, (const void *) NULL,
                                     target, len, pe, FI_ATOMIC_READ, datatype);
#endif
}


/* Query transport layer to detemine if the given combination of <op, datatype>
 * is supported as a one-sided atomic operation.  This is used by reductions to
 * check whether to fall back to software reductions for things like double
 * complex product reductions. */
static inline
int shmem_transport_atomic_supported(shm_internal_op_t op,
                                     shm_internal_datatype_t datatype)
{
#ifdef USE_SHR_ATOMICS
    /* FIXME: Force shared memory atomics build to use software reductions */
    return 0;
#else
    size_t size = 0;

    /* NOTE-MT: It's not clear from the OFI documentation whether this mutex is
     * actually required by FI_THREAD_COMPLETION. */

    SHMEM_TRANSPORT_OFI_CTX_LOCK(&shmem_transport_ctx_default);
    int ret = fi_atomicvalid(shmem_transport_ctx_default.ep,
                             SHMEM_TRANSPORT_DTYPE(datatype),
                             op, &size);
    SHMEM_TRANSPORT_OFI_CTX_UNLOCK(&shmem_transport_ctx_default);

    return !(ret != 0 || size == 0);
#endif
}


static inline
void shmem_transport_put_ct_nb(shmem_transport_ct_t *ct, void *target,
                               const void *source, size_t len, int pe,
                               long *completion)
{
    RAISE_ERROR_STR("OFI transport does not currently support CT operations");
}

static inline
void shmem_transport_get_ct(shmem_transport_ct_t *ct, void *target,
                            const void *source, size_t len, int pe)
{
    RAISE_ERROR_STR("OFI transport does not currently support CT operations");
}

static inline
void shmem_transport_ct_create(shmem_transport_ct_t **ct_ptr)
{
    RAISE_ERROR_STR("OFI transport does not currently support CT operations");
}

static inline
void shmem_transport_ct_free(shmem_transport_ct_t **ct_ptr)
{
    RAISE_ERROR_STR("OFI transport does not currently support CT operations");
}

static inline
long shmem_transport_ct_get(shmem_transport_ct_t *ct)
{
    RAISE_ERROR_STR("OFI transport does not currently support CT operations");
    return -1;
}

static inline
void shmem_transport_ct_set(shmem_transport_ct_t *ct, long value)
{
    RAISE_ERROR_STR("OFI transport does not currently support CT operations");
}

static inline
void shmem_transport_ct_wait(shmem_transport_ct_t *ct, long wait_for)
{
    RAISE_ERROR_STR("OFI transport does not currently support CT operations");
}

static inline
uint64_t shmem_transport_received_cntr_get(void)
{
#ifndef ENABLE_HARD_POLLING
    shmem_internal_assert(shmem_internal_thread_level == SHMEM_THREAD_SINGLE);
    /* NOTE-MT: This is only reachable in single-threaded runs, otherwise
     * we would need a mutex to support FI_THREAD_COMPLETION builds. */
    return fi_cntr_read(shmem_transport_ofi_target_cntrfd);
#else
    RAISE_ERROR_STR("OFI transport configured for hard polling");
    return 0;
#endif
}

static inline
void shmem_transport_received_cntr_wait(uint64_t ge_val)
{
#ifndef ENABLE_HARD_POLLING
    shmem_internal_assert(shmem_internal_thread_level == SHMEM_THREAD_SINGLE);
    /* NOTE-MT: This is only reachable in single-threaded runs, otherwise
     * we would need a mutex to support FI_THREAD_COMPLETION builds. */
    int ret = fi_cntr_wait(shmem_transport_ofi_target_cntrfd, ge_val, -1);

    OFI_CHECK_ERROR(ret);
#else
    RAISE_ERROR_STR("OFI transport configured for hard polling");
#endif
}

static inline
void shmem_transport_syncmem(void)
{
    /* TODO: libfabric does not yet have an analog to PtlAtomicSync() in
     * Portals4, so the OFI transport routine will be a nop until an API is
     * provided.
     */
}

static inline
uint64_t shmem_transport_pcntr_get_issued_write(shmem_transport_ctx_t *ctx)
{
    uint64_t cnt;
    SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);
    cnt = SHMEM_TRANSPORT_OFI_CNTR_READ(&ctx->pending_put_cntr);
    SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);

    if (ctx->options & SHMEMX_CTX_BOUNCE_BUFFER) {
        SHMEM_TRANSPORT_OFI_CTX_BB_LOCK(ctx);
        cnt += ctx->pending_bb_cntr;
        SHMEM_TRANSPORT_OFI_CTX_BB_UNLOCK(ctx);
    }
    return cnt;
}

static inline
uint64_t shmem_transport_pcntr_get_issued_read(shmem_transport_ctx_t *ctx)
{
    uint64_t cnt;
    SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);
    cnt = SHMEM_TRANSPORT_OFI_CNTR_READ(&ctx->pending_get_cntr);
    SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
    return cnt;
}

static inline
uint64_t shmem_transport_pcntr_get_completed_write(shmem_transport_ctx_t *ctx)
{
    uint64_t cnt;
    SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);
    cnt = fi_cntr_read(ctx->put_cntr);
    SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);

    if (ctx->options & SHMEMX_CTX_BOUNCE_BUFFER) {
        SHMEM_TRANSPORT_OFI_CTX_BB_LOCK(ctx);
        cnt += ctx->completed_bb_cntr;
        SHMEM_TRANSPORT_OFI_CTX_BB_UNLOCK(ctx);
    }
    return cnt;
}

static inline
uint64_t shmem_transport_pcntr_get_completed_read(shmem_transport_ctx_t *ctx)
{
    uint64_t cnt;
    SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);
    cnt = fi_cntr_read(ctx->get_cntr);
    SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
    return cnt;
}

static inline
uint64_t shmem_transport_pcntr_get_completed_target(void)
{
    uint64_t cnt = 0;
#if ENABLE_TARGET_CNTR
#  ifdef USE_THREAD_COMPLETION
    if (0 == pthread_mutex_lock(&shmem_transport_ofi_progress_lock)) {
#  endif
        cnt = fi_cntr_read(shmem_transport_ofi_target_cntrfd);
#  ifdef USE_THREAD_COMPLETION
        pthread_mutex_unlock(&shmem_transport_ofi_progress_lock);
    }
#  endif
#else
    cnt = 0;
#endif
    return cnt;
}

static inline
void shmem_transport_pcntr_get_all(shmem_transport_ctx_t *ctx, shmemx_pcntr_t *pcntr)
{
    SHMEM_TRANSPORT_OFI_CTX_LOCK(ctx);
    pcntr->completed_put = 0;
    pcntr->pending_put = 0;

    if (ctx->options & SHMEMX_CTX_BOUNCE_BUFFER) {
        SHMEM_TRANSPORT_OFI_CTX_BB_LOCK(ctx);
        pcntr->completed_put = ctx->completed_bb_cntr;
        pcntr->pending_put = ctx->pending_bb_cntr;
        SHMEM_TRANSPORT_OFI_CTX_BB_UNLOCK(ctx);
    }
    pcntr->completed_put += fi_cntr_read(ctx->put_cntr);
    pcntr->completed_get = fi_cntr_read(ctx->get_cntr);

    pcntr->pending_put += SHMEM_TRANSPORT_OFI_CNTR_READ(&ctx->pending_put_cntr);
    pcntr->pending_get = SHMEM_TRANSPORT_OFI_CNTR_READ(&ctx->pending_get_cntr);

    SHMEM_TRANSPORT_OFI_CTX_UNLOCK(ctx);
    pcntr->target = shmem_transport_pcntr_get_completed_target();
}

#endif /* TRANSPORT_OFI_H */
