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

#ifndef TRANSPORT_PORTALS_H
#define TRANSPORT_PORTALS_H

#include <portals4.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include "shmem_free_list.h"
#include "shmem_internal.h"
#include "shmem_atomic.h"
#include "shmem_team.h"

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

extern int shmem_transport_dtype_table[];
#define SHMEM_TRANSPORT_DTYPE(DTYPE) shmem_transport_dtype_table[(DTYPE)]

typedef ptl_op_t shm_internal_op_t;

#define SHM_INTERNAL_BAND PTL_BAND
#define SHM_INTERNAL_BOR PTL_BOR
#define SHM_INTERNAL_BXOR PTL_BXOR
#define SHM_INTERNAL_MIN PTL_MIN
#define SHM_INTERNAL_MAX PTL_MAX
#define SHM_INTERNAL_SUM PTL_SUM
#define SHM_INTERNAL_PROD PTL_PROD

#define SHMEM_TRANSPORT_PORTALS4_NUM_PTS 32

/* NOTE: If these values change, the shmem_transport_portals4_pt_state[]
 * initialization in transport_portals4.c must also be updated. */
#ifdef ENABLE_REMOTE_VIRTUAL_ADDRESSING
#define shmem_transport_portals4_pt      8
#else
#define shmem_transport_portals4_data_pt 8
#define shmem_transport_portals4_heap_pt 9
#endif

extern int8_t shmem_transport_portals4_pt_state[SHMEM_TRANSPORT_PORTALS4_NUM_PTS];

extern ptl_handle_ni_t shmem_transport_portals4_ni_h;
extern ptl_handle_md_t shmem_transport_portals4_put_event_md_h;
extern ptl_handle_ct_t shmem_transport_portals4_put_event_ct_h;
extern shmem_internal_cntr_t shmem_transport_portals4_pending_put_event_cntr;
#ifndef ENABLE_HARD_POLLING
extern ptl_handle_ct_t shmem_transport_portals4_target_ct_h;
#endif
extern ptl_handle_eq_t shmem_transport_portals4_eq_h;

extern shmem_free_list_t *shmem_transport_portals4_bounce_buffers;
extern shmem_free_list_t *shmem_transport_portals4_long_frags;

extern ptl_size_t shmem_transport_portals4_bounce_buffer_size;
extern ptl_size_t shmem_transport_portals4_max_volatile_size;
extern ptl_size_t shmem_transport_portals4_max_atomic_size;
extern ptl_size_t shmem_transport_portals4_max_fetch_atomic_size;
extern ptl_size_t shmem_transport_portals4_max_fence_size;
extern ptl_size_t shmem_transport_portals4_max_msg_size;

extern int32_t shmem_transport_portals4_event_slots;

#if WANT_TOTAL_DATA_ORDERING != 0
extern int shmem_transport_portals4_total_data_ordering;
extern int shmem_transport_portals4_long_pending;
#endif

#ifdef ENABLE_THREADS
extern shmem_internal_mutex_t shmem_internal_mutex_ptl4_ctx;
extern shmem_internal_mutex_t shmem_internal_mutex_ptl4_pt_state;
extern shmem_internal_mutex_t shmem_internal_mutex_ptl4_frag;
extern shmem_internal_mutex_t shmem_internal_mutex_ptl4_event_slots;
#endif

#define SHMEM_TRANSPORT_PORTALS4_TYPE_BOUNCE  0x01
#define SHMEM_TRANSPORT_PORTALS4_TYPE_LONG    0x02

enum shmem_transport_portals4_pt_states_t {
    PT_FREE = 0, PT_ALLOCATED = 1, PT_RESERVED = 2
};

struct shmem_transport_portals4_frag_t {
    shmem_free_list_item_t item;
    char type;
};
typedef struct shmem_transport_portals4_frag_t shmem_transport_portals4_frag_t;

struct shmem_transport_portals4_bounce_buffer_t {
    shmem_transport_portals4_frag_t frag;
    char data[];
};
typedef struct shmem_transport_portals4_bounce_buffer_t shmem_transport_portals4_bounce_buffer_t;

struct shmem_transport_portals4_long_frag_t {
    shmem_transport_portals4_frag_t frag;
    int reference;
    long *completion;
};
typedef struct shmem_transport_portals4_long_frag_t shmem_transport_portals4_long_frag_t;

struct shmem_transport_ct_t {
#ifdef ENABLE_REMOTE_VIRTUAL_ADDRESSING
    ptl_pt_index_t shr_pt;
    ptl_handle_le_t shr_le;
#else
    ptl_pt_index_t data_pt, heap_pt;
    ptl_handle_le_t data_le, heap_le;
#endif
    ptl_handle_ct_t ct;
};
typedef struct shmem_transport_ct_t shmem_transport_ct_t;

struct shmem_transport_ctx_t {
    int id;
    long options;
    ptl_handle_md_t put_volatile_md;
    ptl_handle_md_t put_md;
    ptl_handle_md_t get_md;
    ptl_handle_ct_t put_ct;
    ptl_handle_ct_t get_ct;

    /* NOTE-MT: Pending counters must be incremented before ops are issued to avoid
     * a race between the pending counter increment and corresponding counting
     * event arrival.  This race can cause early exit from quiet. */
    shmem_internal_cntr_t pending_put_cntr;
    shmem_internal_cntr_t pending_get_cntr;
    struct shmem_internal_team_t   *team;
};

typedef struct shmem_transport_ctx_t shmem_transport_ctx_t;
extern shmem_transport_ctx_t shmem_transport_ctx_default;
int shmem_transport_ctx_create(struct shmem_internal_team_t *team, long options, shmem_transport_ctx_t **ctx);
void shmem_transport_ctx_destroy(shmem_transport_ctx_t *ctx);

int shmem_transport_session_start(shmem_transport_ctx_t *ctx, long options, const shmem_session_config_t *config, long config_mask);
int shmem_transport_session_stop(shmem_transport_ctx_t *ctx);

/*
 * PORTALS4_GET_REMOTE_ACCESS is used to get the correct PT and offset
 * from the base of the list entry on that PT for a given target
 * virtual address.  In the common case, there are two list entries
 * (on different PTs), one for the data region and one for the
 * symmetric heap.  On platforms where we can reliably assume the
 * symmetric heap is actually symmetric relative to virtual addresses
 * and the implementation supports BIND_INACCESSIBLE, we can use only
 * one LE and use the virtual address ass the offset.
*/
#ifdef ENABLE_ERROR_CHECKING
#ifdef ENABLE_REMOTE_VIRTUAL_ADDRESSING
#define PORTALS4_GET_REMOTE_ACCESS_ONEPT(target, pt, offset, shr_pt)    \
    do {                                                                \
        if (((void*) target > shmem_internal_data_base) &&              \
            ((char*) target < (char*) shmem_internal_data_base + shmem_internal_data_length)) { \
        } else if (((void*) target > shmem_internal_heap_base) &&       \
                   ((char*) target < (char*) shmem_internal_heap_base + shmem_internal_heap_length)) { \
        } else {                                                        \
            offset = 0;                                                 \
            pt = -1;                                                    \
            RAISE_ERROR_MSG("target (0x%"PRIXPTR") outside of symmetric areas\n", \
                            (uintptr_t) target);                        \
        }                                                               \
        pt = shr_pt;                                                    \
        offset = (uintptr_t) target;                                    \
    } while (0)
#else
#define PORTALS4_GET_REMOTE_ACCESS_TWOPT(target, pt, offset, data_pt, heap_pt) \
    do {                                                                \
        if (((void*) target > shmem_internal_data_base) &&              \
            ((char*) target < (char*) shmem_internal_data_base + shmem_internal_data_length)) { \
            pt = (data_pt);                                             \
            offset = (char*) target - (char*) shmem_internal_data_base; \
        } else if (((void*) target > shmem_internal_heap_base) &&       \
                   ((char*) target < (char*) shmem_internal_heap_base + shmem_internal_heap_length)) { \
            pt = (heap_pt);                                             \
            offset = (char*) target - (char*) shmem_internal_heap_base; \
        } else {                                                        \
            offset = 0;                                                 \
            pt = -1;                                                    \
            RAISE_ERROR_MSG("target (0x%"PRIXPTR") outside of symmetric areas\n", \
                            (uintptr_t) target);                        \
        }                                                               \
    } while (0)
#endif
#else
#ifdef ENABLE_REMOTE_VIRTUAL_ADDRESSING
#define PORTALS4_GET_REMOTE_ACCESS_ONEPT(target, pt, offset, shr_pt)    \
    do {                                                                \
        pt = (shr_pt);                                                  \
        offset = (uintptr_t) target;                                    \
    } while (0)
#else
#define PORTALS4_GET_REMOTE_ACCESS_TWOPT(target, pt, offset, data_pt, heap_pt) \
    do {                                                                \
        if ((void*) target < shmem_internal_heap_base) {                \
            pt = (data_pt);                                             \
            offset = (char*) target - (char*) shmem_internal_data_base; \
        } else {                                                        \
            pt = (heap_pt);                                             \
            offset = (char*) target - (char*) shmem_internal_heap_base; \
        }                                                               \
    } while (0)
#endif
#endif

#ifdef ENABLE_REMOTE_VIRTUAL_ADDRESSING
#define PORTALS4_GET_REMOTE_ACCESS(target, pt, offset)                  \
        PORTALS4_GET_REMOTE_ACCESS_ONEPT(target, pt, offset,            \
                                         shmem_transport_portals4_pt)
#else
#define PORTALS4_GET_REMOTE_ACCESS(target, pt, offset)                  \
        PORTALS4_GET_REMOTE_ACCESS_TWOPT(target, pt, offset,            \
                                    shmem_transport_portals4_data_pt,   \
                                    shmem_transport_portals4_heap_pt)
#endif

#if WANT_TOTAL_DATA_ORDERING == 0
#define PORTALS4_TOTAL_DATA_ORDERING 0
#elif WANT_TOTAL_DATA_ORDERING == 1
#define PORTALS4_TOTAL_DATA_ORDERING 1
#else
#define PORTALS4_TOTAL_DATA_ORDERING shmem_transport_portals4_total_data_ordering
#endif

int shmem_transport_init(void);

int shmem_transport_startup(void);

int shmem_transport_fini(void);

static inline void shmem_transport_get_wait(shmem_transport_ctx_t*);

static inline void shmem_transport_probe(void) {
    return;
}

static inline
int
shmem_transport_quiet(shmem_transport_ctx_t* ctx)
{
    int ret;
    ptl_ct_event_t ct;
    uint64_t cnt, cnt_new;

    /* wait for completion of all pending NB get events */
    shmem_transport_get_wait(ctx);

    /* wait for remote completion (acks) of all buffered puts */
    /* NOTE-MT: continue to wait if additional operations are issued during the quiet */
    cnt_new = shmem_internal_cntr_read(&shmem_transport_portals4_pending_put_event_cntr);
    do {
        cnt = cnt_new;
        ret = PtlCTWait(shmem_transport_portals4_put_event_ct_h, cnt, &ct);
        cnt_new = shmem_internal_cntr_read(&shmem_transport_portals4_pending_put_event_cntr);
        if (PTL_OK != ret) { return ret; }
        if (ct.failure != 0) {
            RETURN_ERROR_MSG("buffered put operations failed, %" PRIu64 "\n", ct.failure);
            return -1;
        }
    } while (cnt < cnt_new);
    shmem_internal_assert(cnt == cnt_new);

    /* wait for remote completion (acks) of all pending put events */
    /* NOTE-MT: continue to wait if additional operations are issued during the quiet */
    cnt_new = shmem_internal_cntr_read(&ctx->pending_put_cntr);
    do {
        cnt = cnt_new;
        ret = PtlCTWait(ctx->put_ct, cnt, &ct);
        cnt_new = shmem_internal_cntr_read(&ctx->pending_put_cntr);
        if (PTL_OK != ret) { return ret; }
        if (ct.failure != 0) {
            RETURN_ERROR_MSG("put operations failed, %" PRIu64 "\n", ct.failure);
            return -1;
        }
    } while (cnt < cnt_new);
    shmem_internal_assert(cnt == cnt_new);

    return 0;
}


static inline
int
shmem_transport_fence(shmem_transport_ctx_t* ctx)
{
    int ret = 0;

    if (0 == PORTALS4_TOTAL_DATA_ORDERING) {
        ret = shmem_transport_quiet(ctx);
    }
#if WANT_TOTAL_DATA_ORDERING != 0
    else if (0 != shmem_transport_portals4_long_pending) {
        ret = shmem_transport_quiet(ctx);
        shmem_transport_portals4_long_pending = 0;
    }
#endif

    return ret;
}

static inline
void
shmem_transport_portals4_drain_eq(void)
{
    int ret;
    ptl_event_t ev;

    /* NOTE-MT: Assume that ptl4_event_slots mutex is already held.  Release the
     * mutex before blocking on the EQ, then reqacuire before proceeding. */
    SHMEM_MUTEX_UNLOCK(shmem_internal_mutex_ptl4_event_slots);
    ret = PtlEQWait(shmem_transport_portals4_eq_h, &ev);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
    if (ev.ni_fail_type != PTL_OK) { RAISE_ERROR(ev.ni_fail_type); }
    SHMEM_MUTEX_LOCK(shmem_internal_mutex_ptl4_event_slots);

    /* The only event type we should see on a success is a send event */
    shmem_internal_assert(ev.type == PTL_EVENT_SEND);

    shmem_transport_portals4_event_slots++;

    shmem_transport_portals4_frag_t *frag =
         (shmem_transport_portals4_frag_t*) ev.user_ptr;

    /* NOTE-MT: A different thread may have created this frag, so we need a
     * memory barrier here before accessing any of the SHMEM-related fields in
     * the frag.  Right now that comes from the mutex. */
    SHMEM_MUTEX_LOCK(shmem_internal_mutex_ptl4_frag);
    if (SHMEM_TRANSPORT_PORTALS4_TYPE_BOUNCE == frag->type) {
         /* it's a short send completing */
         SHMEM_MUTEX_UNLOCK(shmem_internal_mutex_ptl4_frag);
         shmem_free_list_lock(shmem_transport_portals4_bounce_buffers);
         shmem_free_list_free(shmem_transport_portals4_bounce_buffers,
                              frag);
         shmem_free_list_unlock(shmem_transport_portals4_bounce_buffers);
    } else {
         /* it's one of the long messages we're waiting for */
         shmem_transport_portals4_long_frag_t *long_frag =
              (shmem_transport_portals4_long_frag_t*) frag;

         (*(long_frag->completion))--;
         if (0 >= --long_frag->reference) {
              long_frag->reference = 0;
              SHMEM_MUTEX_UNLOCK(shmem_internal_mutex_ptl4_frag);
              shmem_free_list_lock(shmem_transport_portals4_long_frags);
              shmem_free_list_free(shmem_transport_portals4_long_frags,
                                   frag);
              shmem_free_list_unlock(shmem_transport_portals4_long_frags);
         } else {
              SHMEM_MUTEX_UNLOCK(shmem_internal_mutex_ptl4_frag);
         }
    }
}


static inline
void
shmem_transport_put_scalar(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len, int pe)
{
    int ret;
    ptl_process_t peer;
    ptl_pt_index_t pt;
    long offset;

    peer.rank = pe;
    PORTALS4_GET_REMOTE_ACCESS(target, pt, offset);

    shmem_internal_assert(len <= shmem_transport_portals4_max_volatile_size);

    shmem_internal_cntr_inc(&ctx->pending_put_cntr);

    ret = PtlPut(ctx->put_volatile_md,
                 (ptl_size_t) source,
                 len,
                 PTL_OC_ACK_REQ,
                 peer,
                 pt,
                 0,
                 offset,
                 NULL,
                 0);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
}


static inline
void
shmem_transport_portals4_put_nb_internal(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                                int pe, long *completion, ptl_pt_index_t data_pt,
                                ptl_pt_index_t heap_pt)
{
    int ret;
    ptl_process_t peer;
    ptl_pt_index_t pt;
    long offset;

    peer.rank = pe;
#ifdef ENABLE_REMOTE_VIRTUAL_ADDRESSING
    PORTALS4_GET_REMOTE_ACCESS_ONEPT(target, pt, offset, data_pt);
#else
    PORTALS4_GET_REMOTE_ACCESS_TWOPT(target, pt, offset, data_pt, heap_pt);
#endif

    if (len <= shmem_transport_portals4_max_volatile_size) {
        shmem_internal_cntr_inc(&ctx->pending_put_cntr);
        ret = PtlPut(ctx->put_volatile_md,
                     (ptl_size_t) source,
                     len,
                     PTL_OC_ACK_REQ,
                     peer,
                     pt,
                     0,
                     offset,
                     NULL,
                     0);
        if (PTL_OK != ret) { RAISE_ERROR(ret); }

    } else if (len <= shmem_transport_portals4_bounce_buffer_size &&
               ctx->options & SHMEMX_CTX_BOUNCE_BUFFER) {
        shmem_transport_portals4_bounce_buffer_t *buff;

        SHMEM_MUTEX_LOCK(shmem_internal_mutex_ptl4_event_slots);
        while (0 >= --shmem_transport_portals4_event_slots) {
            shmem_transport_portals4_event_slots++;
            shmem_transport_portals4_drain_eq();
        }
        SHMEM_MUTEX_UNLOCK(shmem_internal_mutex_ptl4_event_slots);

        shmem_free_list_lock(shmem_transport_portals4_bounce_buffers);
        buff = (shmem_transport_portals4_bounce_buffer_t*)
            shmem_free_list_alloc(shmem_transport_portals4_bounce_buffers);
        shmem_free_list_unlock(shmem_transport_portals4_bounce_buffers);
        if (NULL == buff) RAISE_ERROR(-1);

        shmem_internal_assert(buff->frag.type == SHMEM_TRANSPORT_PORTALS4_TYPE_BOUNCE);

        memcpy(buff->data, source, len);

        shmem_internal_cntr_inc(&shmem_transport_portals4_pending_put_event_cntr);
        ret = PtlPut(shmem_transport_portals4_put_event_md_h,
                     (ptl_size_t) buff->data,
                     len,
                     PTL_OC_ACK_REQ,
                     peer,
                     pt,
                     0,
                     offset,
                     buff,
                     0);
        if (PTL_OK != ret) { RAISE_ERROR(ret); }

#if WANT_TOTAL_DATA_ORDERING != 0
        shmem_transport_portals4_long_pending = 1;
#endif
    } else {
        shmem_transport_portals4_long_frag_t *long_frag;

        shmem_internal_assert(len <= shmem_transport_portals4_max_msg_size);

        /* User requested completion notification, create a frag object and
         * append the completion pointer */

        SHMEM_MUTEX_LOCK(shmem_internal_mutex_ptl4_event_slots);
        while (0 >= --shmem_transport_portals4_event_slots) {
            shmem_transport_portals4_event_slots++;
            shmem_transport_portals4_drain_eq();
        }
        SHMEM_MUTEX_UNLOCK(shmem_internal_mutex_ptl4_event_slots);

        shmem_free_list_lock(shmem_transport_portals4_long_frags);
        long_frag = (shmem_transport_portals4_long_frag_t*)
            shmem_free_list_alloc(shmem_transport_portals4_long_frags);
        shmem_free_list_unlock(shmem_transport_portals4_long_frags);
        if (NULL == long_frag) { RAISE_ERROR(-1); }

        shmem_internal_assert(long_frag->frag.type == SHMEM_TRANSPORT_PORTALS4_TYPE_LONG);
        shmem_internal_assert(long_frag->reference == 0);
        long_frag->completion = completion;

        /* NOTE-MT: Frag mutex is not needed here because the frag doesn't get
         * exposed to other threads until the PtlPut. */
        (*(long_frag->completion))++;
        long_frag->reference++;

        shmem_internal_cntr_inc(&shmem_transport_portals4_pending_put_event_cntr);
        ret = PtlPut(shmem_transport_portals4_put_event_md_h,
                     (ptl_size_t) source,
                     len,
                     PTL_OC_ACK_REQ,
                     peer,
                     pt,
                     0,
                     offset,
                     long_frag,
                     0);
        if (PTL_OK != ret) { RAISE_ERROR(ret); }

#if WANT_TOTAL_DATA_ORDERING != 0
        shmem_transport_portals4_long_pending = 1;
#endif
    }
}


static inline
void
shmem_transport_portals4_put_nbi_internal(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                                int pe, ptl_pt_index_t data_pt, ptl_pt_index_t heap_pt)
{
    int ret;
    ptl_process_t peer;
    ptl_pt_index_t pt;
    long offset;

    peer.rank = pe;
#ifdef ENABLE_REMOTE_VIRTUAL_ADDRESSING
    PORTALS4_GET_REMOTE_ACCESS_ONEPT(target, pt, offset, data_pt);
#else
    PORTALS4_GET_REMOTE_ACCESS_TWOPT(target, pt, offset, data_pt, heap_pt);
#endif

    shmem_internal_cntr_inc(&ctx->pending_put_cntr);

    if (len <= shmem_transport_portals4_max_volatile_size) {
        ret = PtlPut(ctx->put_volatile_md,
                     (ptl_size_t) source,
                     len,
                     PTL_OC_ACK_REQ,
                     peer,
                     pt,
                     0,
                     offset,
                     NULL,
                     0);
        if (PTL_OK != ret) { RAISE_ERROR(ret); }

    } else {
        shmem_internal_assert(len <= shmem_transport_portals4_max_msg_size);

        ret = PtlPut(ctx->put_md,
                     (ptl_size_t) source,
                     len,
                     PTL_OC_ACK_REQ,
                     peer,
                     pt,
                     0,
                     offset,
                     NULL,
                     0);
        if (PTL_OK != ret) { RAISE_ERROR(ret); }

#if WANT_TOTAL_DATA_ORDERING != 0
        shmem_transport_portals4_long_pending = 1;
#endif
    }
}


static inline
void
shmem_transport_put_nbi(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len, int pe)
{
#ifdef ENABLE_REMOTE_VIRTUAL_ADDRESSING
    shmem_transport_portals4_put_nbi_internal(ctx, target, source, len, pe,
                                             shmem_transport_portals4_pt,
                                             -1);
#else
    shmem_transport_portals4_put_nbi_internal(ctx, target, source, len, pe,
                                             shmem_transport_portals4_data_pt,
                                             shmem_transport_portals4_heap_pt);
#endif
}


static inline
void
shmem_transport_put_nb(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                                int pe, long *completion)
{
    if (ctx->options & SHMEMX_CTX_BOUNCE_BUFFER) {
#ifdef ENABLE_REMOTE_VIRTUAL_ADDRESSING
        shmem_transport_portals4_put_nb_internal(ctx, target, source, len, pe,
                                                 completion,
                                                 shmem_transport_portals4_pt,
                                                 -1);
#else
        shmem_transport_portals4_put_nb_internal(ctx, target, source, len, pe,
                                                 completion,
                                                 shmem_transport_portals4_data_pt,
                                                 shmem_transport_portals4_heap_pt);
#endif
    } else {
        shmem_transport_put_nbi(ctx, target, source, len, pe);
    }
}


static inline
void
shmem_transport_put_ct_nb(shmem_transport_ct_t *ct, void *target, const void *source,
                          size_t len, int pe, long *completion)
{
#ifdef ENABLE_REMOTE_VIRTUAL_ADDRESSING
    shmem_transport_portals4_put_nb_internal((shmem_transport_ctx_t *)SHMEM_CTX_DEFAULT, target, source, len, pe,
                                             completion, ct->shr_pt, -1);
#else
    shmem_transport_portals4_put_nb_internal((shmem_transport_ctx_t *)SHMEM_CTX_DEFAULT, target, source, len, pe,
                                             completion, ct->data_pt, ct->heap_pt);
#endif
}

static inline
void
shmem_transport_put_wait(shmem_transport_ctx_t* ctx, long *completion)
{
    if (ctx->options & SHMEMX_CTX_BOUNCE_BUFFER) {
        while (*completion > 0) {
            shmem_transport_portals4_drain_eq();
        }
    } else {
        shmem_transport_quiet(ctx);
    }
}

static inline
void
shmem_transport_portals4_get_internal(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len, int pe,
                             ptl_pt_index_t data_pt, ptl_pt_index_t heap_pt)
{
    int ret;
    ptl_process_t peer;
    ptl_pt_index_t pt;
    long offset;

    peer.rank = pe;
#ifdef ENABLE_REMOTE_VIRTUAL_ADDRESSING
    PORTALS4_GET_REMOTE_ACCESS_ONEPT(source, pt, offset, data_pt);
#else
    PORTALS4_GET_REMOTE_ACCESS_TWOPT(source, pt, offset, data_pt, heap_pt);
#endif

    shmem_internal_assert(len <= shmem_transport_portals4_max_msg_size);
    shmem_internal_cntr_inc(&ctx->pending_get_cntr);

    ret = PtlGet(ctx->get_md,
                 (ptl_size_t) target,
                 len,
                 peer,
                 pt,
                 0,
                 offset,
                 0);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
}


static inline
void shmem_transport_get(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len, int pe)
{
#ifdef ENABLE_REMOTE_VIRTUAL_ADDRESSING
    shmem_transport_portals4_get_internal(ctx, target, source, len, pe,
                                          shmem_transport_portals4_pt, -1);
#else
    shmem_transport_portals4_get_internal(ctx, target, source, len, pe,
                                          shmem_transport_portals4_data_pt,
                                          shmem_transport_portals4_heap_pt);
#endif
}


static inline
void shmem_transport_get_ct(shmem_transport_ct_t *ct, void *target,
                            const void *source, size_t len, int pe)
{
#ifdef ENABLE_REMOTE_VIRTUAL_ADDRESSING
    shmem_transport_portals4_get_internal((shmem_transport_ctx_t *)SHMEM_CTX_DEFAULT, target, source, len, pe, ct->shr_pt, -1);
#else
    shmem_transport_portals4_get_internal((shmem_transport_ctx_t *)SHMEM_CTX_DEFAULT, target, source, len, pe,
                                          ct->data_pt, ct->heap_pt);
#endif
}


static inline
void
shmem_transport_get_wait(shmem_transport_ctx_t* ctx)
{
    int ret;
    ptl_ct_event_t ct;
    uint64_t cnt, cnt_new;

    /* NOTE-MT: continue to wait if additional operations are issued */
    cnt_new = shmem_internal_cntr_read(&ctx->pending_get_cntr);
    do {
        cnt = cnt_new;
        ret = PtlCTWait(ctx->get_ct, cnt, &ct);
        cnt_new = shmem_internal_cntr_read(&ctx->pending_get_cntr);
        if (PTL_OK != ret) { RAISE_ERROR(ret); }
        if (ct.failure != 0) { RAISE_ERROR_MSG("get operations failed (%" PRIu64 "\n", ct.failure); }
    } while (cnt < cnt_new);
    shmem_internal_assert(cnt == cnt_new);
}


static inline
void
shmem_transport_swap(shmem_transport_ctx_t* ctx, void *target, const void *source, void *dest, size_t len,
                     int pe, ptl_datatype_t datatype)
{
    int ret;
    ptl_process_t peer;
    ptl_pt_index_t pt;
    long offset;

    peer.rank = pe;
    PORTALS4_GET_REMOTE_ACCESS(target, pt, offset);

    shmem_internal_assert(len <= sizeof(long double _Complex));
    shmem_internal_assert(len <= shmem_transport_portals4_max_volatile_size);

    shmem_internal_cntr_inc(&ctx->pending_get_cntr);

    /* note: No ack is generated on the ct associated with the
       volatile md because the reply comes back on the get md.  So no
       need to increment the put counter */
    ret = PtlSwap(ctx->get_md,
                  (ptl_size_t) dest,
                  ctx->put_volatile_md,
                  (ptl_size_t) source,
                  len,
                  peer,
                  pt,
                  0,
                  offset,
                  NULL,
                  0,
                  NULL,
                  PTL_SWAP,
                  SHMEM_TRANSPORT_DTYPE(datatype));
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
}


static inline
void
shmem_transport_swap_nbi(shmem_transport_ctx_t* ctx, void *target,
                         const void *source, void *dest, size_t len,
                         int pe, ptl_datatype_t datatype)
{
    /* transport_swap already buffers the source argument */
    shmem_transport_swap(ctx, target, source, dest, len, pe, datatype);
}


static inline
void
shmem_transport_cswap(shmem_transport_ctx_t* ctx, void *target, const void *source, void *dest,
                      const void *operand, size_t len, int pe,
                      ptl_datatype_t datatype)
{
    int ret;
    ptl_process_t peer;
    ptl_pt_index_t pt;
    long offset;

    peer.rank = pe;
    PORTALS4_GET_REMOTE_ACCESS(target, pt, offset);

    shmem_internal_assert(len <= sizeof(long double _Complex));
    shmem_internal_assert(len <= shmem_transport_portals4_max_volatile_size);

    shmem_internal_cntr_inc(&ctx->pending_get_cntr);

    /* note: No ack is generated on the ct associated with the
       volatile md because the reply comes back on the get md.  So no
       need to increment the put counter */
    ret = PtlSwap(ctx->get_md,
                  (ptl_size_t) dest,
                  ctx->put_volatile_md,
                  (ptl_size_t) source,
                  len,
                  peer,
                  pt,
                  0,
                  offset,
                  NULL,
                  0,
                  operand,
                  PTL_CSWAP,
                  SHMEM_TRANSPORT_DTYPE(datatype));
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
}


static inline
void
shmem_transport_cswap_nbi(shmem_transport_ctx_t* ctx, void *target,
                          const void *source, void *dest,
                          const void *operand, size_t len, int pe,
                          ptl_datatype_t datatype)
{
    /* transport_cswap already buffers the source and operand arguments */
    shmem_transport_cswap(ctx, target, source, dest, operand, len, pe, datatype);
}


static inline
void
shmem_transport_mswap(shmem_transport_ctx_t* ctx, void *target, const void *source, void *dest,
                      const void *mask, size_t len, int pe,
                      ptl_datatype_t datatype)
{
    int ret;
    ptl_process_t peer;
    ptl_pt_index_t pt;
    long offset;

    peer.rank = pe;
    PORTALS4_GET_REMOTE_ACCESS(target, pt, offset);

    shmem_internal_assert(len <= sizeof(long double _Complex));
    shmem_internal_assert(len <= shmem_transport_portals4_max_volatile_size);

    shmem_internal_cntr_inc(&ctx->pending_get_cntr);

    /* note: No ack is generated on the ct associated with the
       volatile md because the reply comes back on the get md.  So no
       need to increment the put counter */
    ret = PtlSwap(ctx->get_md,
                  (ptl_size_t) dest,
                  ctx->put_volatile_md,
                  (ptl_size_t) source,
                  len,
                  peer,
                  pt,
                  0,
                  offset,
                  NULL,
                  0,
                  mask,
                  PTL_MSWAP,
                  SHMEM_TRANSPORT_DTYPE(datatype));
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
}


static inline
void
shmem_transport_atomic(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                       int pe, ptl_op_t op, ptl_datatype_t datatype)
{
    int ret;
    ptl_pt_index_t pt;
    long offset;
    ptl_process_t peer;

    peer.rank = pe;
    PORTALS4_GET_REMOTE_ACCESS(target, pt, offset);

    shmem_internal_assert(len <= shmem_transport_portals4_max_volatile_size);

    shmem_internal_cntr_inc(&ctx->pending_put_cntr);

    ret = PtlAtomic(ctx->put_volatile_md,
                    (ptl_size_t) source,
                    len,
                    PTL_OC_ACK_REQ,
                    peer,
                    pt,
                    0,
                    offset,
                    NULL,
                    0,
                    op,
                    SHMEM_TRANSPORT_DTYPE(datatype));
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
}


static inline
void
shmem_transport_atomicv(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t count, size_t type_size, int pe,
                        ptl_op_t op, ptl_datatype_t datatype, long *completion)
{
    size_t len = count*type_size;
    int ret;
    ptl_pt_index_t pt;
    long offset;
    ptl_process_t peer;

    shmem_internal_assert(completion != NULL);

    peer.rank = pe;
    PORTALS4_GET_REMOTE_ACCESS(target, pt, offset);

    if (len <= shmem_transport_portals4_max_volatile_size) {
        shmem_internal_cntr_inc(&ctx->pending_put_cntr);
        ret = PtlAtomic(ctx->put_volatile_md,
                        (ptl_size_t) source,
                        len,
                        PTL_OC_ACK_REQ,
                        peer,
                        pt,
                        0,
                        offset,
                        NULL,
                        0,
                        op,
                        SHMEM_TRANSPORT_DTYPE(datatype));
        if (PTL_OK != ret) { RAISE_ERROR(ret); }

    } else if (len <= MIN(shmem_transport_portals4_bounce_buffer_size,
                          shmem_transport_portals4_max_atomic_size)) {
        shmem_transport_portals4_bounce_buffer_t *buff;

        SHMEM_MUTEX_LOCK(shmem_internal_mutex_ptl4_event_slots);
        while (0 >= --shmem_transport_portals4_event_slots) {
            shmem_transport_portals4_event_slots++;
            shmem_transport_portals4_drain_eq();
        }
        SHMEM_MUTEX_UNLOCK(shmem_internal_mutex_ptl4_event_slots);

        shmem_free_list_lock(shmem_transport_portals4_bounce_buffers);
        buff = (shmem_transport_portals4_bounce_buffer_t*)
            shmem_free_list_alloc(shmem_transport_portals4_bounce_buffers);
        shmem_free_list_unlock(shmem_transport_portals4_bounce_buffers);
        if (NULL == buff) RAISE_ERROR(-1);

        shmem_internal_assert(buff->frag.type == SHMEM_TRANSPORT_PORTALS4_TYPE_BOUNCE);

        memcpy(buff->data, source, len);

        shmem_internal_cntr_inc(&shmem_transport_portals4_pending_put_event_cntr);
        ret = PtlAtomic(shmem_transport_portals4_put_event_md_h,
                        (ptl_size_t) buff->data,
                        len,
                        PTL_OC_ACK_REQ,
                        peer,
                        pt,
                        0,
                        offset,
                        buff,
                        0,
                        op,
                        SHMEM_TRANSPORT_DTYPE(datatype));
        if (PTL_OK != ret) { RAISE_ERROR(ret); }
#if WANT_TOTAL_DATA_ORDERING != 0
        shmem_transport_portals4_long_pending = 1;
#endif
    } else {
        size_t sent = 0;
        ptl_size_t base_offset;
        shmem_transport_portals4_long_frag_t *long_frag;

        shmem_free_list_lock(shmem_transport_portals4_long_frags);
        long_frag = (shmem_transport_portals4_long_frag_t*)
             shmem_free_list_alloc(shmem_transport_portals4_long_frags);
        shmem_free_list_unlock(shmem_transport_portals4_long_frags);
        if (NULL == long_frag) { RAISE_ERROR(-1); }

        shmem_internal_assert(long_frag->frag.type == SHMEM_TRANSPORT_PORTALS4_TYPE_LONG);
        shmem_internal_assert(long_frag->reference == 0);

        long_frag->completion = completion;

        base_offset = (ptl_size_t) source;

        /* NOTE-MT: Must hold the frag mutex for the whole loop to prevent another
         * thread from seeing the incomplete frag, completing and freeing it. */
        SHMEM_MUTEX_LOCK(shmem_internal_mutex_ptl4_frag);
        while (sent < len) {
             SHMEM_MUTEX_LOCK(shmem_internal_mutex_ptl4_event_slots);
             while (0 >= --shmem_transport_portals4_event_slots) {
                  shmem_transport_portals4_event_slots++;
                  shmem_transport_portals4_drain_eq();
             }
             SHMEM_MUTEX_UNLOCK(shmem_internal_mutex_ptl4_event_slots);

            size_t bufsize = MIN(len - sent, shmem_transport_portals4_max_atomic_size);

            shmem_internal_cntr_inc(&shmem_transport_portals4_pending_put_event_cntr);
            ret = PtlAtomic(shmem_transport_portals4_put_event_md_h,
                            base_offset + sent,
                            bufsize,
                            PTL_OC_ACK_REQ,
                            peer,
                            pt,
                            0,
                            offset + sent,
                            long_frag,
                            0,
                            op,
                            SHMEM_TRANSPORT_DTYPE(datatype));
            if (PTL_OK != ret) { RAISE_ERROR(ret); }
            (*(long_frag->completion))++;
            long_frag->reference++;
            sent += bufsize;
        }
        SHMEM_MUTEX_UNLOCK(shmem_internal_mutex_ptl4_frag);
#if WANT_TOTAL_DATA_ORDERING != 0
        shmem_transport_portals4_long_pending = 1;
#endif
    }
}


static inline
void
shmem_transport_fetch_atomic(shmem_transport_ctx_t* ctx, void *target, const void *source, void *dest,
                             size_t len, int pe, ptl_op_t op,
                             ptl_datatype_t datatype)
{
    int ret;
    ptl_pt_index_t pt;
    long offset;
    ptl_process_t peer;

    peer.rank = pe;
    PORTALS4_GET_REMOTE_ACCESS(target, pt, offset);

    shmem_internal_assert(len <= shmem_transport_portals4_max_fetch_atomic_size);
    shmem_internal_assert(len <= shmem_transport_portals4_max_volatile_size);

    shmem_internal_cntr_inc(&ctx->pending_get_cntr);

    /* note: No ack is generated on the ct associated with the
       volatile md because the reply comes back on the get md.  So no
       need to increment the put counter */
    ret = PtlFetchAtomic(ctx->get_md,
                         (ptl_size_t) dest,
                         ctx->put_volatile_md,
                         (ptl_size_t) source,
                         len,
                         peer,
                         pt,
                         0,
                         offset,
                         NULL,
                         0,
                         op,
                         SHMEM_TRANSPORT_DTYPE(datatype));
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
}


static inline
void
shmem_transport_fetch_atomic_nbi(shmem_transport_ctx_t* ctx, void *target,
                                 const void *source, void *dest,
                                 size_t len, int pe, ptl_op_t op,
                                 ptl_datatype_t datatype)
{
    /* transport_fetch_atomic already buffers the source argument */
    shmem_transport_fetch_atomic(ctx, target, source, dest, len, pe, op, datatype);
}


static inline
void
shmem_transport_atomic_set(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                           int pe, int datatype)
{
    shmem_internal_assert(len <= shmem_transport_portals4_max_atomic_size);

    shmem_transport_put_scalar(ctx, target, source, len, pe);
}


static inline
void
shmem_transport_atomic_fetch(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                             int pe, int datatype)
{
    shmem_internal_assert(len <= shmem_transport_portals4_max_fetch_atomic_size);

    shmem_transport_get(ctx, target, source, len, pe);
}


static inline
int shmem_transport_atomic_supported(ptl_op_t op, ptl_datatype_t datatype)
{
#ifdef USE_SHR_ATOMICS
    /* FIXME: Force shared memory atomics build to use software reductions */
    return 0;
#else
    return 1;
#endif
}

static inline
void shmem_transport_put_signal_nbi(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                                    uint64_t *sig_addr, uint64_t signal, int sig_op, int pe)
{
    /* FIXME: Need to optimize non-blocking put with signal for Portals. Current implementation below keeps
 *      * the "fence" in between data and signal put */
    shmem_transport_put_nbi(ctx, target, source, len, pe);
    shmem_transport_fence(ctx);
    if (sig_op == SHMEM_SIGNAL_ADD)
        shmem_transport_atomic(ctx, sig_addr, &signal, sizeof(uint64_t), pe, SHM_INTERNAL_SUM, SHM_INTERNAL_UINT64);
    else
        shmem_transport_atomic_set(ctx, sig_addr, &signal, sizeof(uint64_t), pe, SHM_INTERNAL_UINT64);
}

static inline
void shmem_transport_portals4_ct_attach(ptl_handle_ct_t ptl_ct, void *seg_base,
                                        ptl_size_t seg_length, ptl_pt_index_t *seg_pt,
                                        ptl_handle_le_t *seg_le)
{
    int ret;
    ptl_pt_index_t des_pt;
    ptl_le_t le;

    SHMEM_MUTEX_LOCK(shmem_internal_mutex_ptl4_pt_state);

    /* Find the next free PT index.  These are allocated collectively by
     * all PEs, so the state array should stay in sync on all PEs. */

    /* NOTE-MT: The above assertion only hold for multithreaded SHMEM,
     * as long as we restrict collectives to be free from thread-level
     * concurrency at all PEs. */
    for (des_pt = 0; des_pt < SHMEM_TRANSPORT_PORTALS4_NUM_PTS &&
         shmem_transport_portals4_pt_state[des_pt] != PT_FREE; des_pt++)
        ;

    if (des_pt >= SHMEM_TRANSPORT_PORTALS4_NUM_PTS) {
        RAISE_ERROR_STR("Out of PT entries allocating CT object");
    }

    shmem_transport_portals4_pt_state[des_pt] = PT_ALLOCATED;

    SHMEM_MUTEX_UNLOCK(shmem_internal_mutex_ptl4_pt_state);

    /* Counters are distinguished using distinct portal table entries.
     * Allocate PT entry for the given segment */
    ret = PtlPTAlloc(shmem_transport_portals4_ni_h,
                     0,
                     shmem_transport_portals4_eq_h,
                     des_pt,
                     seg_pt);
    if (PTL_OK != ret) {
        RAISE_ERROR_MSG("PtlPTAlloc of data table failed: %d\n", ret);
    }
    if (*seg_pt != des_pt) {
        RAISE_ERROR_MSG("data portal table index mis-match: "
                        "desired = %d, actual = %d\n", des_pt, *seg_pt);
    }

    /* Open LE to data segment */
    le.start = seg_base;
    le.length = seg_length;
    le.ct_handle = ptl_ct;
    le.uid = PTL_UID_ANY;
    le.options = PTL_LE_OP_PUT | PTL_LE_OP_GET |
        PTL_LE_EVENT_LINK_DISABLE |
        PTL_LE_EVENT_SUCCESS_DISABLE |
        PTL_LE_EVENT_CT_COMM;
    ret = PtlLEAppend(shmem_transport_portals4_ni_h,
                      *seg_pt,
                      &le,
                      PTL_PRIORITY_LIST,
                      NULL,
                      seg_le);
    if (PTL_OK != ret) {
        RAISE_ERROR_MSG("PtlLEAppend of data section failed: %d\n", ret);
    }
}


static inline
void shmem_transport_ct_create(shmem_transport_ct_t **ct_ptr)
{
    int ret;
    shmem_transport_ct_t *ct;

    ct = malloc(sizeof(shmem_transport_ct_t));
    if (NULL == ct) {
        RAISE_ERROR_STR("Out of memory allocating CT object");
    }
    *ct_ptr = ct;

    /* Allocate the counting event */
    ret = PtlCTAlloc(shmem_transport_portals4_ni_h, &ct->ct);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }

#ifdef ENABLE_REMOTE_VIRTUAL_ADDRESSING
    shmem_transport_portals4_ct_attach(ct->ct, NULL, PTL_SIZE_MAX, &ct->shr_pt, &ct->shr_le);
#else
    shmem_transport_portals4_ct_attach(ct->ct, shmem_internal_data_base, shmem_internal_data_length,
                                       &ct->data_pt, &ct->data_le);
    shmem_transport_portals4_ct_attach(ct->ct, shmem_internal_heap_base, shmem_internal_heap_length,
                                       &ct->heap_pt, &ct->heap_le);
#endif
}


static inline
void shmem_transport_ct_free(shmem_transport_ct_t **ct_ptr)
{
    int ret;
    shmem_transport_ct_t *ct = *ct_ptr;

    ret = PtlCTFree(ct->ct);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }

#ifdef ENABLE_REMOTE_VIRTUAL_ADDRESSING
    ret = PtlLEUnlink(ct->shr_le);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }

    ret = PtlPTFree(shmem_transport_portals4_ni_h, ct->shr_pt);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }

    SHMEM_MUTEX_LOCK(shmem_internal_mutex_ptl4_pt_state);
    shmem_transport_portals4_pt_state[ct->shr_pt] = PT_FREE;
    SHMEM_MUTEX_UNLOCK(shmem_internal_mutex_ptl4_pt_state);
#else
    ret = PtlLEUnlink(ct->data_le);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
    ret = PtlLEUnlink(ct->heap_le);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }

    ret = PtlPTFree(shmem_transport_portals4_ni_h, ct->data_pt);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
    ret = PtlPTFree(shmem_transport_portals4_ni_h, ct->heap_pt);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }

    SHMEM_MUTEX_LOCK(shmem_internal_mutex_ptl4_pt_state);
    shmem_transport_portals4_pt_state[ct->data_pt] = PT_FREE;
    shmem_transport_portals4_pt_state[ct->heap_pt] = PT_FREE;
    SHMEM_MUTEX_UNLOCK(shmem_internal_mutex_ptl4_pt_state);
#endif

    free(ct);
    *ct_ptr = NULL;
}


static inline
long shmem_transport_ct_get(shmem_transport_ct_t *ct)
{
    int ret;
    ptl_ct_event_t ev;

    ret = PtlCTGet(ct->ct, &ev);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }

    /* TODO: Handle failures gracefully, instead of aborting */
    if (ev.failure != 0) { RAISE_ERROR(ret); }

    return ev.success;
}


static inline
void shmem_transport_ct_set(shmem_transport_ct_t *ct, long value)
{
    int ret;
    ptl_ct_event_t ev;

    ev.success = (ptl_size_t) value;
    ev.failure = (ptl_size_t) 0;

    ret = PtlCTSet(ct->ct, ev);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
}


static inline
void shmem_transport_ct_wait(shmem_transport_ct_t *ct, long wait_for)
{
    int ret;
    ptl_ct_event_t ev;

    ret = PtlCTWait(ct->ct, (ptl_size_t) wait_for, &ev);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }

    /* TODO: Handle failures gracefully, instead of aborting */
    if (ev.failure != (ptl_size_t) 0 || ev.success < (ptl_size_t) wait_for) {
        RAISE_ERROR(ret);
    }
}

static inline
uint64_t shmem_transport_received_cntr_get(void)
{
#ifndef ENABLE_HARD_POLLING
    int ret;
    ptl_ct_event_t ct;

    ret = PtlCTGet(shmem_transport_portals4_target_ct_h, &ct);

    if (0 != ct.failure) {
        RAISE_ERROR_STR("Target CT failure");
    }

    if (PTL_OK != ret) {
        RAISE_ERROR(ret);
    }

    return (uint64_t) ct.success;
#else
    RAISE_ERROR_STR("Portals transport configured for hard polling");
    return 0;
#endif
}


static inline
void shmem_transport_received_cntr_wait(uint64_t ge_val)
{
#ifndef ENABLE_HARD_POLLING
    int ret;
    ptl_ct_event_t ct;

    ret = PtlCTWait(shmem_transport_portals4_target_ct_h,
                    ge_val, &ct);

    if (PTL_OK != ret) {
        RAISE_ERROR(ret);
    }

    if (0 != ct.failure) {
        RAISE_ERROR_STR("Target CT failure");
    }
#else
    RAISE_ERROR_STR("Portals transport configured for hard polling");
#endif
}

static inline
void shmem_transport_syncmem(void)
{
    PtlAtomicSync();
}

static inline
uint64_t shmem_transport_pcntr_get_issued_write(shmem_transport_ctx_t *ctx)
{
    uint64_t cnt = 0;
    if (ctx->options & SHMEMX_CTX_BOUNCE_BUFFER) {
        cnt = shmem_internal_cntr_read(&shmem_transport_portals4_pending_put_event_cntr);
    }
    cnt += shmem_internal_cntr_read(&ctx->pending_put_cntr);
    return cnt;
}

static inline
uint64_t shmem_transport_pcntr_get_issued_read(shmem_transport_ctx_t *ctx)
{
    uint64_t cnt = shmem_internal_cntr_read(&ctx->pending_get_cntr);
    return cnt;
}

static inline
uint64_t shmem_transport_pcntr_get_completed_write(shmem_transport_ctx_t *ctx)
{
    int ret;
    ptl_ct_event_t ev;

    ret = PtlCTGet(ctx->put_ct, &ev);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }

    uint64_t cnt = ev.success;
    if (ctx->options & SHMEMX_CTX_BOUNCE_BUFFER) {
        ret = PtlCTGet(shmem_transport_portals4_put_event_ct_h, &ev);
        if (PTL_OK != ret) { RAISE_ERROR(ret); }
        cnt += ev.success;
    }
    return cnt;
}

static inline
uint64_t shmem_transport_pcntr_get_completed_read(shmem_transport_ctx_t *ctx)
{
    int ret;
    ptl_ct_event_t ev;

    ret = PtlCTGet(ctx->get_ct, &ev);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }

    return (uint64_t) ev.success;
}

static inline
uint64_t shmem_transport_pcntr_get_completed_target(void)
{
#ifndef ENABLE_HARD_POLLING
    int ret;
    ptl_ct_event_t ev;

    ret = PtlCTGet(shmem_transport_portals4_target_ct_h, &ev);

    if (PTL_OK != ret) { RAISE_ERROR(ret); }
    return (uint64_t) ev.success;
#else
    return 0;
#endif
}

static inline
void shmem_transport_pcntr_get_all(shmem_transport_ctx_t *ctx, shmemx_pcntr_t *pcntr)
{
    pcntr->pending_get = shmem_transport_pcntr_get_issued_read(ctx); 
    pcntr->completed_get = shmem_transport_pcntr_get_completed_read(ctx); 
    pcntr->pending_put = shmem_transport_pcntr_get_issued_write(ctx); 
    pcntr->completed_put = shmem_transport_pcntr_get_completed_write(ctx); 
    pcntr->target = shmem_transport_pcntr_get_completed_target(); 
}

#endif /* TRANSPORT_PORTALS_H */
