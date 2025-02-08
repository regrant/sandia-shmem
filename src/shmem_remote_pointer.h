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

#ifndef SHMEM_REMOTE_POINTER_H
#define SHMEM_REMOTE_POINTER_H

#include "shmem_comm.h"


static inline void *
shmem_internal_ptr(const void *target, int pe)
{
    int node_rank;

    /* shmem_internal_get_shr_rank will only return a valid rank when on-node
     * comms are enabled.  Check for the local PE here to return the pointer
     * even when on-node comms are disabled. */
    if (pe == shmem_internal_my_pe)
        return (void *) target;

    // Only if regular load/stores are used to implement put/get!
    if (-1 != (node_rank = shmem_internal_get_shr_rank(pe))) {
#if USE_XPMEM
        return shmem_transport_xpmem_ptr(target, pe, node_rank);
#elif USE_MMAP
        return shmem_transport_mmap_ptr(target, pe, node_rank);
#else
        return NULL;
#endif
    } else {
        return NULL;
    }
}


#endif /* #ifndef SHMEM_REMOTE_POINTER_H */
