/* -*- C -*-
 *
 * Copyright 2011 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S.  Government
 * retains certain rights in this software.
 * 
 * This file is part of the Portals SHMEM software package. For license
 * information, see the LICENSE file in the top level directory of the
 * distribution.
 *
 */

#ifndef TRANSPORT_PORTALS_H
#define TRANSPORT_PORTALS_H

#include <portals4.h>
#include <string.h>

#include "shmem_free_list.h"

#define shmem_transport_portals4_data_pt 8
#define shmem_transport_portals4_heap_pt 9

extern ptl_handle_ni_t shmem_transport_portals4_ni_h;
extern ptl_handle_md_t shmem_transport_portals4_put_volatile_md_h;
extern ptl_handle_md_t shmem_transport_portals4_put_event_md_h;
extern ptl_handle_md_t shmem_transport_portals4_get_md_h;
extern ptl_handle_ct_t shmem_transport_portals4_target_ct_h;
extern ptl_handle_ct_t shmem_transport_portals4_put_ct_h;
extern ptl_handle_ct_t shmem_transport_portals4_get_ct_h;
extern ptl_handle_eq_t shmem_transport_portals4_eq_h;

extern shmem_free_list_t *shmem_transport_portals4_bounce_buffers;
extern shmem_free_list_t *shmem_transport_portals4_long_frags;

extern ptl_size_t shmem_transport_portals4_bounce_buffer_size;
extern ptl_size_t shmem_transport_portals4_max_volatile_size;
extern ptl_size_t shmem_transport_portals4_max_atomic_size;
extern ptl_size_t shmem_transport_portals4_max_fetch_atomic_size;

extern ptl_size_t shmem_transport_portals4_pending_put_counter;
extern ptl_size_t shmem_transport_portals4_pending_get_counter;

extern int32_t shmem_transport_portals4_event_slots;

#define SHMEM_TRANSPORT_PORTALS4_TYPE_BOUNCE  0x01
#define SHMEM_TRANSPORT_PORTALS4_TYPE_LONG    0x02

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

#ifdef ENABLE_ERROR_CHECKING
#define PORTALS4_GET_REMOTE_ACCESS(target, pt, offset)                  \
    do {                                                                \
        if (((void*) target > shmem_internal_data_base) &&              \
            ((char*) target < (char*) shmem_internal_data_base + shmem_internal_data_length)) { \
            pt = shmem_transport_portals4_data_pt;                      \
            offset = (char*) target - (char*) shmem_internal_data_base; \
        } else if (((void*) target > shmem_internal_heap_base) &&       \
                   ((char*) target < (char*) shmem_internal_heap_base + shmem_internal_heap_length)) { \
            pt = shmem_transport_portals4_heap_pt;                      \
            offset = (char*) target - (char*) shmem_internal_heap_base; \
        } else {                                                        \
            printf("[%03d] ERROR: target (0x%lx) outside of symmetric areas\n", \
                   shmem_internal_my_pe, (unsigned long) target);       \
            abort();                                                    \
        }                                                               \
    } while (0)
#else 
#define PORTALS4_GET_REMOTE_ACCESS(target, pt, offset)                  \
    do {                                                                \
        if ((void*) target < shmem_internal_heap_base) {                \
            pt = shmem_transport_portals4_data_pt;                      \
            offset = (char*) target - (char*) shmem_internal_data_base; \
        } else {                                                        \
            pt = shmem_transport_portals4_heap_pt;                      \
            offset = (char*) target - (char*) shmem_internal_heap_base; \
        }                                                               \
    } while (0)
#endif

int shmem_transport_portals4_init(long eager_size);

int shmem_transport_portals4_startup(void);

int shmem_transport_portals4_fini(void);

static inline
int
shmem_transport_portals4_quiet(void)
{
    int ret;
    ptl_ct_event_t ct;

    /* wait for remote completion (acks) of all pending events */
    ret = PtlCTWait(shmem_transport_portals4_put_ct_h, 
                    shmem_transport_portals4_pending_put_counter, &ct);
    if (PTL_OK != ret) { return ret; }
    if (ct.failure != 0) { return -1; }

    return 0;
}


static inline
int
shmem_transport_portals4_fence(void)
{
    return shmem_transport_portals4_quiet();
}


static inline
void
shmem_transport_portals4_drain_eq(void)
{
    int ret;
    ptl_event_t ev;

    ret = PtlEQWait(shmem_transport_portals4_eq_h, &ev);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
    if (ev.ni_fail_type != PTL_OK) { RAISE_ERROR(ev.ni_fail_type); }

    /* The only event type we should see on a success is a send event */
    assert(ev.type == PTL_EVENT_SEND);

    shmem_transport_portals4_event_slots++;

    shmem_transport_portals4_frag_t *frag = 
         (shmem_transport_portals4_frag_t*) ev.user_ptr;

    if (SHMEM_TRANSPORT_PORTALS4_TYPE_BOUNCE == frag->type) {
         /* it's a short send completing */
         shmem_free_list_free(shmem_transport_portals4_bounce_buffers,
                              frag);
    } else {
         /* it's one of the long messages we're waiting for */
         shmem_transport_portals4_long_frag_t *long_frag = 
              (shmem_transport_portals4_long_frag_t*) frag;

         (*(long_frag->completion))--;
         if (0 >= --long_frag->reference) {
              long_frag->reference = 0;
              shmem_free_list_free(shmem_transport_portals4_long_frags,
                                   frag);
         }
    }
}


static inline
void
shmem_transport_portals4_put_small(void *target, const void *source, size_t len, int pe)
{
    int ret;
    ptl_process_t peer;
    ptl_pt_index_t pt;
    long offset;
    peer.rank = pe;
    PORTALS4_GET_REMOTE_ACCESS(target, pt, offset);

    assert(len <= shmem_transport_portals4_max_volatile_size);

    ret = PtlPut(shmem_transport_portals4_put_volatile_md_h,
                 (ptl_size_t) source,
                 len,
                 PTL_CT_ACK_REQ,
                 peer,
                 pt,
                 0,
                 offset,
                 NULL,
                 0);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
    shmem_transport_portals4_pending_put_counter++;
}


static inline
void
shmem_transport_portals4_put_nb(void *target, const void *source, size_t len,
                                int pe, long *completion)
{
    int ret;
    ptl_process_t peer;
    ptl_pt_index_t pt;
    long offset;
    peer.rank = pe;
    PORTALS4_GET_REMOTE_ACCESS(target, pt, offset);

    if (len <= shmem_transport_portals4_max_volatile_size) {
        ret = PtlPut(shmem_transport_portals4_put_volatile_md_h,
                     (ptl_size_t) source,
                     len,
                     PTL_CT_ACK_REQ,
                     peer,
                     pt,
                     0,
                     offset,
                     NULL,
                     0);
        if (PTL_OK != ret) { RAISE_ERROR(ret); }

    } else if (len <= shmem_transport_portals4_bounce_buffer_size) {
        shmem_transport_portals4_bounce_buffer_t *buff;

        while (0 >= --shmem_transport_portals4_event_slots) {
            shmem_transport_portals4_event_slots++;
            shmem_transport_portals4_drain_eq();
        }

        buff = (shmem_transport_portals4_bounce_buffer_t*)
            shmem_free_list_alloc(shmem_transport_portals4_bounce_buffers);
        if (NULL == buff) RAISE_ERROR(-1);

        assert(buff->frag.type == SHMEM_TRANSPORT_PORTALS4_TYPE_BOUNCE);

        memcpy(buff->data, source, len);
        ret = PtlPut(shmem_transport_portals4_put_event_md_h,
                     (ptl_size_t) buff->data,
                     len,
                     PTL_CT_ACK_REQ,
                     peer,
                     pt,
                     0,
                     offset,
                     buff,
                     0);
        if (PTL_OK != ret) { RAISE_ERROR(ret); }

    } else {
        shmem_transport_portals4_long_frag_t *long_frag;  

        while (0 >= --shmem_transport_portals4_event_slots) {
            shmem_transport_portals4_event_slots++;
            shmem_transport_portals4_drain_eq();
        }

        long_frag = (shmem_transport_portals4_long_frag_t*)
            shmem_free_list_alloc(shmem_transport_portals4_long_frags);
        if (NULL == long_frag) RAISE_ERROR(-1);

        assert(long_frag->frag.type == SHMEM_TRANSPORT_PORTALS4_TYPE_LONG);
        assert(long_frag->reference == 0);
        long_frag->completion = completion;

        ret = PtlPut(shmem_transport_portals4_put_event_md_h,
                     (ptl_size_t) source,
                     len,
                     PTL_CT_ACK_REQ,
                     peer,
                     pt,
                     0,
                     offset,
                     long_frag,
                     0);
        if (PTL_OK != ret) { RAISE_ERROR(ret); } 
        (*(long_frag->completion))++;
        long_frag->reference++;
    }
    shmem_transport_portals4_pending_put_counter++;
}


static inline
void
shmem_transport_portals4_put_wait(long *completion)
{
    while (*completion > 0) {
        shmem_transport_portals4_drain_eq();
    }
}


static inline
void
shmem_transport_portals4_get(void *target, const void *source, size_t len, int pe)
{
    int ret;
    ptl_process_t peer;
    ptl_pt_index_t pt;
    long offset;
    peer.rank = pe;
    PORTALS4_GET_REMOTE_ACCESS(source, pt, offset);

    ret = PtlGet(shmem_transport_portals4_get_md_h,
                 (ptl_size_t) target,
                 len,
                 peer,
                 pt,
                 0,
                 offset,
                 0);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
    shmem_transport_portals4_pending_get_counter++;
}


static inline
void
shmem_transport_portals4_get_wait(void)
{
    int ret;
    ptl_ct_event_t ct;

    ret = PtlCTWait(shmem_transport_portals4_get_ct_h, 
                    shmem_transport_portals4_pending_get_counter,
                    &ct);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
    if (ct.failure != 0) { RAISE_ERROR(ct.failure); }
}


static inline
void
shmem_transport_portals4_swap(void *target, void *source, void *dest, size_t len, 
                              int pe, ptl_datatype_t datatype)
{
    int ret;
    ptl_process_t peer;
    ptl_pt_index_t pt;
    long offset;
    peer.rank = pe;
    PORTALS4_GET_REMOTE_ACCESS(target, pt, offset);

    assert(len <= sizeof(long double complex));
    assert(len <= shmem_transport_portals4_max_volatile_size);

    /* note: No ack is generated on the ct associated with the
       volatile md because the reply comes back on the get md.  So no
       need to increment the put counter */
    ret = PtlSwap(shmem_transport_portals4_get_md_h,
                  (ptl_size_t) dest,
                  shmem_transport_portals4_put_volatile_md_h,
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
                  datatype);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
    shmem_transport_portals4_pending_get_counter++;
}


static inline
void
shmem_transport_portals4_cswap(void *target, void *source, void *dest, void *operand, size_t len, 
                               int pe, ptl_datatype_t datatype)
{
    int ret;
    ptl_process_t peer;
    ptl_pt_index_t pt;
    long offset;
    peer.rank = pe;
    PORTALS4_GET_REMOTE_ACCESS(target, pt, offset);

    assert(len <= sizeof(long double complex));
    assert(len <= shmem_transport_portals4_max_volatile_size);

    /* note: No ack is generated on the ct associated with the
       volatile md because the reply comes back on the get md.  So no
       need to increment the put counter */
    ret = PtlSwap(shmem_transport_portals4_get_md_h,
                  (ptl_size_t) dest,
                  shmem_transport_portals4_put_volatile_md_h,
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
                  datatype);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
    shmem_transport_portals4_pending_get_counter++;
}


static inline
void
shmem_transport_portals4_mswap(void *target, void *source, void *dest, void *mask, size_t len, 
                               int pe, ptl_datatype_t datatype)
{
    int ret;
    ptl_process_t peer;
    ptl_pt_index_t pt;
    long offset;
    peer.rank = pe;
    PORTALS4_GET_REMOTE_ACCESS(target, pt, offset);

    assert(len <= sizeof(long double complex));
    assert(len <= shmem_transport_portals4_max_volatile_size);

    /* note: No ack is generated on the ct associated with the
       volatile md because the reply comes back on the get md.  So no
       need to increment the put counter */
    ret = PtlSwap(shmem_transport_portals4_get_md_h,
                  (ptl_size_t) dest,
                  shmem_transport_portals4_put_volatile_md_h,
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
                  datatype);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
    shmem_transport_portals4_pending_get_counter++;
}


static inline
void
shmem_transport_portals4_atomic_small(void *target, void *source, size_t len,
                                       int pe, ptl_op_t op, ptl_datatype_t datatype)
{
    int ret;
    ptl_pt_index_t pt;
    long offset;
    ptl_process_t peer;
    peer.rank = pe;
    PORTALS4_GET_REMOTE_ACCESS(target, pt, offset);

    assert(len <= shmem_transport_portals4_max_volatile_size);

    ret = PtlAtomic(shmem_transport_portals4_put_volatile_md_h,
                    (ptl_size_t) source,
                    len,
                    PTL_CT_ACK_REQ,
                    peer,
                    pt,
                    0,
                    offset,
                    NULL,
                    0,
                    op,
                    datatype);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
    shmem_transport_portals4_pending_put_counter += 1;
}


static inline
void
shmem_transport_portals4_atomic_nb(void *target, void *source, size_t len,
                                   int pe, ptl_op_t op, ptl_datatype_t datatype,
                                   long *completion)
{
    int ret;
    ptl_pt_index_t pt;
    long offset;
    ptl_process_t peer;
    peer.rank = pe;
    PORTALS4_GET_REMOTE_ACCESS(target, pt, offset);

    if (len <= shmem_transport_portals4_max_volatile_size) {
        ret = PtlAtomic(shmem_transport_portals4_put_volatile_md_h,
                        (ptl_size_t) source,
                        len,
                        PTL_CT_ACK_REQ,
                        peer,
                        pt,
                        0,
                        offset,
                        NULL,
                        0,
                        op,
                        datatype);
        if (PTL_OK != ret) { RAISE_ERROR(ret); }
        shmem_transport_portals4_pending_put_counter++;

    } else if (len <= MIN(shmem_transport_portals4_bounce_buffer_size,
                          shmem_transport_portals4_max_atomic_size)) {
        shmem_transport_portals4_bounce_buffer_t *buff;

        while (0 >= --shmem_transport_portals4_event_slots) {
            shmem_transport_portals4_event_slots++;
            shmem_transport_portals4_drain_eq();
        }

        buff = (shmem_transport_portals4_bounce_buffer_t*)
            shmem_free_list_alloc(shmem_transport_portals4_bounce_buffers);
        if (NULL == buff) RAISE_ERROR(-1);

        assert(buff->frag.type == SHMEM_TRANSPORT_PORTALS4_TYPE_BOUNCE);

        memcpy(buff->data, source, len);
        ret = PtlAtomic(shmem_transport_portals4_put_event_md_h,
                        (ptl_size_t) buff->data,
                        len,
                        PTL_CT_ACK_REQ,
                        peer,
                        pt,
                        0,
                        offset,
                        buff,
                        0,
                        op,
                        datatype);
        if (PTL_OK != ret) { RAISE_ERROR(ret); }
        shmem_transport_portals4_pending_put_counter += 1;

    } else {
        size_t sent = 0;
        shmem_transport_portals4_long_frag_t *long_frag =
            (shmem_transport_portals4_long_frag_t*)
             shmem_free_list_alloc(shmem_transport_portals4_long_frags);
        if (NULL == long_frag) RAISE_ERROR(-1);

        assert(long_frag->frag.type == SHMEM_TRANSPORT_PORTALS4_TYPE_LONG);
        assert(long_frag->reference == 0);

        long_frag->completion = completion;

        while (sent < len) {
             while (0 >= --shmem_transport_portals4_event_slots) {
                  shmem_transport_portals4_event_slots++;
                  shmem_transport_portals4_drain_eq();
             }

            size_t bufsize = MIN(len - sent, shmem_transport_portals4_max_atomic_size);
            ret = PtlAtomic(shmem_transport_portals4_put_event_md_h,
                            (ptl_size_t) ((char*) source + sent),
                            bufsize,
                            PTL_CT_ACK_REQ,
                            peer,
                            pt,
                            0,
                            offset + sent,
                            long_frag,
                            0,
                            op,
                            datatype);
            if (PTL_OK != ret) { RAISE_ERROR(ret); }
            (*(long_frag->completion))++;
            long_frag->reference++;
            shmem_transport_portals4_pending_put_counter++;
            sent += bufsize;
        }
    }
}


static inline
void
shmem_transport_portals4_fetch_atomic(void *target, void *source, void *dest, size_t len,
                                      int pe, ptl_op_t op, ptl_datatype_t datatype)
{
    int ret;
    ptl_pt_index_t pt;
    long offset;
    ptl_process_t peer;
    peer.rank = pe;
    PORTALS4_GET_REMOTE_ACCESS(target, pt, offset);

    assert(len <= shmem_transport_portals4_max_fetch_atomic_size);
    assert(len <= shmem_transport_portals4_max_volatile_size);

    /* note: No ack is generated on the ct associated with the
       volatile md because the reply comes back on the get md.  So no
       need to increment the put counter */
    ret = PtlFetchAtomic(shmem_transport_portals4_get_md_h,
                         (ptl_size_t) dest,
                         shmem_transport_portals4_put_volatile_md_h,
                         (ptl_size_t) source,
                         len,
                         peer,
                         pt,
                         0,
                         offset,
                         NULL,
                         0,
                         op,
                         datatype);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
    shmem_transport_portals4_pending_get_counter++;
}

#endif
