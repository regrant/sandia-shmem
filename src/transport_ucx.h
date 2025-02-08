/* -*- C -*-
 *
 * Copyright (c) 2020 NVidia Corporation. All rights reserved.
 * This software is available to you under the BSD license.
 *
 * This file is part of the Sandia OpenSHMEM software package. For license
 * information, see the LICENSE file in the top level directory of the
 * distribution.
 *
 */

#ifndef TRANSPORT_UCX_H
#define TRANSPORT_UCX_H

#include <string.h>
#include "shmem_internal.h"
#include "transport.h"
#include <ucs/type/status.h>
#include <ucp/api/ucp_def.h>
#include <ucp/api/ucp.h>

/* Operations */
enum shm_internal_op_t {
    SHM_INTERNAL_BAND,
    SHM_INTERNAL_BOR,
    SHM_INTERNAL_BXOR,
    SHM_INTERNAL_SUM,
    SHM_INTERNAL_MIN,
    SHM_INTERNAL_MAX,
    SHM_INTERNAL_PROD
};

/* The last op supported by the transport layer atomics. Additional ops/types
 * can be implemented to support reductions and are enabled via the
 * shmem_transport_atomic_supported routine below. */
#define SHMEM_TRANSPORT_UCX_OP_LAST SHM_INTERNAL_SUM

extern ucp_atomic_post_op_t shmem_transport_ucx_post_op[];
extern ucp_atomic_fetch_op_t shmem_transport_ucx_fetch_op[];

typedef enum shm_internal_op_t shm_internal_op_t;
typedef int shmem_transport_ct_t;

struct shmem_transport_ctx_t {
    long options;
    struct shmem_internal_team_t *team;
};
typedef struct shmem_transport_ctx_t shmem_transport_ctx_t;

typedef struct {
    size_t         addr_len;
    ucp_address_t *addr;
    ucp_ep_h       ep;
#ifndef ENABLE_REMOTE_VIRTUAL_ADDRESSING
    uint8_t       *data_base, *heap_base;
#endif
    ucp_rkey_h     data_rkey, heap_rkey;
} shmem_transport_peer_t;

extern shmem_transport_peer_t *shmem_transport_peers;
extern ucp_worker_h shmem_transport_ucp_worker;

void shmem_transport_ucx_cb_nop(void *request, ucs_status_t status);
void shmem_transport_ucx_cb_complete(void *request, ucs_status_t status, void *user_data);

int shmem_transport_init(void);
int shmem_transport_startup(void);
int shmem_transport_fini(void);

#define UCX_CHECK_STATUS(status)                                                        \
    do {                                                                                \
        if (status != UCS_OK) {                                                         \
            RAISE_ERROR_MSG("UCX error %d: %s\n", status, ucs_status_string(status));   \
        }                                                                               \
    } while (0)

#define UCX_CHECK_STATUS_INPROGRESS(status)                                             \
    do {                                                                                \
        if (status != UCS_OK && status != UCS_INPROGRESS) {                             \
            RAISE_ERROR_MSG("UCX error %d: %s\n", status, ucs_status_string(status));   \
        }                                                                               \
    } while (0)

static inline
void
shmem_transport_probe(void)
{
    ucp_worker_progress(shmem_transport_ucp_worker);
}

static inline
ucs_status_t shmem_transport_ucx_complete_op(ucs_status_ptr_t req) {
    if (req == NULL) {
        /* All calls to complete_op must generate progress to avoid deadlock
         * in application-level polling loops */
        shmem_transport_probe();
        return UCS_OK;
    } else if (UCS_PTR_IS_ERR(req)) {
        return UCS_PTR_STATUS(req);
    } else {
        ucs_status_t status;
        do {
            shmem_transport_probe();
            status = ucp_request_check_status(req);
        } while (status == UCS_INPROGRESS);
        ucp_request_free(req);
        return status;
    }
}

static inline
ucs_status_t shmem_transport_ucx_release_op(ucs_status_ptr_t req) {
    if (req == NULL)
         return UCS_OK;
    else if (UCS_PTR_IS_ERR(req))
        return UCS_PTR_STATUS(req);
    else {
        ucp_request_free(req);
        return UCS_INPROGRESS;
    }
}

static inline
ucs_status_t shmem_transport_ucx_post_cb_op(ucs_status_ptr_t req, void *completion) {
    if (req == NULL) {
        __atomic_store_n((long*)completion, 0, __ATOMIC_RELEASE);
        return UCS_OK;
    } else if (UCS_PTR_IS_ERR(req)) {
        return UCS_PTR_STATUS(req);
    } else {
        ucp_request_free(req);
        return UCS_INPROGRESS;
    }
}

static inline
void shmem_transport_ucx_get_mr(const void *addr, int dest_pe,
                                uint8_t **remote_addr, ucp_rkey_h *rkey) {
    if ((void*) addr >= shmem_internal_data_base &&
        (uint8_t*) addr < (uint8_t*) shmem_internal_data_base + shmem_internal_data_length) {

        *rkey = shmem_transport_peers[dest_pe].data_rkey;
#ifdef ENABLE_REMOTE_VIRTUAL_ADDRESSING
        *remote_addr = (uint8_t *) addr;
#else
        *remote_addr = (uint8_t*) (((uint8_t *) addr - (uint8_t *) shmem_internal_data_base) +
                       shmem_transport_peers[dest_pe].data_base);
#endif
    } else if ((void*) addr >= shmem_internal_heap_base &&
               (uint8_t*) addr < (uint8_t*) shmem_internal_heap_base + shmem_internal_heap_length) {

        *rkey = shmem_transport_peers[dest_pe].heap_rkey;
#ifdef ENABLE_REMOTE_VIRTUAL_ADDRESSING
        *remote_addr = (uint8_t *) addr;
#else
        *remote_addr = (uint8_t*) (((uint8_t *) addr - (uint8_t *) shmem_internal_heap_base) +
                       shmem_transport_peers[dest_pe].heap_base);
#endif
    } else {
        RAISE_ERROR_MSG("address (%p) outside of symmetric areas\n", addr);
    }
}

static inline
int
shmem_transport_ctx_create(struct shmem_internal_team_t *team, long options, shmem_transport_ctx_t **ctx)
{
    /* FIXME: Contexts are not optimized yet. We could create a separate set of
     * EPs for each context (or team), and/or we could use completion callbacks
     * to implement completion counters at the SOS level to enable separate
     * completion tracking per context. */

    if (team == NULL)
        RAISE_ERROR_STR("Context creation occured on a NULL team");

    *ctx = malloc(sizeof(shmem_transport_ctx_t));

    if (*ctx == NULL)
        return 1;

    (*ctx)->team = team;
    (*ctx)->options = 0;

    return 0;
}

static inline
void
shmem_transport_ctx_destroy(shmem_transport_ctx_t *ctx)
{
    if (ctx == NULL)
        return;
    else if (ctx == (shmem_transport_ctx_t *) SHMEM_CTX_DEFAULT)
        RAISE_ERROR_STR("Cannot destroy SHMEM_CTX_DEFAULT");
    else
        free(ctx);

    return;
}

static inline
int
shmem_transport_session_start(shmem_transport_ctx_t *ctx, long options, const shmem_session_config_t *config, long config_mask)
{
    return 0;
}

static inline
int
shmem_transport_session_stop(shmem_transport_ctx_t *ctx)
{
      return 0;
}

static inline
int
shmem_transport_quiet(shmem_transport_ctx_t* ctx)
{
    ucs_status_t status;

    status = ucp_worker_flush(shmem_transport_ucp_worker);
    UCX_CHECK_STATUS(status);

    return 0;
}

static inline
int
shmem_transport_fence(shmem_transport_ctx_t* ctx)
{
    ucs_status_t status;

#if defined(USE_CMA) || (defined(USE_XPMEM) && !defined(USE_SHR_ATOMICS))
    /* Put/get use shared memory and atomics use UCX. Flush to resolve a race
     * across transports. */
    status = ucp_worker_flush(shmem_transport_ucp_worker);
#else
    status = ucp_worker_fence(shmem_transport_ucp_worker);
#endif
    UCX_CHECK_STATUS(status);

    return 0;
}

static inline
void
shmem_transport_put_scalar(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len, int pe)
{
    ucs_status_t status;
    ucp_rkey_h rkey;
    uint8_t *remote_addr;

    shmem_transport_ucx_get_mr(target, pe, &remote_addr, &rkey);

    status = ucp_put_nbi(shmem_transport_peers[pe].ep, source, len, (uint64_t) remote_addr, rkey);
    UCX_CHECK_STATUS_INPROGRESS(status);

    /* SOS expects scalar puts to complete locally. Use ucp_put_nbi in the hope
     * scalar puts are buffered/inlined and fix via quieting if not. If UCX
     * isn't providing immediate remote completion, ucp_put_nb may be more
     * efficient than ucp_put_nbi + quiet */
    if (status != UCS_OK)
        shmem_transport_quiet(ctx);
}

static inline
void
shmem_transport_put_nb(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                       int pe, long *completion)
{
    ucs_status_t status;
    ucp_rkey_h rkey;
    uint8_t *remote_addr;

    ucp_request_param_t param = {
        .op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_USER_DATA,
        .cb.send      = &shmem_transport_ucx_cb_complete,
        .user_data    = completion
    };

    shmem_transport_ucx_get_mr(target, pe, &remote_addr, &rkey);

    ucs_status_ptr_t pstatus = ucp_put_nbx(shmem_transport_peers[pe].ep, source,
                                           len, (uint64_t) remote_addr, rkey, &param);

    status = shmem_transport_ucx_post_cb_op(pstatus, completion);
    UCX_CHECK_STATUS_INPROGRESS(status);
}

static inline
void
shmem_transport_put_wait(shmem_transport_ctx_t* ctx, long *completion)
{
    while (__atomic_load_n(completion, __ATOMIC_ACQUIRE) > 0)
        shmem_transport_probe();
}

static inline
void
shmem_transport_put_nbi(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                       int pe)
{
    ucs_status_t status;
    ucp_rkey_h rkey;
    uint8_t *remote_addr;

    shmem_transport_ucx_get_mr(target, pe, &remote_addr, &rkey);

    status = ucp_put_nbi(shmem_transport_peers[pe].ep, source, len, (uint64_t) remote_addr, rkey);
    UCX_CHECK_STATUS_INPROGRESS(status);
}

static inline
void
shmem_transport_get(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len, int pe)
{
    ucs_status_ptr_t pstatus;
    ucp_rkey_h rkey;
    uint8_t *remote_addr;

    shmem_transport_ucx_get_mr(source, pe, &remote_addr, &rkey);

    pstatus = ucp_get_nb(shmem_transport_peers[pe].ep, target, len,
                         (uint64_t) remote_addr, rkey, &shmem_transport_ucx_cb_nop);

    ucs_status_t status = shmem_transport_ucx_complete_op(pstatus);
    UCX_CHECK_STATUS(status);
}

static inline
void
shmem_transport_get_wait(shmem_transport_ctx_t* ctx)
{
    /* Blocking fetching ops are completed in place, so this is a nop */
}


static inline
void
shmem_transport_swap(shmem_transport_ctx_t* ctx, void *target, const void *source, void *dest,
                     size_t len, int pe, shm_internal_datatype_t datatype)
{
    uint8_t *remote_addr;
    ucp_rkey_h rkey;
    ucs_status_ptr_t pstatus;
    uint64_t value;

    shmem_transport_ucx_get_mr(target, pe, &remote_addr, &rkey);

    switch (len) {
        case 1:
            value = (uint64_t) *(uint8_t*)source;
            break;
        case 2:
            value = (uint64_t) *(uint16_t*)source;
            break;
        case 4:
            value = (uint64_t) *(uint32_t*)source;
            break;
        case 8:
            value = (uint64_t) *(uint64_t*)source;
            break;
        default:
            RAISE_ERROR_MSG("Unsupported datatype len=%zu\n", len);
    }

    pstatus = ucp_atomic_fetch_nb(shmem_transport_peers[pe].ep, UCP_ATOMIC_FETCH_OP_SWAP, value,
                                  dest, len, (uint64_t) remote_addr, rkey,
                                  &shmem_transport_ucx_cb_nop);

    ucs_status_t status = shmem_transport_ucx_complete_op(pstatus);
    UCX_CHECK_STATUS(status);
}

static inline
void
shmem_transport_swap_nbi(shmem_transport_ctx_t* ctx, void *target, const void *source, void *dest,
                         size_t len, int pe, shm_internal_datatype_t datatype)
{
    uint8_t *remote_addr;
    ucp_rkey_h rkey;
    ucs_status_ptr_t pstatus;
    uint64_t value;

    shmem_transport_ucx_get_mr(target, pe, &remote_addr, &rkey);

    switch (len) {
        case 1:
            value = (uint64_t) *(uint8_t*)source;
            break;
        case 2:
            value = (uint64_t) *(uint16_t*)source;
            break;
        case 4:
            value = (uint64_t) *(uint32_t*)source;
            break;
        case 8:
            value = (uint64_t) *(uint64_t*)source;
            break;
        default:
            RAISE_ERROR_MSG("Unsupported datatype len=%zu\n", len);
    }

    pstatus = ucp_atomic_fetch_nb(shmem_transport_peers[pe].ep, UCP_ATOMIC_FETCH_OP_SWAP, value,
                                  dest, len, (uint64_t) remote_addr, rkey,
                                  &shmem_transport_ucx_cb_nop);

    /* Manual progress to avoid deadlock for application-level polling */
    shmem_transport_probe();

    ucs_status_t status = shmem_transport_ucx_release_op(pstatus);
    UCX_CHECK_STATUS_INPROGRESS(status);
}

static inline
void
shmem_transport_cswap(shmem_transport_ctx_t* ctx, void *target, const void *source, void *dest,
                      const void *operand, size_t len, int pe,
                      shm_internal_datatype_t datatype)
{
    uint8_t *remote_addr;
    ucp_rkey_h rkey;
    ucs_status_ptr_t pstatus;
    uint64_t value;

    shmem_transport_ucx_get_mr(target, pe, &remote_addr, &rkey);

    memcpy(dest, source, len);

    switch (len) {
        case 1:
            value = (uint64_t) *(uint8_t*)operand;
            break;
        case 2:
            value = (uint64_t) *(uint16_t*)operand;
            break;
        case 4:
            value = (uint64_t) *(uint32_t*)operand;
            break;
        case 8:
            value = (uint64_t) *(uint64_t*)operand;
            break;
        default:
            RAISE_ERROR_MSG("Unsupported datatype len=%zu\n", len);
    }

    pstatus = ucp_atomic_fetch_nb(shmem_transport_peers[pe].ep, UCP_ATOMIC_FETCH_OP_CSWAP,
                                  value, dest, len, (uint64_t) remote_addr, rkey,
                                  &shmem_transport_ucx_cb_nop);

    ucs_status_t status = shmem_transport_ucx_complete_op(pstatus);
    UCX_CHECK_STATUS(status);
}

static inline
void
shmem_transport_cswap_nbi(shmem_transport_ctx_t* ctx, void *target, const void *source, void *dest,
                          const void *operand, size_t len, int pe,
                          shm_internal_datatype_t datatype)
{
    uint8_t *remote_addr;
    ucp_rkey_h rkey;
    ucs_status_ptr_t pstatus;
    uint64_t value;

    shmem_transport_ucx_get_mr(target, pe, &remote_addr, &rkey);

    memcpy(dest, source, len);

    switch (len) {
        case 1:
            value = (uint64_t) *(uint8_t*)operand;
            break;
        case 2:
            value = (uint64_t) *(uint16_t*)operand;
            break;
        case 4:
            value = (uint64_t) *(uint32_t*)operand;
            break;
        case 8:
            value = (uint64_t) *(uint64_t*)operand;
            break;
        default:
            RAISE_ERROR_MSG("Unsupported datatype len=%zu\n", len);
    }

    pstatus = ucp_atomic_fetch_nb(shmem_transport_peers[pe].ep, UCP_ATOMIC_FETCH_OP_CSWAP,
                                  value, dest, len, (uint64_t) remote_addr, rkey,
                                  &shmem_transport_ucx_cb_nop);

    /* Manual progress to avoid deadlock for application-level polling */
    shmem_transport_probe();

    ucs_status_t status = shmem_transport_ucx_release_op(pstatus);
    UCX_CHECK_STATUS_INPROGRESS(status);
}

static inline
void
shmem_transport_atomic(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                       int pe, shm_internal_op_t op, shm_internal_datatype_t datatype)
{
    uint8_t *remote_addr;
    ucp_rkey_h rkey;
    ucs_status_t status;
    uint64_t value;

    shmem_transport_ucx_get_mr(target, pe, &remote_addr, &rkey);

    shmem_internal_assert(op <= SHMEM_TRANSPORT_UCX_OP_LAST);

    switch (len) {
        case 1:
            value = (uint64_t) *(uint8_t*)source;
            break;
        case 2:
            value = (uint64_t) *(uint16_t*)source;
            break;
        case 4:
            value = (uint64_t) *(uint32_t*)source;
            break;
        case 8:
            value = (uint64_t) *(uint64_t*)source;
            break;
        default:
            RAISE_ERROR_MSG("Unsupported datatype len=%zu\n", len);
    }

    status = ucp_atomic_post(shmem_transport_peers[pe].ep, shmem_transport_ucx_post_op[op],
                             value, len, (uint64_t) remote_addr, rkey);
    UCX_CHECK_STATUS_INPROGRESS(status);
}

static inline
void
shmem_transport_atomicv(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t count, size_t type_size,
                        int pe, shm_internal_op_t op, shm_internal_datatype_t datatype, long *completion)
{
    for (size_t i = 0; i < count; i++) {
            shmem_transport_atomic(ctx, ((uint8_t *) target) + (i * type_size),
                                   ((uint8_t *) source) + (i * type_size), type_size,
                                   pe, op, datatype);
    }
}

static inline
void
shmem_transport_fetch_atomic(shmem_transport_ctx_t* ctx, void *target, const void *source, void *dest, size_t len,
                             int pe, shm_internal_op_t op, shm_internal_datatype_t datatype)
{
    uint8_t *remote_addr;
    ucp_rkey_h rkey;
    ucs_status_ptr_t pstatus;
    uint64_t value;

    shmem_transport_ucx_get_mr(target, pe, &remote_addr, &rkey);

    shmem_internal_assert(op <= SHMEM_TRANSPORT_UCX_OP_LAST);

    switch (len) {
        case 1:
            value = (uint64_t) *(uint8_t*)source;
            break;
        case 2:
            value = (uint64_t) *(uint16_t*)source;
            break;
        case 4:
            value = (uint64_t) *(uint32_t*)source;
            break;
        case 8:
            value = (uint64_t) *(uint64_t*)source;
            break;
        default:
            RAISE_ERROR_MSG("Unsupported datatype len=%zu\n", len);
    }

    pstatus = ucp_atomic_fetch_nb(shmem_transport_peers[pe].ep,
                                  shmem_transport_ucx_fetch_op[op], value,
                                  dest, len, (uint64_t) remote_addr, rkey,
                                  &shmem_transport_ucx_cb_nop);

    ucs_status_t status = shmem_transport_ucx_complete_op(pstatus);
    UCX_CHECK_STATUS(status);
}

static inline
void
shmem_transport_fetch_atomic_nbi(shmem_transport_ctx_t* ctx, void *target, const void *source, void *dest, size_t len,
                                 int pe, shm_internal_op_t op, shm_internal_datatype_t datatype)
{
    uint8_t *remote_addr;
    ucp_rkey_h rkey;
    ucs_status_ptr_t pstatus;
    uint64_t value;

    shmem_transport_ucx_get_mr(target, pe, &remote_addr, &rkey);

    shmem_internal_assert(op <= SHMEM_TRANSPORT_UCX_OP_LAST);

    switch (len) {
        case 1:
            value = (uint64_t) *(uint8_t*)source;
            break;
        case 2:
            value = (uint64_t) *(uint16_t*)source;
            break;
        case 4:
            value = (uint64_t) *(uint32_t*)source;
            break;
        case 8:
            value = (uint64_t) *(uint64_t*)source;
            break;
        default:
            RAISE_ERROR_MSG("Unsupported datatype len=%zu\n", len);
    }

    pstatus = ucp_atomic_fetch_nb(shmem_transport_peers[pe].ep,
                                  shmem_transport_ucx_fetch_op[op], value,
                                  dest, len, (uint64_t) remote_addr, rkey,
                                  &shmem_transport_ucx_cb_nop);

    /* Manual progress to avoid deadlock for application-level polling */
    shmem_transport_probe();

    ucs_status_t status = shmem_transport_ucx_release_op(pstatus);
    UCX_CHECK_STATUS_INPROGRESS(status);
}

static inline
void
shmem_transport_atomic_fetch(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                             int pe, shm_internal_datatype_t datatype)
{
    uint8_t *remote_addr;
    ucp_rkey_h rkey;
    ucs_status_ptr_t pstatus;

    shmem_transport_ucx_get_mr(source, pe, &remote_addr, &rkey);

    pstatus = ucp_atomic_fetch_nb(shmem_transport_peers[pe].ep, UCP_ATOMIC_FETCH_OP_FADD, 0,
                                  target, len, (uint64_t) remote_addr, rkey,
                                  &shmem_transport_ucx_cb_nop);

    ucs_status_t status = shmem_transport_ucx_complete_op(pstatus);
    UCX_CHECK_STATUS(status);
}

static inline
void
shmem_transport_atomic_set(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                             int pe, shm_internal_datatype_t datatype)
{
    uint8_t *remote_addr;
    ucp_rkey_h rkey;
    ucs_status_ptr_t pstatus;
    uint64_t value;

    /* XXX: Set is implemented as swap, so dest is thrown away. Allocate dest
     * as a static rather than on the stack to avoid needing to block on
     * completion before returning. */
    static uint64_t dest;

    shmem_transport_ucx_get_mr(target, pe, &remote_addr, &rkey);

    switch (len) {
        case 1:
            value = (uint64_t) *(uint8_t*)source;
            break;
        case 2:
            value = (uint64_t) *(uint16_t*)source;
            break;
        case 4:
            value = (uint64_t) *(uint32_t*)source;
            break;
        case 8:
            value = (uint64_t) *(uint64_t*)source;
            break;
        default:
            RAISE_ERROR_MSG("Unsupported datatype len=%zu\n", len);
    }

    pstatus = ucp_atomic_fetch_nb(shmem_transport_peers[pe].ep, UCP_ATOMIC_FETCH_OP_SWAP, value,
                                  &dest, len, (uint64_t) remote_addr, rkey,
                                  &shmem_transport_ucx_cb_nop);

    ucs_status_t status = shmem_transport_ucx_release_op(pstatus);
    UCX_CHECK_STATUS_INPROGRESS(status);
}

static inline
void
shmem_transport_mswap(shmem_transport_ctx_t* ctx, void *target, const void *source, void *dest,
                      const void *mask, size_t len, int pe,
                      shm_internal_datatype_t datatype)
{
    uint8_t *remote_addr;
    ucp_rkey_h rkey;
    int done = 0;

    shmem_transport_ucx_get_mr(target, pe, &remote_addr, &rkey);

    if (len != 4)
        RAISE_ERROR_STR("Unsupported datatype");

    /* XXX: Emulate MSWAP, since it is not exposed by UCX */
    while (!done) {
        uint32_t v;

        shmem_transport_atomic_fetch(ctx, &v, target, len, pe, datatype);

        uint32_t new = (v & ~*(uint32_t *)mask) | (*(uint32_t *)source & *(uint32_t *)mask);

        shmem_transport_cswap(ctx, target, &new, dest, &v, len, pe, datatype);
        if (*(uint32_t *)dest == v) done = 1;

        /* Manual progress to avoid deadlock for application-level polling */
        shmem_transport_probe();
    }
}

static inline
int shmem_transport_atomic_supported(shm_internal_op_t op, shm_internal_datatype_t datatype)
{
    /* Use software reductions, instead of atomic vector operation */
    return 0;
}

static inline
void shmem_transport_syncmem(void)
{
    return;
}

static inline
void
shmem_transport_put_signal_nbi(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                               uint64_t *sig_addr, uint64_t signal, int sig_op, int pe)
{
    shmem_transport_put_nbi(ctx, target, source, len, pe);
    shmem_transport_fence(ctx);
    switch (sig_op) {
        case SHMEM_SIGNAL_ADD:
            shmem_transport_atomic(ctx, sig_addr, &signal, sizeof(uint64_t),
                                   pe, SHM_INTERNAL_SUM, SHM_INTERNAL_UINT64);
            break;
        case SHMEM_SIGNAL_SET:
            shmem_transport_atomic_set(ctx, sig_addr, &signal, sizeof(uint64_t),
                                       pe, SHM_INTERNAL_UINT64);
            break;
        default:
            RAISE_ERROR_MSG("Unsupported operation (%d)\n", sig_op);
    }
}

/*** Functions below are not supported ***/

static inline
void shmem_transport_ct_create(shmem_transport_ct_t **ct_ptr)
{
    RAISE_ERROR_STR("No path to peer");
}

static inline
void shmem_transport_ct_free(shmem_transport_ct_t **ct_ptr)
{
    RAISE_ERROR_STR("No path to peer");
}

static inline
long shmem_transport_ct_get(shmem_transport_ct_t *ct)
{
    RAISE_ERROR_STR("No path to peer");
    return 0;
}

static inline
void shmem_transport_ct_set(shmem_transport_ct_t *ct, long value)
{
    RAISE_ERROR_STR("No path to peer");
}

static inline
void shmem_transport_ct_wait(shmem_transport_ct_t *ct, long wait_for)
{
    RAISE_ERROR_STR("No path to peer");
}

static inline
void
shmem_transport_put_ct_nb(shmem_transport_ct_t *ct, void *target, const void
                          *source, size_t len, int pe, long *completion)
{
    RAISE_ERROR_STR("No path to peer");
}

static inline
void shmem_transport_get_ct(shmem_transport_ct_t *ct, void
                            *target, const void *source, size_t len, int pe)
{
    RAISE_ERROR_STR("No path to peer");
}

/**
 * Query the value of the transport's received messages counter.
 */
static inline
uint64_t shmem_transport_received_cntr_get(void)
{
    RAISE_ERROR_STR("Transport does not support received counter");
    return 0;
}

/**
 * Wait for the transport's received messages counter to be greater than or
 * equal to the given value.
 *
 * @param ge_val Function returns when received messages >= ge_val
 */
static inline
void shmem_transport_received_cntr_wait(uint64_t ge_val)
{
    RAISE_ERROR_STR("Transport does not support received counter");
}

static inline
uint64_t shmem_transport_pcntr_get_issued_write(shmem_transport_ctx_t *ctx)
{
    return 0;
}

static inline
uint64_t shmem_transport_pcntr_get_issued_read(shmem_transport_ctx_t *ctx)
{
    return 0;
}

static inline
uint64_t shmem_transport_pcntr_get_completed_write(shmem_transport_ctx_t *ctx)
{
    return 0;
}

static inline
uint64_t shmem_transport_pcntr_get_completed_read(shmem_transport_ctx_t *ctx)
{
    return 0;
}

static inline
uint64_t shmem_transport_pcntr_get_completed_target(void)
{
    return 0;
}

static inline
void shmem_transport_pcntr_get_all(shmem_transport_ctx_t *ctx, shmemx_pcntr_t *pcntr)
{
    return;
}

#endif /* TRANSPORT_UCX_H */
