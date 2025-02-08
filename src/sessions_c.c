/* -*- C -*-
 *
 * Copyright (c) 2024 Intel Corporation. All rights reserved.
 * This software is available to you under the BSD license.
 *
 * This file is part of the Sandia OpenSHMEM software package. For license
 * information, see the LICENSE file in the top level directory of the
 * distribution.
 *
 */

#include "config.h"

#define SHMEM_INTERNAL_INCLUDE
#include "shmem.h"
#include "shmem_internal.h"
#include "transport.h"

#ifdef ENABLE_PROFILING
#include "pshmem.h"

#pragma weak shmem_session_start = pshmem_session_start
#define shmem_session_start pshmem_session_start

#pragma weak shmem_session_stop = pshmem_session_stop
#define shmem_session_stop pshmem_session_stop

#endif /* ENABLE_PROFILING */

SHMEM_FUNCTION_ATTRIBUTES void
shmem_session_start(shmem_ctx_t ctx, long options, const shmem_session_config_t *config, long config_mask)
{
    SHMEM_ERR_CHECK_INITIALIZED();

    int ret = shmem_transport_session_start((shmem_transport_ctx_t *) ctx, options, config, config_mask);
    if (0 != ret) {
        DEBUG_MSG("Session did not start correctly (%d)\n", ret);
    } else {
        DEBUG_MSG("Session started correctly, but all hints are currently ignored\n");
    }

    return;
}

SHMEM_FUNCTION_ATTRIBUTES void
shmem_session_stop(shmem_ctx_t ctx)
{
    SHMEM_ERR_CHECK_INITIALIZED();

    int ret = shmem_transport_session_stop((shmem_transport_ctx_t *) ctx);
    if (0 != ret) {
        DEBUG_MSG("Session didn't stop correctly (%d)\n", ret);
    } else {
        DEBUG_MSG("Session stopped successfully\n");
    }

    return;
}
