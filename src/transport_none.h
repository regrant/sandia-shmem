
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

#ifndef TRANSPORT_NONE_H
#define TRANSPORT_NONE_H

#include "shmem_internal.h"
#include "transport.h"


/* Operations */
enum shm_internal_op_t {
    SHM_INTERNAL_BAND,
    SHM_INTERNAL_BOR,
    SHM_INTERNAL_BXOR,
    SHM_INTERNAL_MIN,
    SHM_INTERNAL_MAX,
    SHM_INTERNAL_SUM,
    SHM_INTERNAL_PROD
};

typedef enum shm_internal_op_t shm_internal_op_t;
typedef int shmem_transport_ct_t;

struct shmem_transport_ctx_t {
    long options;
    struct shmem_internal_team_t *team;
};
typedef struct shmem_transport_ctx_t shmem_transport_ctx_t;

int shmem_transport_init(void);

static inline
int
shmem_transport_startup(void)
{
    return 0;
}

static inline
int
shmem_transport_fini(void)
{
    return 0;
}

static inline
void
shmem_transport_probe(void)
{
    return;
}

static inline
int
shmem_transport_ctx_create(struct shmem_internal_team_t *team, long options, shmem_transport_ctx_t **ctx)
{
    if (team == SHMEM_TEAM_INVALID)
        return 1;

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
    if (ctx == SHMEM_CTX_INVALID)
        return;
    else if (ctx == (shmem_transport_ctx_t *) SHMEM_CTX_DEFAULT)
        RAISE_ERROR_STR("Cannot destroy SHMEM_CTX_DEFAULT");
    else
        free(ctx);

    return;
}

static inline
int
shmem_transport_quiet(shmem_transport_ctx_t* ctx)
{
    return 0;
}

static inline
int
shmem_transport_fence(shmem_transport_ctx_t* ctx)
{
    return 0;
}

static inline
void
shmem_transport_put_scalar(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len, int pe)
{
    RAISE_ERROR_STR("No path to peer");
}

static inline
void
shmem_transport_put_nb(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                       int pe, long *completion)
{
    RAISE_ERROR_STR("No path to peer");
}

static inline
void
shmem_transport_put_signal_nbi(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                               uint64_t *sig_addr, uint64_t signal, int sig_op, int pe)
{
    RAISE_ERROR_STR("No path to peer");
}

static inline
void
shmem_transport_put_wait(shmem_transport_ctx_t* ctx, long *completion)
{
    /* No op */
}

static inline
void
shmem_transport_put_nbi(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                       int pe)
{
    RAISE_ERROR_STR("No path to peer");
}

static inline
void
shmem_transport_get(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len, int pe)
{
    RAISE_ERROR_STR("No path to peer");
}

static inline
void
shmem_transport_get_wait(shmem_transport_ctx_t* ctx)
{
    /* Nop */
}


static inline
void
shmem_transport_swap(shmem_transport_ctx_t* ctx, void *target, const void *source, void *dest,
                     size_t len, int pe, shm_internal_datatype_t datatype)
{
    RAISE_ERROR_STR("No path to peer");
}

static inline
void
shmem_transport_swap_nbi(shmem_transport_ctx_t* ctx, void *target, const void *source, void *dest,
                         size_t len, int pe, shm_internal_datatype_t datatype)
{
    RAISE_ERROR_STR("No path to peer");
}

static inline
void
shmem_transport_cswap(shmem_transport_ctx_t* ctx, void *target, const void *source, void *dest,
                      const void *operand, size_t len, int pe,
                      shm_internal_datatype_t datatype)
{
    RAISE_ERROR_STR("No path to peer");
}

static inline
void
shmem_transport_cswap_nbi(shmem_transport_ctx_t* ctx, void *target, const void *source, void *dest,
                          const void *operand, size_t len, int pe,
                          shm_internal_datatype_t datatype)
{
    RAISE_ERROR_STR("No path to peer");
}

static inline
void
shmem_transport_mswap(shmem_transport_ctx_t* ctx, void *target, const void *source, void *dest,
                      const void *mask, size_t len, int pe,
                      shm_internal_datatype_t datatype)
{
    RAISE_ERROR_STR("No path to peer");
}

static inline
void
shmem_transport_atomic(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                       int pe, shm_internal_op_t op, shm_internal_datatype_t datatype)
{
    RAISE_ERROR_STR("No path to peer");
}

static inline
void
shmem_transport_atomicv(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t count, size_t type_size,
                        int pe, shm_internal_op_t op, shm_internal_datatype_t datatype, long *completion)
{
    RAISE_ERROR_STR("No path to peer");
}

static inline
void
shmem_transport_fetch_atomic(shmem_transport_ctx_t* ctx, void *target, const void *source, void *dest, size_t len,
                             int pe, shm_internal_op_t op, shm_internal_datatype_t datatype)
{
    RAISE_ERROR_STR("No path to peer");
}

static inline
void
shmem_transport_fetch_atomic_nbi(shmem_transport_ctx_t* ctx, void *target, const void *source, void *dest, size_t len,
                                 int pe, shm_internal_op_t op, shm_internal_datatype_t datatype)
{
    RAISE_ERROR_STR("No path to peer");
}

static inline
void
shmem_transport_atomic_fetch(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                             int pe, shm_internal_datatype_t datatype)
{
    RAISE_ERROR_STR("No path to peer");
}

static inline
void
shmem_transport_atomic_set(shmem_transport_ctx_t* ctx, void *target, const void *source, size_t len,
                             int pe, shm_internal_datatype_t datatype)
{
    RAISE_ERROR_STR("No path to peer");
}

static inline
int shmem_transport_atomic_supported(shm_internal_op_t op, shm_internal_datatype_t datatype)
{
    return 0;
}

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
    RAISE_ERROR_STR("No remote peers");
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
    RAISE_ERROR_STR("No remote peers");
}

static inline
void shmem_transport_syncmem(void)
{
    return;
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

#endif /* TRANSPORT_NONE_H */
